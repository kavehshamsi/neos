/*
 * bmc.cpp
 *
 *  Created on: Sep 2, 2018
 *      Author: kaveh
 */

#include "mc/bmc.h"

namespace mc {

BmcAig::BmcAig(const aig_t& ntk, const oidset& frozens,
		const Bmc_config_t& config) : Bmc(config), tranaig(ntk) {

	// cir.write_bench();
	// assert(cir.nOutputs() == 1); // only single ended

	neos_check(tranaig.nLatches() != 0,
			"no BMC for latchless circuits");

	unraig.set_tr(tranaig, frozens, false);

	//std::cout << "transition simplification : " << config.tran_simp << "\n";
	//std::cout << "relative simplification : " << config.relative_simp << "\n";

	frozenPis = frozens;
	BmcS = new sat_solver();
	BmcS->reseed_solver();

}


bool BmcAig::getValue(id wid, int f) {
	try {
		return BmcS->get_value(lmaps[f].at(wid));
	}
	catch (std::out_of_range&) {
		neos_abort("missing wire " << tranaig.ndname(wid));
	}
}

// tries to assert the out signal in newBound rounds
int BmcAig::bmc_solve(int newBound, id out) {

	cout << "already checked bound : "
			<< checkedBound << " new bound : " << newBound << "\n";

	if (checkedBound > newBound) {
		neos_abort("already checked this bound! must be UNSAT");
		return 0;
	}

	for (int f = checkedBound; f < newBound; f++) {


		if ( f + 1 > currentlyUnrolled) {

			assert(currentlyUnrolled == unraig.cur_len());

			// add latch variables
			unraig.add_frame();

			// unraig.unaig.write_bench();
			lmaps.push_back(id2litmap_t());

			if (f == 0) {
				bmc_init_lit = BmcS->create_new_var(true);
			}

			// move to SAT
			foreach_node_ordered(unraig.tran, nd, nid) {
				alit ua = unraig.c2umaps[f].at(nid);
				id unid = ua.lid();
				if (unid == unraig.unaig.get_cconst0id()) {
					//std::cout << "node " << unraig.tran.ndname(nid) << " consted in frame " << f << "\n";
					lmaps[f][nid] = ua.sgn() ? BmcS->true_lit():BmcS->false_lit();
				}
				else if (nd.isLatch()) {
					if (f == 0) {
						slit s = BmcS->create_new_var(true);
						bmc_inits.push_back(~s);
						BmcS->add_clause(~bmc_init_lit, ~s);
						lmaps[f][nid] = s;
					}
					else {
						lmaps[f][nid] =  lmaps[f - 1].at(nd.fi0());
					}
				}
				else if (nd.isInput()) {
					if (f == 0 || _is_not_in(nid, unraig.frozen_tr)) {
						lmaps[f][nid] = BmcS->create_new_var(true);
					}
					else {
						lmaps[f][nid] = lmaps[0].at(nid);
					}
				}
				else if (nd.isAnd() || nd.isOutput()) {
					if (_is_in(nid, unraig.new_ands) || nd.isOutput()) {
						lmaps[f][nid] = BmcS->create_new_var();

						alit f0 = unraig.c2umaps[f].at(nd.fi0());
						alit f1 = unraig.c2umaps[f].at(nd.fi1());

						add_aignode_clause(*BmcS, nid,
								nd.fanin0 ^ f0.sgn(),
								nd.fanin1 ^ f1.sgn(),
								lmaps[f]);
					}
					else {
						lmaps[f][nid] = umap.at(unid);
					}
				}

				umap[unid] = lmaps[f].at(nid);
			}

			currentlyUnrolled++;
			std::cout << "frame: " << f << "  stats -> "; BmcS->print_stats();
		}

		slit bmc_outlit;
		try { bmc_outlit = lmaps[f].at(out); } catch (std::out_of_range&) {
			neos_abort("bmc out " << tranaig.ndname(out) << " not found in litmap");
		};

		bmc_assumps.push_back(bmc_outlit);
		bmc_assumps.push_back(bmc_init_lit);

		//neos_println("checking output " << tranaig.ndname(out));

		if (BmcS->solve(bmc_assumps)) {
			neos_println("output " << tranaig.ndname(out) <<
					" asserted at f: " << f << "\n");
			bmc_assumps.pop_back();
			bmc_assumps.pop_back();
			cexLen = f;
			return 1;
		}
		else {
			neos_println("UNSAT up to frame " << f);
			bmc_assumps.pop_back();
			bmc_assumps.pop_back();
			checkedBound = f;
		}
	}

	cexLen = -1;
	checkedBound = newBound - 1;
	neos_println("no trace up to bound " << newBound);

	return 0;
}

void BmcAig::restart_to_unrolled(const AigUnroll& new_aunr, const std::string& outname) {

	int nr = new_aunr.cur_len();

	assert(!new_aunr.zero_init);
	assert(unraig.cur_len() == new_aunr.cur_len());
	assert(unraig.frozen_tr == new_aunr.frozen_tr);

	unraig = new_aunr;
	const auto& ua = unraig.unaig;
	const auto& ta = unraig.tran;

	_restart_solver();

	umap.clear();
	lmaps = std::vector<id2litmap_t>(nr, id2litmap_t());

	bmc_assumps.clear();
	bmc_inits.clear();
	bmc_init_lit = BmcS->create_new_var(true);

	std::cout << " restarting bmc-engine to unrolled network with nr : " << nr << "\n";

	// take care of frozen wires first
	// recreate frozen wires that have been removed
	idset done;
	for (auto tid : ta.inputs()) {
		if (_is_in(tid, unraig.frozen_tr)) {
			// std::cout << "in frozen : " << ta.ndname(tid) << "\n";
			slit p = BmcS->create_new_var(true);
			for (int f = 0; f < nr; f++) {
				//std::cout << "adding " << ta.ndname(tid) << " at frame " << f << "\n";
				lmaps[f][tid] = p;
			}
			id uid = ua.find_node(ta.ndname(tid));
			if (uid != -1) {
				done.insert(uid);
				//std::cout << "linking to uid " << ua.ndname(uid) << "\n";
				umap[uid] = p;
			}
		}
		else {
			for (int f = 0; f < nr; f++) {
				slit p = BmcS->create_new_var();
				lmaps[f][tid] = p;
				id uid = ua.find_node( _frame_name(ta.ndname(tid), f) );
				//std::cout << "at " << f << " non-frozen " << ta.ndname(tid) << " \n";
				if (uid != -1) {
					//std::cout << " linked to uid " << ua.ndname(uid) << "\n";
					done.insert(uid);
					umap[uid] = p;
				}
			}
		}
	}

	foreach_node(ua, nd, uid) {
		slit p;
		if (_is_in(uid, done)) {
			//std::cout << "skipping " << ua.ndname(uid) << "\n";
			continue;
		}
		else if (ua.isConst0(uid))
			p = BmcS->false_lit();
		else
			p = BmcS->create_new_var();

		auto uf = _unframe_name(ua.ndname(uid));

		if (uf.second > nr)
			neos_abort("uncir wire is out of cycle bound");

		id tid = ta.find_wcheck(uf.first);

		/*if (ta.getcNode(trwid).isLatch())
			std::cout << "adding latch " << ua.ndname(uid)
				<< " for tr " << ta.ndname(trwid) << "\n";*/

		lmaps[uf.second][tid] = p;

		/*if (uf.first == outname) {
			//std::cout << "output " << uf.first << " for " << uf.second << "\n";
			//bmc_assumps.push_back(p);
		}*/

		assert(unraig.c2umaps.at(uf.second).at(tid) == uid);

		umap[uid] = p;
	}

	// add initial states
	for (auto laid : ta.latches()) {
		const auto& ld = ta.getcNode(laid);
		const auto& lnm = ta.ndname(laid);
		id unlaid = ua.find_node(_init_name(lnm));
		//std::cout << "init latch " << ta.ndname(laid) << "  " << ua.ndname(unlaid) << "\n";
		slit fs = lmaps[0][laid] = (unlaid == -1) ? BmcS->create_new_var(true):lmaps.at(0).at(laid);
		alit ul = unraig.c2umaps[nr - 1].at(ld.fi0());
		/*std::cout << "latch mapping: " << ua.ndname(ul)
				<< " to " << ta.ndname(laid) << "\n";*/
		slit ls = lmaps[nr - 1][ld.fi0()] = umap.at(ul.lid()) ^ ul.sgn();
		BmcS->setFrozen(ls);
		BmcS->setFrozen(fs);
		bmc_inits.push_back(~fs);
		BmcS->add_clause(~bmc_init_lit, ~fs);
	}


	// add gate clauses
	foreach_node(ua, nd, nid) {
		if (nd.isAnd() || nd.isOutput())
			add_aignode_clause(*BmcS, nd, umap);
	}

	cexLen = -1;
	currentlyUnrolled = nr;
	checkedBound = nr - 1;
}

void BmcAig::_simplify_tr(int f) {
	neos_abort("under construction");
}


void _unroll_aig(const aig_t& aig, aig_t& unaig,
		int numFrames, const idset& frozens, bool zero_init) {

	id2alitmap c2umaps;

	const idvec& gorder = aig.top_order();
	assert(gorder.size() == aig.nNodes());

	foreach_node(aig, nd, nid) {
		if ( !_is_in(nid, gorder) ) {
			neos_error("node " << aig.ndname(nid) << " not in gorder\n");
		}
	}

	if (aig.has_const0())
		c2umaps[aig.get_cconst0id()] = unaig.get_const0();

	for (int f = 0; f < numFrames; f++) {

		// take care of latch inputs and outputs
		for (auto laid : aig.latches()) {
			if (f == 0) {
				c2umaps[laid] = (zero_init) ? unaig.get_const0id()
						: unaig.add_input(fmt("%s_%03d", aig.ndname(laid) % f));
			}
			else {
				const auto& nd = aig.getcNode(laid);
				c2umaps[laid] = c2umaps.at(nd.fi0()) ^ nd.cm0();
			}
		}


		// adding inputs
		for (auto nid : aig.inputs()) {
			if (_is_in(nid, frozens)) {
				if (f == 0)
					c2umaps[nid] = unaig.add_input(aig.ndname(nid));
			}
			else
				c2umaps[nid] = unaig.add_input(fmt("%s_%03d", aig.ndname(nid) % f));

			if (_is_in(nid, aig.keys()))
				unaig.add_to_keys(c2umaps.at(nid).lid());
		}

		// adding nodes
		for (auto nid : gorder) {

			const auto& nd = aig.getcNode(nid);

			if (nd.isAnd()) {
				c2umaps[nid] = unaig.add_and(nd, c2umaps, fmt("%s_%03d", aig.ndname(nid) % f));
			}
		}

		// add outputs
		for (auto oid : aig.outputs()) {
			const auto& ond = aig.getcNode(oid);
			unaig.add_output(fmt("%s_%03d", aig.ndname(oid) % f), c2umaps.at(ond.fi0()) ^ ond.cm0());
		}
	}
}

}

