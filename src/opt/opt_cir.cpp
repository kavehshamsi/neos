/*
 * opt.cpp
 *
 *  Created on: Aug 29, 2018
 *      Author: kaveh
 */
#include "opt/opt.h"
#include "base/circuit_util.h"

namespace opt_ns {

void circuit_satsweep(circuit& cir, double time_budget) {
	SATCircuitSweep ssw(cir);
	ssw.time_budget = time_budget;
	ssw.sweep();
}

void circuit_satsweep(circuit& cir, SATsweep::sweep_params_t& params,
		double time_budget) {
	SATCircuitSweep ssw(cir, params);
	ssw.time_budget = time_budget;
	ssw.sweep();
}


/*
 * BDD sweeping methods
 */
void circuit_bddsweep(circuit& cir, int s, double time_budget) {
	BDDCircuitSweep bddswp(cir, s);
	bddswp.time_budget = time_budget;
	bddswp.sweep();
}

void BDDCircuitSweep::analyze() {

	std::cout << "called analyze for circuit BDDsweep\n";
	std::cout << "size limit is " << size_limit << "\n";
	auto tm = utl::_start_wall_timer();
/*
	//initialize classes
	classes.push_back(idset());
	foreach_wireAndid(*cir, w, wid) {
		if (w.isInter()
				&& !cir->isLatch_output(wid)
				//TODO: this prevents the merging of latches
				//&& !cir->isLatch_input(wid)
				) {
			classes[0].insert(wid);
			id2class[wid] = 0;
		}
	}
*/

	//std::cout << "size limit is " << size_limit << "\n";

	cir->add_const_to_map(wid2bddmap, _mgr.bddZero(), _mgr.bddOne());

	foreach_inid(*cir, inid) {
		wid2bddmap[inid] = _mgr.bddVar();
	}
	foreach_keyid(*cir, kid) {
		wid2bddmap[kid] = _mgr.bddVar();
	}

	const idvec& gate_order = cir->get_topsort_gates();

	bool premature = false;
	for (auto gid : gate_order) {
		analyze_time = utl::_stop_wall_timer(tm);
		if (time_budget != -1 && analyze_time > time_budget) {
			premature = true;
			break;
		}

		auto& g = cir->getcgate(gid);
		id gout = g.gfanout();

		/*for (auto gin : g.fanin) {
			assert(wid2bddmap.at(gin).nodeCount() <= size_limit);
		}*/

		BDD res = gate_bdd_op(g, _mgr, wid2bddmap, size_limit);

		if (res.getNode() == NULL) {
			res = wid2bddmap[gout] = _mgr.bddVar();
		}
		else {
			//std::cout << "node " << cir->wname(gout) << " has size " << _mgr.nodeCount({res}) << "\n";
			wid2bddmap[gout] = res;
		}
		//std::cout << "num nodes at " << cir->wname(gout) << " "  << _mgr.nodeCount({res}) << "\n";

		if (_is_not_in(res.getNode(), bdd2widmap)) {
			_create_new_singleton_class(gout);
			bdd2widmap[res.getNode()] = gout;
		}
		else {
			id eq = bdd2widmap.at(res.getNode());
			_add_to_class(eq, gout);
			//std::cout << "equal nodes " << cir->wname(gout) << "  " << cir->wname(bdd2widmap.at(res.getNode())) << "\n";
		}
	}

	assert(!premature);
	if (premature) {
		for (auto gid : gate_order) {
			id gout = cir->gfanout(gid);
			if (_is_not_in(gout, wid2bddmap)) {
				_create_new_singleton_class(gout);
			}
		}
	}

	analyze_time = utl::_stop_wall_timer(tm);
	std::cout << " time: " << analyze_time << "\n";

	print_equiv_classes();
}

// initialize classes as every wire in one class
void CircuitSweep::_init_classes_allone() {

	//initialize classes
	classes.push_back(idset());
	foreach_wire(*cir, w, wid) {
		if (w.isInter()
				&& !cir->isLatch_output(wid)
				//TODO: this prevents the merging of latches
				//&& !cir->isLatch_input(wid)
				) {
			classes[0].insert(wid);
			id2class[wid] = 0;
		}
	}

}


void SATCircuitSweep::_get_wire_order() {

	idvec wire_order_all = cir->get_topsort_wires();

	for (auto wid : wire_order_all)
		if (cir->isInter(wid) && !cir->isLatch_output(wid))
			wire_order.push_back(wid);

	{ uint order = 0;
		for (auto wid : wire_order) {
			ordermap[wid] = order++;
		}
	}

}


void SATCircuitSweep::_mine_sim_patterns_allsat() {
	sat_solver TS = *mp.S;
	for (uint i = 0; i < mp.num_patterns; i++) {
		TS.solve();
		std::vector<slit> cl;
		for(auto inid : cir->allins()) {
			slit p;
			bool inb = (rand() % 2) == 0;
			if (_is_in(inid, mp.vmap)) {
				p = mp.vmap.at(inid);
				inb = TS.get_value(p);
			}
			else
				p = inb ? TS.true_lit() : TS.false_lit();
			cl.push_back(inb ? p:~p);
			sim_data[inid].push_back(inb);
		}
		TS.add_clause(cl);
	}
}

void SATCircuitSweep::_analyze_fraig() {

	std::cout << "performing non-AIG (RBC) FRAIG\n";

	auto tm0 = utl::_start_wall_timer();

	// if pre-conditions exist do not do pre-simulation
	mp.num_patterns = _get_num_patterns(cir->nGates());

	if (mp.external_condition) {
		// use AllSAT to get 10 patterns under external condition
		_mine_sim_patterns_allsat();
	}
	else {
		// if no external condition just pick random patterns
		for (uint i = 0; i < mp.num_patterns; i++) {
			for(auto xid : cir->allins())
				sim_data[xid].push_back((rand() % 2) == 0);
		}
		mp.S->set_simp(true);
	}

	wire_order = cir->get_topsort_wires();
	{ uint i = 0; for (auto nid : wire_order) ordermap[nid] = i++; }

	// add input and constant variables to solver
	add_const_sat_anyway(*cir, *mp.S, mp.vmap);


	for (auto xid : cir->combins()) {
		if (_is_not_in(xid, mp.vmap)) {
			mp.vmap[xid] = mp.S->create_new_var();
		}
		nd2fgmap[xid] = xid;
		_create_new_singleton_class(xid);
	}

	zero_wid = cir->has_const0() ? cir->get_cconst0() : (cir->get_max_wid() + 1);
	one_wid = cir->has_const1() ? cir->get_cconst1() : (cir->get_max_wid() + 2);

	mp.vmap[zero_wid] = mp.S->false_lit();
	mp.vmap[one_wid] = mp.S->true_lit();
	nd2fgmap[zero_wid] = zero_wid;
	nd2fgmap[one_wid] = one_wid;

	cirfaninTab.clear();

	_create_new_singleton_class(zero_wid);
	_create_new_singleton_class(one_wid);
	sim_data[zero_wid] = dbitset(mp.num_patterns, 0);
	sim_data[one_wid] = dbitset(mp.num_patterns, 1);

	simclasses[dbitset(mp.num_patterns, 0)].insert(zero_wid);

	uint strashed_nodes = 0, equals = 0;

	// main fraig pass
	for (auto wid : wire_order) {

		id cgid = cir->wfanin0(wid);
		if (cgid == -1) continue;

		//std::cout << "now at wire " << cir->wname(wid) << " : " << wid << "\n";

		const auto& cg = cir->getcgate(cgid);

		// first check constness and one-level hashing
		gate fg; // fraig gate
		fg.setgfun(cg.gfun());
		fg.setfanout(cg.fo0());
		id2boolHmap consts_map;
		for (auto fanin : cg.fanins) {
			fg.fanins.push_back(nd2fgmap.at(fanin));
			if (fanin == zero_wid)
				consts_map[fanin] = 0;
			else if (fanin == one_wid)
				consts_map[fanin] = 1;

			if ( _is_not_in(nd2fgmap.at(fanin), sim_data) ) {
				neos_abort( "missing from sim data: " << cir->wname(nd2fgmap.at(fanin)) );
			}
		}

		gate tg = fg;
		int output_val;
		bool gate_consted;
		circuit::simplify_gate(fg, tg, consts_map, output_val, gate_consted);

		if (gate_consted) {
			std::cout << "gate strashed " << cir->wname(tg.fo0()) << " to " << output_val << "\n";
			assert(output_val != -1);
			id strashed_wid = output_val ? one_wid:zero_wid;
			_add_to_class(strashed_wid, wid);
			mp.vmap[wid] = mp.vmap.at(strashed_wid);

			nd2fgmap[wid] = strashed_wid;
			assert(_is_in(strashed_wid, sim_data));

			strashed_nodes++;
			strashed.insert(wid);
			continue;
		}

		cirFanin cf(fg);
		auto cfit = cirfaninTab.find(cf);
		if (cfit != cirfaninTab.end()) {
			id st = cfit->second;
			_add_to_class(st, wid);
			nd2fgmap[wid] = nd2fgmap.at(st);
			mp.vmap[wid] = mp.vmap.at(st);
			strashed_nodes++;
			strashed.insert(wid);
			continue;
		}

		// then try simulation
		auto tm = utl::_start_wall_timer();

		const dbitset& outvec = sim_data[wid] =
				ckt_util::simulate_gate_bitset(fg, sim_data);

		sim_time += utl::_stop_wall_timer(tm);

		auto simit = simclasses.find(outvec);

		mp.vmap[wid] = mp.S->create_new_var();
		add_gate_clause(*mp.S, fg, mp.vmap);

		bool resim_performed = false;
		int need_check = 0; // 0 no check 1: outvec 2: ~outvec
		// new simulation class
		if (simit == simclasses.end())
			need_check = 0;
		else
			need_check = 1;

		if (need_check == 0) {
			_create_new_singleton_class(wid);
			nd2fgmap[wid] = wid;
			simclasses[outvec].insert(wid);
			cirfaninTab[cf] = wid;
		}
		else {

			bool equal = false;

			const auto& simvec = (resim_performed) ? sim_data.at(wid):outvec;
			if (resim_performed) resim_performed = false;

			auto it = simclasses.find(simvec);

			idset candidates = it->second;
			for (auto cand : candidates) {
				if (cand == wid) continue;
				//std::cout << "testing " << aig->nName(nid) << "  " << aig->nName(cand) << " ";

				auto tm2 = utl::_start_wall_timer();
				int status = _test_equivalence_sat(wid, cand, *mp.S, mp.vmap, false,
						mp.assumps, mp.prop_limit, mp.reinforce_equality);
				sat_time += utl::_stop_wall_timer(tm2);

				sat_calls++;
				if ( status == sl_Undef) {
					timed_out++;
					//std::cout << " timout\n";
					//assert(0);
				}
				else if ( status == sl_True ) {
					//std::cout << " equal\n";
					equals++;
					_add_to_class(cand, wid);
					//assert(cand != aig->get_cconst0id());
					id cand_wid = nd2fgmap.at(cand);
					nd2fgmap[wid] = cand_wid;
					mp.vmap[wid] = mp.vmap.at(cand);
					equal = true;
					break;
				}
				else {
					//std::cout << "not equal\n";
					_record_cex_fraig();
					//assert(simclasses.at(sim_data.at(nid)).size() == 1);

					if (cex_count % 10 == 0) {
						_resimulate_with_cex(wid);
						break;
					}
				}
			}

			if (!equal) {
				_create_new_singleton_class(wid);
				cirfaninTab[cf] = wid;
				nd2fgmap[wid] = wid;
				simclasses[outvec].insert(wid);
			}
		}

		assert(_is_in(wid, nd2fgmap));

		analyze_time = utl::_stop_wall_timer(tm0);
	}

	V1(
	std::cout << "strashed nodes: " << strashed_nodes << "\n";
	std::cout << "equals: " << equals << "\n";
	_print_stats();
	std::cout << "timed out: " << timed_out << " / " << sat_calls << "\n";
	);

	V2(print_equiv_classes());
}


void SATCircuitSweep::_record_cex_fraig() {

	cex_count++;

	for (auto xid : cir->allins()) {
		bool val = mp.S->get_value(mp.vmap.at(xid));
		cex_data[xid].push_back(val);
	}

}

void SATCircuitSweep::_resimulate_with_cex(id curnid) {

	auto tm = utl::_start_wall_timer();
	/*std::cout << "now doing cex resimulation. trying to break apart "
			<< cir->wname(curnid) << "\n";*/
	V1(std::cout << "sim data size: " << sim_data.begin()->second.size() << "\n");

	for(auto xid : cir->allins()) {
		_append_dbitset(sim_data.at(xid), cex_data.at(xid));
	}

	uint new_cex_bits = (cex_data.begin())->second.size();
	_append_dbitset(sim_data.at(zero_wid), dbitset(new_cex_bits, 0));
	_append_dbitset(sim_data.at(one_wid), dbitset(new_cex_bits, 1));

	size_t simsz = simclasses.size();

	simclasses.clear();
	simclasses[sim_data.at(zero_wid)].insert(zero_wid);
	simclasses[sim_data.at(one_wid)].insert(one_wid);

	for (auto wid : wire_order) {
		if (_is_not_in(wid, strashed)) {

			id gid = cir->wfanin0(wid);
			if (gid == -1) continue;

			const auto& cg = cir->getcgate(gid);

			// first check constness and one-level hashing
			gate fg; // fraig gate
			fg.setgfun(cg.gfun());
			id2boolHmap consts_map;
			int32_t sz = -1;
			for (auto cgfin : cg.fanins) {
				fg.fanins.push_back(nd2fgmap.at(cgfin));
				id fanin = fg.fanins.back();
				if (sz == -1)
					sz = sim_data.at(fanin).size();
				else
					if (sim_data.at(fanin).size() != sz) {
						std::cout << "size mismatch at " << cir->wname(fanin) << "\n";
						std::cout << sz << " != "
								<< sim_data.at(fanin).size() << "\n";
					}
			}

			const dbitset& outvec =	sim_data[wid] =
					ckt_util::simulate_gate_bitset(fg, sim_data);

			simclasses[outvec].insert(wid);
		}

		if (wid == curnid)
			break;
	}
	assert(simsz != simclasses.size());

	sim_time += utl::_stop_wall_timer(tm);

	// assert(simclasses.size() != simsz);
	/*if (simclasses.size() != simsz) {
		std::cout << "simclasses broken down by " << simsz << "  " << simclasses.size() << "\n";
	}*/

	// clear cex data
	cex_data.clear();

}


/*
 * SAT sweeping methods
 */
void SATCircuitSweep::analyze() {

	if (use_fraig) {
		assert(!mp.clauses_present);
		_analyze_fraig();
	}
	else
		_analyze_cex_driven();

}

void SATCircuitSweep::_analyze_cex_driven() {

	std::cout << "called analyze for circuit SATsweep\n";
	assert(cir != nullptr);

	auto tm = utl::_start_wall_timer();

	mp.num_patterns = _get_num_patterns(cir->nWires());

	_init_classes_allone();
	_get_wire_order();

	// simulate for initial partition
	if (mp.num_patterns != 0) {
		id2boolvecmap sim_str_map;

		// initial random simulation
		for (uint i = 0; i < mp.num_patterns; i++) {
			id2boolmap sim_map;
			cir->fill_inputmap_random(sim_map);
			for (auto p : sim_map) {
				sim_str_map[p.first].push_back(p.second);
			}
		}
		_refine_by_simulation(sim_str_map);
	}

	// perform SAT-sweep for classes
	add_const_sat_necessary(*cir, *mp.S, mp.vmap);

	foreach_wire(*cir, w, wid) {
		if ( cir->isConst(wid) ) { }
		else if (_is_not_in(wid, mp.vmap)) {
			mp.vmap[wid] = mp.S->create_new_var();
		}
	}

	if (!mp.clauses_present)
		add_ckt_gate_clauses(*mp.S, *cir, mp.vmap);

	uint cl = 0;
	while (1) {
		_sweep_class_cex_driven(cl);
		if (!_pick_next_class(cl))
			break;
		analyze_time = utl::_stop_wall_timer(tm);
		if (time_budget != -1 && analyze_time > time_budget) {
			break;
		}
	}

	_classify_unresolved_nodes();

	analyze_time = utl::_stop_wall_timer(tm);

	_print_stats();

	V2(print_equiv_classes());

}

uint SATsweep::_get_num_patterns(uint num_nodes) {

	uint n;
/*
	switch (num_nodes) {
	case 0 ... 500 : n = 16; break;
	case 501 ... 1000 : n = 64; break;
	default : n = 32;
	}
	if (mp.external_condition)
		return n * 16;
*/
	return 2048;

}

void SATCircuitSweep::_classify_unresolved_nodes() {
	uint clsize = classes.size();
	for (uint cl = 0; cl < clsize; cl++) {
		if (classes[cl].size() > 1) {
			idset c = classes[cl];
			for (auto wid : c) {
				if (c.size() > 1 && _is_not_in(wid, proven_set)) {
					classes[cl].erase(wid);
					classes.push_back({wid});
					id2class[wid] = classes.size() - 1;
				}
			}
		}
	}
}

void SATCircuitSweep::_print_stats() {
	std::cout << "timed out: " << timed_out << " / " << sat_calls << "   num nodes: " << cir->nWires();
	std::cout << "  time: " <<  analyze_time << "\n";
	std::cout << " sim time: " << sim_time << "\n";
	std::cout << " ref time: " << ref_time << "\n";
	std::cout << " sat time: " << sat_time << "\n";
}

bool SATCircuitSweep::_pick_next_class(uint& cl) {

	if (mp.ordered_test) {
		for (uint i = order_pos; i < wire_order.size(); i++) {
			id wid = wire_order[i];
			uint cur_cl = id2class.at(wid);
			if (cur_cl != cl) {
				cl = cur_cl;
				order_pos = i;
				return true;
			}
		}
		return false;
	}
	else {
		cl++;
		if (cl >= classes.size())
			return false;
		return true;
	}
}

void SATCircuitSweep::_sweep_class_cex_driven(uint cl) {

	if (classes[cl].size() > 1) {
		l1:
		//idvec wids = utl::to_vec(classes[cl]);
		idvec wids;
		for (auto wid : wire_order) {
			if (_is_in(wid, classes[cl])) {
				wids.push_back(wid);
				if (wids.size() == classes[cl].size())
					break;
			}
		}
		{
			uint x = 0;
			for (uint y = x + 1; y < wids.size(); y++) {

				if ( _is_in(wids[x], dont_merge) ||
					 _is_in(wids[y], dont_merge) ) {
					_refine_by_unequal_pair(wids[x], wids[y]);
					goto l1;
				}


				if (_is_not_in(wids[x], proven_set) || _is_not_in(wids[y], proven_set)) {

					auto sat_tm = utl::_start_wall_timer();
					int status = _test_equivalence_sat(wids[x], wids[y], *mp.S, mp.vmap,
												false, mp.assumps, mp.prop_limit, mp.reinforce_equality);
					sat_time += utl::_stop_wall_timer(sat_tm);

					V2(std::cout << "order " << ordermap.at(wids[x]) << "  " << ordermap.at(wids[y]) << "\n";)
					sat_calls++;
					if ( status != 1) {
						if ( status == -1) {
							timed_out++;
						}
						else {
							V2(std::cout << "not equal " << cir->wname(wids[x]) << " and "
									<< cir->wname(wids[y]) << "\n");
							_simulate_cex();
							goto l1;

						}
					}
					else {
						proven_set.insert(wids[x]);
						proven_set.insert(wids[y]);
						V2(std::cout << "proven equal " << cir->wname(wids[x]) << " " << cir->wname(wids[y]) << "\n");
					}
				}
			}
		}
	}

}

void SATCircuitSweep::_refine_by_simulation(id2boolvecmap& sim_str_map) {

	int num_patterns = (*sim_str_map.begin()).second.size();

	auto tm = utl::_start_wall_timer();

	auto& gate_order = cir->get_topsort_gates();

	id2boolmap simmap;
	for (int i = 0; i < num_patterns; i++) {
		cir->add_const_to_map(simmap);
		foreach_inid(*cir, xid)
			simmap[xid] = sim_str_map[xid][i];
		foreach_keyid(*cir, kid)
			simmap[kid] = sim_str_map[kid][i];
		foreach_latch(*cir, dffid, dffout, dffin)
			simmap[dffout] = sim_str_map[dffout][i];

		for (auto gid : gate_order) {
			bool val = cir->simulate_gate(gid, simmap);
			simmap[cir->gfanout(gid)] = val;
			sim_str_map[cir->gfanout(gid)].push_back(val);
		}
	}

	sim_time += utl::_stop_wall_timer(tm);

/*
	for (auto aid : gate_order) {
		std::cout << ntk->nName(aid) << " : ";
		for (auto val : sim_str_map.at(cir->gfanout(gid))) {
			std::cout << val;
		}
		std::cout << "\n";
	}
*/

	tm = utl::_start_wall_timer();

	int cl_sz = classes.size();
	for (int cl = 0; cl < cl_sz; cl++) {
		if (classes[cl].size() > 1) {
			std::unordered_map<boolvec, idset> sim2class;

			// store simulation strings in hash-table
			for (auto n1 : classes[cl]) {
				V2(std::cout << cir->wname(n1) << ": " << utl::to_str(sim_str_map.at(n1)) << " ");
				if (_is_not_in(n1, sim_str_map)) {
					cir->write_bench("./bench/testing/dump.bench");
					neos_error("wire " << cir->wname(n1) << " missing from map");
				}
				sim2class[sim_str_map.at(n1)].insert(n1);
			}

			// if more than one string is present refine class
			if (sim2class.size() > 1) {
				int c = 0;
				for (auto& strp : sim2class) {
					V2(std::cout << "\nsim2class: " << utl::to_str(strp.first) << "\n";
					utl::printset(strp.second);
					std::cout << "\n";)
					if (c == 0) {
						classes[cl] = strp.second;
					}
					else {
						classes.push_back(strp.second);
						// std::cout << "classes size: " << classes.size() << "\n";
						for (auto nid : classes.back()) {
							id2class[nid] = classes.size() - 1;
						}
					}
					c++;
				}
			}
		}
	}

	ref_time += utl::_stop_wall_timer(tm);
}

void SATCircuitSweep::_simulate_cex() {

	id2boolvecmap sim_str_map;
	V2(std::cout << "cex : ");
	foreach_inid(*cir, inid) {
		V2(std::cout << mp.S->get_value(mp.vmap.at(inid)));
		sim_str_map[inid].push_back(mp.S->get_value(mp.vmap.at(inid)));
	}
	foreach_keyid(*cir, kid) {
		V2(std::cout << mp.S->get_value(mp.vmap.at(kid)));
		sim_str_map[kid].push_back(mp.S->get_value(mp.vmap.at(kid)));
	}
	foreach_latch(*cir, dffid, dffout, dffin) {
		V2(std::cout << mp.S->get_value(mp.vmap.at(dffout)));
		sim_str_map[dffout].push_back(mp.S->get_value(mp.vmap.at(dffout)));
	}
	V2(std::cout << "\n");


	if (!mp.external_condition && mp.num_dist_one != 0) {
		uint insize = sim_str_map.size();
		for (uint i = 0; i < mp.num_dist_one; i++) {
			uint r = rand() % insize;
			uint j = 0;
			for (auto p : sim_str_map) {
				if (r == j++)
					p.second.push_back(!p.second[0]);
				else
					p.second.push_back(p.second[0]);
			}
		}
		cir->add_const_to_map(sim_str_map, boolvec(mp.num_dist_one, 0),
				boolvec(mp.num_dist_one, 1));
	}
	else {
		cir->add_const_to_map(sim_str_map, boolvec({0}), boolvec({1}));
	}

	_refine_by_simulation(sim_str_map);

}

void CircuitSweep::_add_to_class(id wid1, id wid2) {
	id cl = id2class.at(wid1);
	classes[cl].insert(wid2);
	id2class[wid2] = cl;
}

void CircuitSweep::_create_new_singleton_class(id wid1) {
	classes.push_back(idset({wid1}));
	id2class[wid1] = classes.size() - 1;
}

void CircuitSweep::_refine_by_unequal_pair(id wid1, id wid2) {
	int cur_class = id2class.at(wid1);
	classes[cur_class].erase(wid2);
	classes.push_back(idset(wid2));
	id2class[wid2] = classes.size();
}

/*
 * returns -1 for timeout, 0 for not equal, 1 for equal
 */
int SATsweep::_test_equivalence_sat(id wid1, id wid2, sat_solver& S,
		id2litmap_t& vmap, bool inverted,
		const slitvec& assumps, int64_t propagate_limit, bool reinforce) {

	slit wslit1 = vmap.at(wid1);
	slit wslit2 = vmap.at(wid2);
	return _test_equivalence_sat(wslit1, wslit2, S, inverted, assumps, propagate_limit, reinforce);

}

int SATsweep::_test_equivalence_sat(slit wslit1, slit wslit2, sat_solver& S, bool inverted,
		const slitvec& assumps, int64_t propagate_limit, bool reinforce) {

	slitvec test1_lits = {~wslit1, inverted ? ~wslit2:wslit2};
	slitvec test2_lits = { wslit1, inverted ? wslit2:~wslit2};

	std::vector<slit> assumps_test = assumps;

	push_all(assumps_test, test1_lits);

	int test2 = sl_Undef;

	int test1 = S.solve_limited(assumps_test, propagate_limit);

	if (test1 == sl_False) {
		pop_all(assumps_test, test1_lits);
		push_all(assumps_test, test2_lits);
		test2 = S.solve_limited(assumps_test, propagate_limit);
	}

	if (test1 == sl_Undef || (test1 == sl_False && test2 == sl_Undef) ) {
		// timeout
		return sl_Undef;
	}
	else if ( test1 == sl_False && test2 == sl_False) {
		if (reinforce) {
			// FIXME: Have no idea if this actually helps
			S.add_clause(wslit1, ~wslit2);
			S.add_clause(~wslit1, wslit2);
			//S.simplify();
		}
		return sl_True;
	}
	else if ( test1 == sl_True || test2 == sl_True ) {
		return sl_False;
	}
	return sl_Undef;
}

void CircuitSweep::commit() {

	V2(std::cout << "called commit for circuit\n");

	id num_orig_nodes = cir->nGates();
	V1(std::cout << "orig gates: " << num_orig_nodes << "\n");

	// merge equivalent nodes
	_merge_equiv_nodes(*cir, classes, merges);

	id num_new_nodes = cir->nGates();
	V1(std::cout << "new gates: " << num_new_nodes << "\n");

	id saved_nodes = num_orig_nodes - num_new_nodes;
	float saving = (float)saved_nodes / num_orig_nodes * 100;
	printf("sweep saved gates : %lld - %lld = %lld (%%%.2f)\n",
			num_orig_nodes, saved_nodes, num_new_nodes, saving);

}

void _merge_equiv_nodes(circuit& cir, const std::vector<idset>& classes,
		std::vector<idpair>& merges) {

	auto tm = utl::_start_wall_timer();

	// always delete node that is later in the wire order
	const idvec& wire_order = cir.get_topsort_wires();

	for (uint cl = 0; cl < classes.size(); cl++) {
		auto& c = classes[cl];
		if (c.size() > 1) {
			// for some reason this is way more efficient than std::sort
			idvec wids;
			for (auto wid : wire_order) {
				if (_is_in(wid, c)) {
					wids.push_back(wid);
				}
			}

			for (uint i = 1; i < wids.size(); i++) {
				id tr = wids[i]; // to remove
				id tk = wids[0]; // to keep
				// assert(!cir.isOutput(wids[i]));
				if (cir.isGate(cir.wfanin0(tr))) {
					assert(cir.wire_exists(wids[0]));
					assert(cir.wire_exists(wids[i]));
					// std::cout << "removing " << cir.wname(to_remove) << " and keeping " << cir.wname(to_remain) << "\n";
					cir.merge_equiv_nodes(tk, tr);
					merges.push_back(idpair(tk, tr));
				}
			}
		}
	}

	cir.remove_dead_logic();
	assert(cir.error_check());

	V2(std::cout << "commit time: " << utl::_stop_wall_timer(tm) << "\n");

}


void CircuitSweep::print_equiv_classes() {
	std::cout << "\n\ncircuit classes\n";
	int i = 0;
	for (auto& c : classes) {
		if (c.size() > 1) {
			std::cout << "class: " << i++ << ": ";
			for (auto nid : c) {
				std::cout << cir->wname(nid) << " ";
			}
			std::cout << "\n";
		}
	}
}


} // namespace opt_ns
