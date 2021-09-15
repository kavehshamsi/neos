/*
 * opt.cpp
 *
 *  Created on: Aug 29, 2018
 *      Author: kaveh
 */
#include <utl/ext_cmd.h>
#include "opt/opt.h"

namespace opt_ns {


void AigSweep::_add_to_class(alit a1, alit a2) {
	id cl = id2class.at(a1.lid());
	bool sign = (classes[cl].find(a1|1) != classes[cl].end());
	classes[cl].insert(a2 ^ sign);
	id2class[a2.lid()] = cl;
}

void AigSweep::_create_new_singleton_class(alit a1) {
	classes.push_back( alitset({a1|1}) );
	id2class[a1.lid()] = classes.size() - 1;
}

void AigSweep::_refine_by_unequal_pair(alit a1, alit a2) {
	int cur_class = id2class.at(a1.lid());
	classes[cur_class].erase(a2);
	classes.push_back(alitset({a2}));
	id2class[a2.lid()] = classes.size();
}



void aig_cutsweep(aig_t& ntk) {
	CutSweep csw(ntk, 15, 30, 250);
	csw.sweep();
}

void aig_satsweep(aig_t& ntk) {
	SATAigSweep ssw(ntk);
	ssw.mp.mitt_prop_limit = ssw.mp.prop_limit = 5000;
	ssw.sweep();
}

void CutSweep::analyze() {
	_enumerate_cuts();
}

void CutSweep::_enumerate_cuts() {

	for (auto aid : aig->inputs()) {
		BDD var = mgr.bddVar();
		assert(_addCut(aid, var, oidset({aid})) == -1);
	}
	if (aig->has_const0()) {
		id nid = aig->get_cconst0().lid();
		_addCut(nid, mgr.bddOne(), oidset({nid}));
	}

	for (auto aid : aig->top_order()) {
		//ntk.print_node(aid);
		if (aig->isAnd(aid))
			_cutEnum(aid);
	}

}

id CutSweep::_addCut(id aid, BDD ndfun, const oidset& inputs) {
	cut_bdd_t cut;
	cut.inputs = inputs;
	cut.cutfun = ndfun;

	auto it = cut2id.find(cut);
	if (it != cut2id.end())
		return (*it).second;

	id2cuts[aid].push_back(cut);
	cut2id[cut] = aid;
	return -1;
}

void CutSweep::_cutEnum(id aid) {
	//std::cout << "now on node: ";
	//ntk.print_node(aid);

	_addCut(aid, mgr.bddVar(), oidset({aid}));

	auto& cutset0 = id2cuts.at(aig->nfanin0(aid));
	auto& cutset1 = id2cuts.at(aig->nfanin1(aid));

	uint cutset_size = 0;
	for (auto& cut0 : cutset0) {
		for (auto& cut1 : cutset1) {
			oidset newcut_inputs = utl::set_union(cut0.inputs, cut1.inputs);
			V1(std::cout << "new cut: ");
			V1(utl::printset(newcut_inputs));
			if (newcut_inputs.size() > (uint)k) {
				V1(std::cout << "cut too big\n");
				continue;
			}
			if (cutset_size++ > N)
				break;
			BDD z = (aig->ncmask0(aid) ? ~cut0.cutfun : cut0.cutfun)
					& (aig->ncmask1(aid) ? ~cut1.cutfun : cut1.cutfun);
			id v = _addCut(aid, z, newcut_inputs);
			if (v != -1) {
				_add_to_class(v, aid);
				//std::cout << "merge pair: " << aig->nName(aid) << "  " << aig->nName(v) << "\n";
				// merges.push_back(idpair(aid, v));
			}
			else {
				_create_new_singleton_class(aid);
			}
		}
	}
}

/*
 * SAT sweeping methods
 */
SATsweep::SATsweep() {
	mp.S = new sat_solver();
}
SATsweep::~SATsweep() {
	// when external solver is present don't delete it
	if (!mp.external_solver && mp.S != nullptr)
		delete mp.S;
}

SATsweep::SATsweep(sweep_params_t& params) : mp(params) {
	if (!mp.external_solver)
		mp.S = new sat_solver();
}

void AigSweep::_init_classes_allone() {
	classes.push_back(alitset());
	foreach_andid(*aig, aid) {
		classes[0].insert(alit(aid));
		id2class[aid] = 0;
	}
}

void SATAigSweep::analyze() {

	assert(aig != nullptr);

	if (use_fraig) {
		assert(!mp.clauses_present);
		_analyze_fraig();
	}
	else
		_analyze_cex_driven();
}


void SATAigSweep::_analyze_cex_driven() {

	std::cout << "called analyze for circuit SATsweep\n";
	assert(aig != nullptr);

	auto tm = utl::_start_wall_timer();

	mp.num_patterns = _get_num_patterns(aig->nNodes());

	_init_classes_allone();
	node_order = aig->top_andorder();
	{ uint i = 0; for (auto nid : node_order) ordermap[nid] = i++; }

	// simulate for initial partition
	if (mp.num_patterns != 0) {
		// simulate for initial partition
		id2boolvecmap sim_str_map;

		for (uint i = 0; i < mp.num_patterns; i++) {
			foreach_inaid(*aig, inid)
				sim_str_map[inid].push_back((rand() % 2) == 0);
		}

		_refine_by_simulation(sim_str_map);
	}

	// perform SAT-sweep for classes
	add_const_sat_anyway(*aig, *mp.S, mp.vmap);

	foreach_nodeid(*aig, aid) {
		if ( aig->isConst0(aid) ) {}
		else {
			mp.vmap[aid] = mp.S->create_new_var();
		}
	}

	if (!mp.clauses_present)
		add_aig_gate_clauses(*mp.S, *aig, mp.vmap);

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

	//_classify_unresolved_nodes();

	analyze_time = utl::_stop_wall_timer(tm);

	_print_stats();

	V2(print_equiv_classes());
}

bool SATAigSweep::_pick_next_class(uint& cl) {

	if (mp.ordered_test) {
		for (uint i = order_pos; i < node_order.size(); i++) {
			id wid = node_order[i];
			if (_is_not_in(wid, id2class))
				neos_error("problem with node " << aig->ndname(wid));
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


void SATAigSweep::_sweep_class_cex_driven(uint cl) {

	if (classes[cl].size() > 1) {
		l1:
		//idvec wids = utl::to_vec(classes[cl]);
		idvec wids;
		for (auto wid : node_order) {
			if (_is_in(wid, classes[cl])) {
				wids.push_back(wid);
				if (wids.size() == classes[cl].size())
					break;
			}
		}
		{
			uint x = 0;
			for (uint y = x + 1; y < wids.size(); y++) {

				if (  _is_in(wids[x], dont_merge) ||
					  _is_in(wids[y], dont_merge) ) {
					_refine_by_unequal_pair(wids[x], wids[y]);
					goto l1;
				}

				if (_is_not_in(wids[x], proven_set) || _is_not_in(wids[y], proven_set)) {
					int status = _test_equivalence_sat(wids[x], wids[y], *mp.S, mp.vmap,
												false, mp.assumps, mp.prop_limit);
					std::cout << "order " << ordermap.at(wids[x]) << "  " << ordermap.at(wids[y]) << "\n";
					sat_calls++;
					if ( status != 1) {
						if ( status == -1) {
							timed_out++;
						}
						else {
							V2(std::cout << "not equal " << aig->ndname(wids[x]) << " and "
									<< aig->ndname(wids[y]) << "\n");
							_simulate_cex();
							goto l1;
						}
					}
					else {
						proven_set.insert(wids[x]);
						proven_set.insert(wids[y]);
						V2(std::cout << "proven equal " << aig->ndname(wids[x]) << " " << aig->ndname(wids[y]) << "\n");
					}
				}
			}
		}
	}
}

void SATAigSweep::_print_stats() {
	std::cout << " timed out sat calls: " << timed_out << " / " << sat_calls << "   num nodes: " << aig->nNodes() << "\n";
	std::cout << " strashed nodes: " << _fgst.num_strashed << "\n";
	std::cout << " equals: " << equals << "\n";
	std::cout << " analyze time: " <<  analyze_time << "\n";
	std::cout << " sim time: " << sim_time << "\n";
	std::cout << " ref time: " << ref_time << "\n";
	std::cout << " mine time: " << mine_time << "\n";
	std::cout << " sat time: " << sat_time << "\n";
}



void AigSweep::commit() {


	// aig_t precommit = *aig;

	id num_orig_nodes = aig->nAnds();

	auto tm = utl::_start_wall_timer();

	// merge equivalent nodes
	_merge_equiv_nodes(*aig, classes, dont_merge, zero_alit);

	V1(
	commit_time = utl::_stop_wall_timer(tm);
	std::cout << "commit time: " << commit_time << "\n";
	id num_new_nodes = aig->nAnds();
	id saved_nodes = num_orig_nodes - num_new_nodes;

	float saving = (float)saved_nodes / num_orig_nodes * 100;
	printf("sweep saved gates : %lld - %lld = %lld (%%%.2f)\n",
			num_orig_nodes, saved_nodes, num_new_nodes, saving);
	);

	/*std::cout << "checking equivalence of AIG sweep\n";
	assert(ext::check_equivalence_abc(precommit, *aig));*/

}


// TODO: this does not work
void SATAigSweep::_refine_by_simulation(id2boolvecmap& sim_str_map) {

	int num_patterns = (*sim_str_map.begin()).second.size();
	V2(std::cout << "num patterns: " << num_patterns << "\n";)

	auto tm = utl::_start_wall_timer();

	id2boolmap simmap;
	// set inputs
	for (int i = 0; i < num_patterns; i++) {
		foreach_inaid(*aig, nid) {
			simmap[nid] = sim_str_map[nid][i];
		}
		// simulate nodes
		foreach_and_ordered(*aig, nid) {
			bool val = aig->simulate_node(nid, simmap);
			simmap[nid] = val;
			sim_str_map[nid].push_back(val);
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
			std::unordered_map<boolvec, alitset> sim2class;

			// store simulation strings in hash-table
			for (auto n1 : classes[cl]) {
				//V2(std::cout << ntk->nName(n1) << ": " << utl::_to_str(sim_str_map.at(n1)) << " ";)
				sim2class[sim_str_map.at(n1.lid())].insert(n1);
			}

			// if more than one string is present refine class
			if (sim2class.size() > 1) {
				int c = 0;
				for (auto& strp : sim2class) {
					V2(std::cout << "sim2class: " << utl::to_str(strp.first) << "\n";
					utl::printset(strp.second);
					std::cout << "\n";)

					if (c == 0) {
						classes[cl] = strp.second;
					}
					else {
						classes.push_back(strp.second);
						for (auto nd : strp.second) {
							id2class[nd.lid()] = classes.size() - 1;
						}
					}
					c++;
				}
			}
		}
	}
	ref_time += utl::_stop_wall_timer(tm);

	assert(classes.size() <= aig->nNodes());

}

void SATAigSweep::_simulate_cex() {

	uint cex_count = 0;
	id2boolvecmap sim_str_map;
	int status = 0;
	do {
		V2(std::cout << "cex : ";)
		std::vector<slit> cex_clause;
		foreach_inaid(*aig, nid) {
			bool val = mp.S->get_value(mp.vmap.at(nid));
			V2(std::cout << val;)
			sim_str_map[nid].push_back(val);
			cex_clause.push_back(val ? ~mp.vmap.at(nid):mp.vmap.at(nid));
		}
		V2(std::cout << "\n";)
		status = mp.S->solve(cex_clause);
	} while (cex_count++ < mp.num_dist_one && status);
	_refine_by_simulation(sim_str_map);
}

void _merge_equiv_nodes(aig_t& aig, const std::vector<alitset>& classes, const idset& dont_merge, alit zero) {

	// always delete node that is later in the wire order
	aig.clear_orders();
	idvec node_order = aig.top_order();

	assert(!aig.node_exists(zero) || aig.isConst0(zero.lid()));


	for (size_t cl = 0; cl < classes.size(); cl++) {
		const auto& c = classes.at(cl);
		// std::cout << "at class " << cl << " : with " << c.size() << "\n";
		if (c.size() > 1) {
			alitvec eqnodes;
			// sort nodes
			// take care of consts
			id zero_id = zero.lid();

			if (_is_in(zero, c)) {
				eqnodes.push_back(aig.get_const0());
				zero_id = aig.get_const0id();
			}
			else if(_is_in(~zero, c)) {
				eqnodes.push_back(~aig.get_const0());
				zero_id = aig.get_const0id();
			}

			idset visited;
			for (auto nid : node_order) {
				if (nid != zero_id) {
					auto it = c.find(alit(nid));
					auto itb = c.find(alit(nid, 1));
					auto itf = (it == c.end()) ? itb:it;
					if (it != c.end() || itb != c.end()) {
						eqnodes.push_back(*itf);
						if (eqnodes.size() == c.size())
							break;
					}
				}
			}

			id keepid = eqnodes[0].lid();
			for (size_t i = 1; i < eqnodes.size(); i++) {

				id removeid = eqnodes[i].lid();

				if ( _is_in(keepid, dont_merge) )
					break;

				if ( _is_not_in(removeid, dont_merge) && aig.node_exists(keepid)
						&& aig.node_exists(removeid)) {
					if (!aig.isAnd(removeid)) {
						std::cout << "trying to merge " << aig.ndname(eqnodes[i])
								<< " to " << aig.ndname(eqnodes[0]) << "\n";
						neos_abort("merge error");
					}
					V2(std::cout << "merging " << aig.ndname(eqnodes[i])
							<< " and " << aig.ndname(eqnodes[0]) << "\n");
					aig.merge_nodes(removeid, keepid, eqnodes[0].sgn() ^ eqnodes[i].sgn());
				}
			}
		}
	}

	aig.remove_dead_logic();

}

std::string AigSweep::aigsweep_nName(alit al) {
	std::string nm;
	if (al.lid() == zero_alit.lid())
		nm = "gnd";
	else
		nm = aig->ndname(al.lid());
	return (al.sgn() ? "~":"") + nm;
}

void AigSweep::print_equiv_classes() {
	std::cout << "\n\nclasses\n";
	int i = 0;
	for (size_t cl = 0; cl < classes.size(); cl++) {
		if (classes[cl].size() > 1) {
			std::cout << "class: " << cl << ": ";
			for (auto nd : classes[cl]) {
				std::cout << aigsweep_nName(nd) << " ";
			}
			std::cout << "\n";
		}
	}
}


} // namespace opt_ns
