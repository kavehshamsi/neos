/*
 * bmc.cpp
 *
 *  Created on: Sep 2, 2018
 *      Author: kaveh
 */

#include "mc/bmc.h"

namespace mc {

BmcCkt::BmcCkt(const circuit& cir, const oidset& frozens,
		const Bmc_config_t& config) : Bmc(config), bmcckt(cir) {

	// cir.write_bench();
	// assert(cir.nOutputs() == 1); // only single ended

	tranckt = cir;

	bmc_unr.set_tr(tranckt, frozens, false);

	//std::cout << "transition simplification : " << config.tran_simp << "\n";
	//std::cout << "relative simplification : " << config.relative_simp << "\n";

	frozenPis = frozens;
	update_frozen_cone();
	BmcS = new sat_solver(config.sat_simp);
	BmcS->reseed_solver();

}

void BmcCkt::update_frozen_cone() {

	// propagate frozenPis to a frozen cone
	// which is not unrolled

	frozenGates.clear();
	frozenWires = frozenPis;
	if (bmcckt.has_const0())
		frozenWires.insert(bmcckt.get_const0());
	if (bmcckt.has_const1())
		frozenWires.insert(bmcckt.get_const1());


	const idvec& gate_order = bmcckt.get_topsort_gates();

	for (auto gid : gate_order) {
		// cout << "gate order: " << bmcckt.wname(bmcckt.gfanout(gid)) << "\n";
		bool isFrozen = true;
		for (auto gfanin : bmcckt.gfanin(gid)) {
			if (_is_not_in(gfanin, frozenWires)) {
				assert(bmcckt.wname(gfanin) != "gnd" && bmcckt.wname(gfanin) != "vdd");
				isFrozen = false;
			}
		}
		if (isFrozen) {
			frozenWires.insert(bmcckt.gfanout(gid));
			frozenGates.insert(gid);
		}
	}

	for (auto gid : frozenGates) {
		assert(_is_in(bmcckt.gfanout(gid), frozenWires));
		for (auto fanin: bmcckt.gfanin(gid)) {
			assert(_is_in(fanin, frozenWires));
		}
	}

}


void BmcCkt::_add_frame_gate_clauses(sat_solver& S, const circuit& ckt,
		std::vector<id2litmap_t>& vmaps, int f, const oidset& frozen_gates) {

	foreach_gate(ckt, g, gid) {
		if ((f == 0 || _is_not_in(gid, frozen_gates)) && g.gfun() != fnct::DFF) {
			add_gate_clause(S, g, vmaps[f]);
		}
	}
}

bool BmcCkt::getValue(id wid, int f) {
	if (_is_not_in(wid, lmaps[f])) {
		std::cout << "simplified wire " << bmcckt.wname(wid) << "\n";
		return false;
	}
	return BmcS->get_value(lmaps.at(f).at(wid));
}

// external solve interface
int BmcCkt::bmc_solve(int newBound, id out) {

	if (out == -1) {
		out = *bmc_unr.tran.outputs().begin();
		std::cout << "bmc checking for output " << bmc_unr.tran.wname(out) << "\n";
	}

	// update_frozen_cone();

	//cout << "max bound : " << checkedBound << " new bound : " << newBound << "\n";

	if (checkedBound > newBound) {
		cout << "already passed this bound\n";
		assert(0); // this should not happen for now
	}

	// bmcckt.write_bench();
	assert(bmcckt.latches().size() != 0);

	return _solve_int(newBound, out);
}

void BmcCkt::restart_to_unrolled_ckt(const CircuitUnroll& new_cunr) {

	uint nr = new_cunr.cur_len();

	assert(!new_cunr.zero_init);
	assert(bmc_unr.cur_len() == new_cunr.cur_len());
	assert(bmc_unr.frozen_tr == new_cunr.frozen_tr);

	const auto& uc = new_cunr.uncir;
	const auto& tc = new_cunr.tran;

	bmc_unr = new_cunr;

	_restart_solver();

	umap.clear();
	lmaps = std::vector<id2litmap_t>(nr, id2litmap_t());

	bmc_assumps.clear();
	bmc_inits.clear();
	bmc_init_lit = BmcS->create_new_var(true);

	std::cout << " restarting to new circuit with nr : " << nr << "\n";

	// bmc_unr.uncir.write_bench();

	// take care of frozen wires first.
	// recreate frozen wires that have been removed
	idset ufrozens;
	for (auto fid : bmc_unr.frozen_tr) {
		id ufid = uc.find_wire(bmc_unr.tran.wname(fid));
		slit p = BmcS->create_new_var(true);
		for (uint f = 0; f < nr; f++) {
			lmaps[f][fid] = p;
		}

		if (ufid != -1) {
			ufrozens.insert(ufid);
			umap[ufid] = p;
		}
	}

	foreach_wire(uc, uw, uid) {
		slit p;
		if (_is_in(uid, ufrozens))
			continue;
		else if (uc.isConst(uid))
			p = uc.is_const1(uid) ? BmcS->true_lit():BmcS->false_lit();
		else
			p = BmcS->create_new_var();

		if (!uw.isInter()) {
			auto uf = _unframe_name(uc.wname(uid));

			if (uf.second > (int)nr)
				neos_error("uncir wire is out of cycle bound");

			id trwid = tc.find_wcheck(uf.first);

			lmaps[uf.second][trwid] = p;

			BmcS->setFrozen(p);

			// std::cout << uc.wname(uw) << " " << uf.second << " " << tc.wname(trwid) << "\n";
		}


		umap[uid] = p;
	}

	// add first and last states
	foreach_latch(tc, dffid, dffout, dffin) {
		auto nmo = tc.wname(dffout);
		auto nmi = tc.wname(dffin);

		// deal with init
		id uid_init = uc.find_wire(_init_name(nmo));
		slit p = lmaps[0][dffout] = (uid_init == -1) ? BmcS->create_new_var():umap.at(uid_init);
		BmcS->setFrozen(p);
		bmc_inits.push_back(~p);
		BmcS->add_clause(~bmc_init_lit, ~p);

		// link last round next-states
		id uid_last = bmc_unr.c2umaps[nr - 1].at(dffin);
		try {
			lmaps[nr - 1][dffin] = umap.at(uid_last);
		}
		catch(std::out_of_range&) {
			std::cout << "can't find " << uid_last << " tr " << tc.wname(dffin) << " at " << nr << "\n";
			assert(false);
		}

	}

	// add gate clauses
	foreach_gate(uc, ug, gid) {
		assert(!ug.isLatch());
		add_gate_clause(*BmcS, ug, umap);
	}

	cexLen = -1;
	currentlyUnrolled = nr;
	checkedBound = nr - 1;
}

int BmcCkt::_solve_int(int newBound, id out) {

	// bmcckt.write_bench();
	assert(bmcckt.latches().size() != 0);

	std::vector<slit> outslits;

	// solver consistency FIXME: get rid of
	assert(BmcS->solve());

	for (int f = checkedBound; f < newBound; f++) {

		if ( f + 1 > currentlyUnrolled) {

			// add latch variables
			bmc_unr.add_frame();

			// unraig.unaig.write_bench();
			lmaps.push_back(id2litmap_t());

			// move to SAT
			foreach_wire(bmc_unr.tran, tw, twid) {
				slit p;
				id uid = bmc_unr.c2umaps[f].at(twid);
				if ( bmc_unr.tran.isConst(twid) )
					p = bmc_unr.tran.is_const0(twid) ? BmcS->false_lit():BmcS->true_lit();
				// std::cout << bmc_unr.tran.wname(twid) << " -> " << bmc_unr.uncir.wname(uid) << "\n";
				else if (_is_in(uid, umap))
					p = umap.at(uid);
				else
					p = BmcS->create_new_var();

				umap[uid] = lmaps[f][twid] = p;

				if ( !tw.isInter() || bmc_unr.tran.isLatch_output(twid)
						|| bmc_unr.tran.isLatch_input(twid) ) {
					//std::cout << "setting as frozen " << bmc_unr.tran.wname(twid) << "\n";
					BmcS->setFrozen(p);
				}
			}

			foreach_gate(bmc_unr.tran, tg, tgid) {
				if (!tg.isLatch()) {
					idvec ufanins;
					for (auto tgin : tg.fanins) {
						ufanins.push_back(bmc_unr.c2umaps[f].at(tgin));
					}
					id ufanout = bmc_unr.c2umaps[f].at(tg.fo0());

					add_gate_clause(*BmcS, ufanins, ufanout, tg.gfun(), umap);
				}
			}

			// deal with init state
			if (f == 0) {
				bmc_init_lit = BmcS->create_new_var(true);
				for (auto uid : bmc_unr.init_wires) {
					// std::cout << "init wires " << bmc_unr.uncir.wname(uid) << "\n";
					slit s = umap.at(uid);
					bmc_inits.push_back(~s);
					BmcS->add_clause(~bmc_init_lit, ~s);
				}
			}

			currentlyUnrolled++;
			std::cout << "frame: " << f << "  stats -> "; BmcS->print_stats();
		}

		slit bmc_outlit;
		try { bmc_outlit = lmaps[f].at(out); } catch (std::out_of_range&) {
			neos_abort("bmc out " << bmc_unr.tran.wname(out) << " not found in litmap");
		};

		bmc_assumps.push_back(bmc_outlit);
		bmc_assumps.push_back(bmc_init_lit);

		if (BmcS->solve(bmc_assumps)) {
			/*neos_println("output " << bmc_unr.tran.wname(out) <<
					" asserted at f: " << f << "\n");*/
			bmc_assumps.pop_back();
			bmc_assumps.pop_back();
			cexLen = f;
			return 1;
		}
		else {
			// neos_println("UNSAT up to frame " << f);
			bmc_assumps.pop_back();
			bmc_assumps.pop_back();
			bmc_assumps.push_back(~bmc_outlit);
			checkedBound = f;
		}
	}

	//bmc_assumps.clear();

	cexLen = -1;
	checkedBound = newBound - 1;
	neos_println("no trace up to bound " << newBound);

	return 0;
}

void BmcCkt::_add_frame_wire_vars(sat_solver& S, const circuit& ckt,
			std::vector<id2litmap_t>& lmaps, int f, const oidset& frozen_inputs) {

	assert(f == (int)lmaps.size());
	lmaps.push_back(id2litmap_t()); // add a new frame map

	if (f == 0) {
		foreach_latch(ckt, dffgid, dffout, dffin) {
			// cout << "adding latch variable " << bmcckt.wname(dffout) << "\n";
			slit p = S.create_new_var();
			lmaps[f][dffout] = p;
			//S.add_clause(~p);
		}
	}
	else {
		foreach_latch(ckt, dffgid, dffout, dffin) {
			lmaps[f][dffout] = lmaps[f-1].at(dffin);
		}
	}

	// add other wire variables
	foreach_wire(ckt, w, wid) {
		//cout << "wire : " << ckt.getwire_name(wid) << " " << ckt.isLatch_output(wid) << " " << ckt.isLatch_input(wid) << "\n";
		if ( !ckt.isLatch_output(wid) ) {
			if (f == 0 || _is_not_in(wid, frozen_inputs)) {
				// cout << "adding wire variable " << bmcckt.wname(wid) << "\n";
				lmaps[f][wid] = S.create_new_var();
			}
			else {
				lmaps[f][wid] = lmaps[0].at(wid);
			}
		}
	}

}

// split unrolled node names to base and frame-number
void _split_unr_name(const std::string& node_name, std::string& base_name, int& f) {
	base_name = node_name.substr(0, node_name.length() - 4);
	try {
		f = std::stoi(node_name.substr(node_name.length() - 3, node_name.length()));
		//std::cout << "using frame" << f << " for " << node_name << "\n";
	}
	catch(std::invalid_argument&) {
		//std::cout << "using frame 0 for " << node_name << "\n";
		base_name = node_name;
		f = 0;
	}
}

void BmcCkt::_simplify_tr(int f) {
	switch (config.tran_simp) {
	case 1:
		_simplify_tr_single(f);
		break;
	default:
		assert(0);
		break;
	}
}


/* simplify last frame of tr only
 * using the BMC solver without the init condition
 * nothing to do when f = 0;
 * for f = 1 begin simplification
 */
void BmcCkt::_simplify_tr_single(int f) {

	using namespace opt_ns;

	if (f == 0) {
		trlmaps.clear();
		return;
	}

	float initial_ncount = tranckt.nWires();

	pop_all(bmc_assumps, bmc_inits);

	if (config.relative_simp) {

		SATsweep::sweep_params_t swp;
		swp.S = BmcS;
		swp.assumps = bmc_assumps;
		swp.clauses_present = true;
		swp.external_solver = true;
		swp.vmap = lmaps[f - 1];

		circuit_satsweep(tranckt, swp);
	}
	else {
		// will add vars for internal wires as well
		_add_frame_wire_vars(TrS, tranckt, trlmaps, f - 1, frozenWires);
		_add_frame_gate_clauses(TrS, tranckt, trlmaps, f - 1, frozenGates);

		SATsweep::sweep_params_t swp;
		swp.S = &TrS; swp.external_solver = true;
		swp.vmap = trlmaps[f - 1]; swp.clauses_present = true;

		// sweep automatically adds gate-clauses to the solver
		circuit_satsweep(tranckt, swp);
	}

	if (((float)tranckt.nWires() - initial_ncount)/initial_ncount < 0.1) {
		if (tran_simp_stagnant++ > 6) {
			std::cout << "no more gain. Disabling transition simplification\n";
			config.tran_simp = 0;
		}
	}
	else {
		tran_simp_stagnant = 0;
	}

	push_all(bmc_assumps, bmc_inits);
}


void _unroll_ckt_old(const circuit& cir, circuit& uncir,
		int numFrames, const oidset& frozens, bool zero_init) {

	std::vector<id2idmap> c2umaps;


	for (int f = 0; f < numFrames; f++) {
		c2umaps.push_back(id2idmap());

		if (cir.has_const0())
			c2umaps[f][cir.get_cconst0()] = uncir.get_const0();
		if (cir.has_const1())
			c2umaps[f][cir.get_cconst1()] = uncir.get_const1();


		// take care of latch inputs and outputs
		foreach_latch(cir, dffgid, dffout, dffin) {
			if (f == 0) {
				c2umaps[f][dffout] = (zero_init) ? uncir.get_const0()
						: uncir.add_wire(_init_name(cir.wname(dffout)), wtype::IN);
			}
			else {
				c2umaps[f][dffout] = c2umaps[f - 1].at(dffin);
			}
		}

		foreach_wire(cir, w, wid) {
			if (cir.isConst(wid))
				continue;

			std::string tnm = cir.wname(wid);
			if (_is_in(wid, frozens)) {
				if (f == 0) {
					// cout << "frozen while unr: " << cir.wname(wid) << "\n";
					// add frozen inputs once
					c2umaps[f][wid] = uncir.add_wire(tnm, w.wrtype());
				}
				else {
					c2umaps[f][wid] = c2umaps[0].at(wid);
				}
			}
			else {
				if (!cir.isLatch_output(wid)) {
					c2umaps[f][wid] = uncir.add_wire(_frame_name(tnm, f), w.wrtype());
				}
			}
		}

		for (auto oid : cir.outputs()) {
			id uid = c2umaps[f].at(oid);
			if (!uncir.isOutput(uid)) {
				id out = uncir.add_wire(_frame_name(cir.wname(oid), f), wtype::OUT);
				uncir.add_gate({c2umaps[f].at(oid)}, out, fnct::BUF);
				c2umaps[f][oid] = out;
			}
		}

		foreach_gate(cir, g, gid) {
			if (!cir.isLatch(gid)) {
				uncir.add_gate(_frame_name(cir.gname(gid), f), g, c2umaps[f]);
			}
		}
	}
}

}

