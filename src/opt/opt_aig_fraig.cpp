/*
 * opt.cpp
 *
 *  Created on: Aug 29, 2018
 *      Author: kaveh
 */
#include <utl/ext_cmd.h>
#include "opt/opt.h"

namespace opt_ns {

void SATAigSweep::_collect_init_patterns() {

	auto tm0 = _start_wall_timer();

	// if pre-conditions exist do not do pre-simulation
	mp.num_patterns = _get_num_patterns(aig->nNodes());

	if (mp.external_condition) {
		// use AllSAT to get 10 patterns under external condition
		std::cout << "mining patterns with SAT\n";
		sat_solver TS = *mp.S;
		for (uint i = 0; i < mp.num_patterns; i++) {
			TS.solve();
			std::vector<slit> cl;
			foreach_inaid(*aig, inid) {
				slit p;
				bool inb = (rand() % 2) == 0;
				if (_is_in(inid, mp.vmap)) {
					p = mp.vmap.at(inid);
					inb = TS.get_value(p);
				}
				else {
					p = inb ? TS.true_lit() : TS.false_lit();
				}
				cl.push_back(inb ? p:~p);
				fgmp[inid].simvec.push_back(inb);
			}
			TS.add_clause(cl);
		}
	}
	else {
		// if no external condition just pick random patterns
		for (uint i = 0; i < mp.num_patterns; i++) {
			foreach_inaid(*aig, inid)
				fgmp[inid].simvec.push_back((rand() % 2) == 0);
		}
	}

	mine_time += utl::_stop_wall_timer(tm0);

}

void SATAigSweep::_init_fraig() {

	// mp.prop_limit = 5000;
	assert(mp.S);
	mp.S->reseed_solver();

	node_order = aig->top_andorder();
	utl::push_all(node_order, aig->outputs());
	{
		uint i = 0;
		for (auto nid : node_order) {
			ordermap[nid] = i++;
			fgmp[nid].mpnd = -1;
			fgmp[nid].strashed = false;
		}
	}

	// add input and constant variables to solver
	add_const_sat_anyway(*aig, *mp.S, mp.vmap);

	for (auto aid : aig->combins()) {
		if (_is_not_in(aid, mp.vmap)) {
			mp.vmap[aid] = mp.S->create_new_var();
		}
		fgmp[aid].mpnd = aid;
		_create_new_singleton_class(aid);
	}

	if (aig->has_const0()) {
		zero_alit = aig->get_cconst0();
	}
	else {
		// imaginary gnd
		zero_alit = aig->get_max_id() + 2;
		assert(!aig->node_exists(zero_alit));
	}

	mp.vmap[zero_alit.lid()] = mp.S->false_lit();
	fgmp[zero_alit.lid()].mpnd = zero_alit;

	_create_new_singleton_class(zero_alit);
	sim_data[zero_alit.lid()] = dbitset(mp.num_patterns, 0);

	simclasses[dbitset(mp.num_patterns, 0)].insert(zero_alit.lid());

}

int SATAigSweep::_test_equivalence_sat_vardec(id wid1, id wid2, bool inv) {

	idset fanins1, fanins2, tfanin1, tfanin2;
	if (wid1 != zero_alit.lid())
		tfanin1 = aig->trans_fanin(wid1, fanins1);
	else
		tfanin1 = {zero_alit.lid()};

	if (wid2 != zero_alit.lid())
		tfanin2 = aig->trans_fanin(wid2, fanins2);
	else
		tfanin2 = {zero_alit.lid()};


	for (const auto& p : mp.vmap) {
		mp.S->setDecVar(p.second, false);
	}

	for (auto x1 : tfanin1) {
		slit s = mp.vmap.at(x1);
		mp.S->setDecVar(s, true);
	}

	for (auto x1 : tfanin2) {
		slit s = mp.vmap.at(x1);
		mp.S->setDecVar(s, true);
	}


/*
	for (auto x1 : fanins1) {
		if (_is_in(x1, fanins2)) {
			slit s = mp.vmap.at(x1);
			mp.S->setDecVar(s, true);
		}
	}
*/

	//std::cout << mp.prop_limit << "\n";
	int status = _test_equivalence_sat(wid1, wid2, *mp.S, mp.vmap, inv, mp.assumps, mp.prop_limit);
	return status;
}

void SATAigSweep::_fraig_node(id nid) {

	// std::cout << "now at node " << aig->nName(nid) << "\n";
	auto& nd = aig->getNode(nid);

	if (nd.isOutput()) {
		alit x = fgmp[nid].mpnd = fgmp.at(nd.fi0()).mpnd ^ nd.cm0();
		mp.vmap[nid] = mp.vmap.at(x.lid()) ^ x.sgn();
		return;
	}

	// first check constness and one-level hashing
	alit mpf0 = fgmp.at(nd.fi0()).mpnd ^ nd.cm0();
	alit mpf1 = fgmp.at(nd.fi1()).mpnd ^ nd.cm1();
	alit retnd(-1, 0);

	// detect node_state
	if (mpf0 == mpf1)
		retnd = mpf0; // a.a = a
	else if (mpf0 == ~mpf1)
		retnd = zero_alit; // a.abar = 0
	else if (mpf0 == zero_alit)
		retnd = zero_alit;
	else if (mpf0 == ~zero_alit)
		retnd = mpf1;
	else if (mpf1 == zero_alit)
		retnd = zero_alit;
	else if (mpf1 == ~zero_alit)
		retnd = mpf0;


	if (retnd != -1) {
		_add_to_class(retnd, alit(nid, retnd.sgn()));
		mp.vmap[nid] = mp.vmap.at(retnd.lid()) ^ retnd.sgn();

		fgmp[nid].mpnd = retnd;
		fgmp[nid].strashed = true;

		//std::cout << "const strashed " << aig->nName(nid) << " to " << aigsweep_nName(retnd) << "\n";

		_fgst.num_strashed++;
		return;
	}

	aigFanin fgfanin(mpf0, mpf1);
	id st = aig_t::lookup_fanintable(fgfanin, strash_table);
	if (st != -1) {
		// std::cout << "strashed  " << fgfanin.fanin0 << " " << fgfanin.fanin1
		// << " to " << aig->getNode(st).fanin0 << "  " << aig->getNode(st).fanin1 << "\n";
		_add_to_class(st, nid);
		fgmp[nid].mpnd = fgmp.at(st).mpnd;
		fgmp[nid].strashed = true;

		mp.vmap[nid] = mp.vmap.at(st);

		_fgst.num_strashed++;
		return;
	}

	// then try simulation
	auto tm = utl::_start_wall_timer();
	const auto& vec0 = fgmp.at(mpf0.lid()).simvec;
	const auto& vec1 = fgmp.at(mpf1.lid()).simvec;

	const dbitset& outvec = fgmp[nid].simvec =
			(mpf0.sgn() ? ~vec0:vec0) &
			(mpf1.sgn() ? ~vec1:vec1);

	sim_time += utl::_stop_wall_timer(tm);

	auto it = simclasses.find(outvec);

	mp.vmap[nid] = mp.S->create_new_var();
	add_aignode_clause(*mp.S, nid, mpf0, mpf1, mp.vmap);

	bool resim_performed = false;
	int need_check = 0; // 0 no check 1: outvec 2: ~outvec
	// new simulation class
	if (it == simclasses.end()) {
		if (track_complement) {
		auto itb = simclasses.find(~outvec);
			if (itb == simclasses.end())
				need_check = 0;
			else need_check = 2;
		}
		else
			need_check = 0;
	}
	else need_check = 1;

	if (need_check == 0) {
		_create_new_singleton_class(nid);
		fgmp[nid].mpnd = nid;
		simclasses[outvec].insert(nid);
		strash_table[fgfanin] = nid;
	}
	else {

		bool equal = false;

		bool inv = (need_check == 2);

		const auto& simvec = (resim_performed) ? sim_data.at(nid):outvec;
		if (resim_performed) resim_performed = false;

		auto it = simclasses.find(inv ? ~simvec:simvec);

		idset candidates = it->second;
		for (auto cand : candidates) {
			if (cand == nid) continue;
			if ( fgmp.at(nid).satfail || fgmp.at(cand).satfail ) {
				//std::cout << "timeout predicted at " << nid << " or " << cand << "\n";
				continue;
			}

			//std::cout << "testing " << aig->nName(nid) << "  " << aig->nName(cand) << " ";
			//int status = _test_equivalence_sat(nid, cand, *mp.S, mp.vmap, inv, mp.assumps, mp.prop_limit);
			int status = _test_equivalence_sat_vardec(nid, cand, inv);
			sat_calls++;
			if ( status == sl_Undef) {
				timed_out++;
				//std::cout << " timout at " << aig->ndname(nid) << "\n";
				fgmp[nid].satfail = true;
				for (auto fo : aig->trans_fanout(nid)) {
					fgmp[nid].satfail = true;
				}
				//assert(0);
			}
			else if ( status == sl_True ) {
				//std::cout << " equal\n";
				equals++;
				_add_to_class(cand, alit(nid, inv));
				//assert(cand != aig->get_cconst0id());
				alit cand_al = fgmp.at(cand).mpnd;
				fgmp[nid].mpnd = inv ? ~cand_al:cand_al;
				mp.vmap[nid] = (cand_al.sgn() ^ inv) ? ~mp.vmap.at(cand):mp.vmap.at(cand);
				equal = true;
				break;
			}
			else {
				//std::cout << "not equal\n";
				resim_performed = _record_cex_fraig(nid);
				//assert(simclasses.at(sim_data.at(nid)).size() == 1);
				if (resim_performed)
					break;
			}
		}

		if (!equal) {
			_create_new_singleton_class(nid);
			strash_table[fgfanin] = nid;
			fgmp[nid].mpnd = nid;
			simclasses[outvec].insert(nid);
		}
	}

}


void SATAigSweep::_analyze_fraig() {

	// std::cout << "\nperforming FRAIG\n";
	assert(aig->error_check());

	auto tm0 = utl::_start_wall_timer();

	//mp.prop_limit = 10;

	_collect_init_patterns();

	_init_fraig();

	for (auto nid : node_order) {

		_fraig_node(nid);

		assert(_is_in(nid, mp.vmap));
		assert(_is_in(nid, fgmp));

		if (mit_status != sl_Undef)
			break;
	}

	std::cout << "done fraig " << mit_status << "\n";

	// final miter SAT check
	if (mit_status == sl_Undef) {
		for (auto oid : mit_outs) {
			const auto& nd = aig->getcNode(oid);
			int status = mp.S->solve_limited({mp.vmap.at(oid)}, mp.mitt_prop_limit);
			if (status == sl_True) {
				std::cout << "final SAT check activated miter " << aig->ndname(oid) << "\n";

				for (auto xid : aig->inputs()) {
					mit_active_cex[xid] = mp.S->get_value(mp.vmap.at(xid));
				}

				mit_status = sl_True;
			}
			else if (status == sl_Undef) {
				mit_status = sl_Undef;
			}
			else {
				std::cout << "final SAT check UNSAT. miter false.\n";
				mit_status = sl_False;
			}
		}
	}

	std::cout << "done with mit_status " << mit_status << "\n";

	analyze_time = utl::_stop_wall_timer(tm0);

	V1(_print_stats());

	//print_equiv_classes();

}

void SATAigSweep::_check_miter_out() {

	// simulate cex_data
	Hmap<id, dbitset> simmap;
	for (auto xid : aig->inputs()) {
		simmap[xid] = cex_data.at(xid);
	}
	uint cex_len = cex_data.begin()->second.size();
	aig->add_const_to_map(simmap, dbitset(cex_len, 0));

	for (auto nid : node_order) {
		const auto& nd = aig->getcNode(nid);

		const auto& vec0 = simmap.at(nd.fi0());
		const auto& vec1 = simmap.at(nd.fi1());

		simmap[nid] =
				(nd.cm0() ? ~vec0:vec0) &
				(nd.cm1() ? ~vec1:vec1);
	}

	// check if output was activated
	for (auto nid : mit_outs) {
		assert(aig->node_exists(nid));
		if ( simmap.at(nid).any() ) {
			std::cout << "miter output " << aig->ndname(nid)
					<< " activated during cex simulation: " << simmap.at(nid) << "\n";
			mit_active_cex.clear();
			for (uint i = 0; i < simmap.at(nid).size(); i++) {
				if ( simmap.at(nid)[i] ) {
					mit_activating_cex_index = i;

					for (auto xid : aig->inputs()) {
						mit_active_cex[xid] = simmap.at(xid)[i];
					}

					mit_status = sl_True;

					// verify cex
					/*std::cout << "cex size " << mit_active_cex.size() << "\n";
					id2boolmap vermap = mit_active_cex;
					for (auto xid : aig->inputs()) {
						std::cout << aig->ndname(xid) << " " << vermap.at(xid) << "\n";
						assert(_is_in(xid, vermap));
					}
					aig->simulate_comb(vermap);
					for (auto oid : mit_outs) {
						std::cout << "vermap: " << aig->ndname(oid) << " " << vermap.at(oid) << "\n";
					}*/

					break;
				}
			}
		}
	}


}

bool SATAigSweep::_record_cex_fraig(id curnid) {

	cex_count++;

	for (auto nid : aig->inputs()) {
		bool val = mp.S->get_value(mp.vmap.at(nid));
		cex_data[nid].push_back(val);
	}

	// resimulate and classify
	if (cex_count % 16 == 0 ) {

		auto tm = utl::_start_wall_timer();


		// std::cout << "now doing cex resimulation. trying to break apart " << aig->nName(curnid) << "\n";
		V2(std::cout << "sim data size: " << (*fgmp.begin()).second.simvec.size() << "\n";)

		for (auto nid : aig->inputs()) {
			_append_dbitset(fgmp.at(nid).simvec, cex_data.at(nid));
		}

		uint new_cex_bits = (cex_data.begin())->second.size();
		_append_dbitset(fgmp.at(zero_alit.lid()).simvec, dbitset(new_cex_bits, 0));

		size_t simsz = simclasses.size();

		simclasses.clear();
		simclasses[fgmp.at(zero_alit.lid()).simvec].insert(zero_alit.lid());
		for (auto nid : node_order) {
			if (!fgmp.at(nid).strashed) {
				const auto& nd = aig->getcNode(nid);
				alit mpf0 = fgmp.at(nd.fi0()).mpnd ^ nd.cm0();
				alit mpf1 = fgmp.at(nd.fi1()).mpnd ^ nd.cm1();

				const auto& vec0 = fgmp.at(mpf0.lid()).simvec;
				const auto& vec1 = fgmp.at(mpf1.lid()).simvec;

				if (vec0.size() != vec1.size()) {
					std::cout << "problem at " << aig->to_str(nid) << "\n";
					assert(false);
				}

				const dbitset& outvec =	fgmp[nid].simvec =
						(mpf0.sgn() ? ~vec0:vec0) &
						(mpf1.sgn() ? ~vec1:vec1);

				simclasses[outvec].insert(nid);
			}

			if ( nid == curnid )
				break;
		}
		assert(simsz != simclasses.size());

		// checking miter output
		if (!mit_outs.empty()) {
			_check_miter_out();
		}

		sim_time += utl::_stop_wall_timer(tm);

		// assert(simclasses.size() != simsz);
		/*if (simclasses.size() != simsz) {
			std::cout << "simclasses broken down by " << simsz << "  " << simclasses.size() << "\n";
		}*/

		// clear cex data
		cex_data.clear();
		return true;
	}

	return false;
}


} // namespace opt_ns
