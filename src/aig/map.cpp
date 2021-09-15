/*
 * map.cpp
 *
 *  Created on: Jun 26, 2020
 *      Author: kaveh
 */

#include "aig/map.h"
#include "base/circuit_util.h"

namespace aig_ns {

using namespace ckt;
using namespace aig_ns;

// compute P and N class of each cell and place in maps
void tech_map_t::compute_lib_funcs() {
	id cell_fun_index = (_clib->getccell(0)).attch.get_attachment_id("funcir");

	// get max_num_ins
	for (const auto& x : _clib->cells) {
		const circuit& funcir = _clib->get_cell_attr<circuit>(x.first, cell_fun_index);
		uint num_cell_ins = funcir.nInputs();
		max_num_ins = MAX(num_cell_ins, max_num_ins);
	}

	if (max_num_ins > 6) {
		neos_error("cannot efficiently map to cells with more than 6 inputs");
	}
	neos_println("max number of inputs in cell library: " << max_num_ins);

	build_perms_and_negs();
	build_invar_tts();

	cMaps.clear();
	cMaps.resize(max_num_ins);

	for (const auto& x : _clib->cells) {
		std::cout << "for cell " << x.second.cell_name << "\n";
		id cellid = x.first;
		const circuit& funcir = (x.second.attch.get_cattatchment<circuit>(cell_fun_index));
		float cell_area = _clib->get_cell_attr<float>(cellid, "area");

		uint num_cell_ins = funcir.nInputs();

		id funtip = *(funcir.outputs().begin());

		auto& cm = cMaps[num_cell_ins - 1];
		auto invec = utl::to_vec(funcir.inputs());

		auto& npn = npn_enums.at(num_cell_ins - 1);

		for (uint px = 0; px < npn.NP; px++) {
			const auto& perm = npn.varperms[px];
			idmap<dbitset> simmap;
			for (uint i = 0; i < num_cell_ins; i++) {
				simmap[invec[i]] = invar_tts[num_cell_ins - 1][perm[i]];
			}
			ckt_util::simulate_comb_bitset(funcir, simmap);
			dbitset tt = simmap.at(funtip);
			if (num_cell_ins == 1) {
				if (tt == dbitset(2, 2)) {
					std::cout << "single-ended cell " << x.second.cell_name << " has bufer functionality\n";
					bufs.emplace(cell_area, cellid);
				}
				else if (tt == dbitset(2, 1)) {
					std::cout << "single-ended cell " << x.second.cell_name << " has inverter functionality\n";
					invs.emplace(cell_area, cellid);
				}
				else {
					neos_abort("cell library has single-input cell "
							<< x.second.cell_name << " that is neither bufer nor inverter\n");
				}
			}
			std::cout << " tip at " << utl::to_delstr(perm, ", ") << ": " << tt << "\n";
			cm[tt].impls[cellid] = {px};
		}
	}

	std::cout << "available functions\n";
	for (uint i = 0; i < cMaps.size(); i++) {
		std::cout << "c at " << i << "\n";
		for (const auto& p : cMaps[i]) {
			std::cout << "fun: " << p.first << " : ";
			for (const auto& c : p.second.impls) {
				std::cout << _clib->cells.at(c.first).cell_name << ", ";
			}
			std::cout << "\n";
		}
	}

}

void tech_map_t::build_perms_and_negs() {

	// build all permutations
	for (int x = 0; x < max_num_ins; x++) {
		npn_enums.push_back(dyn_npn_enumerator(x + 1, 1));
	}
}

void tech_map_t::build_invar_tts() {

	invar_tts = vector<vector<dbitset>>(max_num_ins);
	for (uint ini = 0; ini < max_num_ins; ini++) {
		uint num_in = ini + 1;
		uint16_t num_tts = std::pow(2, num_in);
		invar_tts[ini] = vector<dbitset>(num_in, dbitset(num_tts));
		for (uint inj = 0; inj < num_in; inj++) {
			for (uint16_t j = 0; j < num_tts; j++) {
				invar_tts[ini][inj][j] = (j >> inj) & 1;
			}
		}
	}

	uint n = 1;
	for (auto& invar_tt : invar_tts) {
		std::cout << "\n\nfor " << n++ << " inputs:\n";
		uint ini = 0;
		for (auto& tt : invar_tt) {
			std::cout << "invar " << ini << " " << tt << "\n";
			ini++;
		}
	}

}


void tech_map_t::init_map() {
	for (auto nid : ntk->top_andorder()) {
		auto& md = mMap[nid];
		md.cellid = -1;
		md.nid = nid;
		md.is_root = true;
	}
}

void tech_map_t::simulate_cut(const cut_t& cut, idmap<dbitset>& simmap) {

	id nid = cut.root;
	idvec cut_order;
	topsort_cut(*ntk, cut, cut_order);
	uint num_ins = cut.inputs.size();

	// simulate_cut
	idvec invec = utl::to_vec(cut.inputs);
	for (uint i = 0; i < cut.inputs.size(); i++) {
		simmap[invec[i]] = invar_tts[num_ins-1][i];
	}
	for (auto nid : cut_order) {
		const auto& nd = ntk->getcNode(nid);
		if (nd.isAnd()) {
			simmap[nid] =  ( nd.cm0() ? ~simmap.at(nd.fi0()) : simmap.at(nd.fi0()) )
						&  ( nd.cm1() ? ~simmap.at(nd.fi1()) : simmap.at(nd.fi1()) );
		}
	}
	//std::cout << "cut root " << simmap.at(nid) << "\n";
}

void tech_map_t::match_cuts() {

	// iterate through rtt permutations

	for (auto nid : ntk->top_andorder()) {
		bool found_cell_atall = false;
		idmap<dbitset> simmap;
		std::cout << "at node " << ntk->ndname(nid) << "\n";
		int cut_ind = 0;
		for (int cut_ind = 0; cut_ind < cutMap.at(nid).size(); cut_ind++) {
			const auto& cut = cutMap.at(nid).at(cut_ind);
			uint num_ins = cut.inputs.size();
			if (num_ins == 1) continue; // skip trivial cuts
			assert(num_ins <= max_num_ins);
			std::cout << " for cut " << cut_ind << "\n";
			print_cut(*ntk, cut);
			std::cout << "num ins: " << num_ins << "\n";

			// get cut truth_table
			simulate_cut(cut, simmap);

			// maping part
			dbitset rtt = simmap.at(nid);

			using truth_t = dyn_npn_enumerator::truth_t;

			// iterate through rtt permutations
			bool found_cell = false;

			auto& npn = npn_enums.at(num_ins - 1);
			for (truth_t xtt = npn.init(rtt); npn.next_neg(xtt); ) {
				const auto& cmap = cMaps.at(num_ins-1);
				std::cout << "neg: " << xtt << "\n";
				auto it = cmap.find(xtt);
				if (it != cmap.end()) {
					std::cout << "found table \n";
					std::cout << "   matched cells ";
					for (const auto& x : it->second.impls) {
						 std::cout << _clib->cells.at(x.first).cell_name << ", ";
						mMap[nid].matches.push_back({x.first, x.second.perm_index, npn.lix.nInd, cut_ind});
					}
					std::cout << "\n";
					found_cell = true;
					found_cell_atall = true;
					break;
				}
			}

			if (!found_cell) {
				//std::cout << "   no match found\n";
			}
		}

		if (!found_cell_atall) {
			std::cout << "  no match for node\n";
			assert(false);
		}
	}

}

void tech_map_t::find_cover() {

	idvec torder = ntk->top_andorder();
	std::reverse(torder.begin(), torder.end());
	int cell_area_index = 1;

	auto cost_fun_area = [&](const cell_match_t& cm) {
		return - _clib->get_cell_attr<float>(cm.cellid, cell_area_index);
	};

	auto cost_fun = cost_fun_area;

	for (id i = 0; i < torder.size(); i++) {
		id nid = torder[i];
		auto& mM = mMap.at(nid);

		if (!mM.is_root) {
			std::cout << "skipping " << nid << "\n";
			continue;
		}

		// find best match
		float best_cost = std::numeric_limits<float>::max();
		const auto& matches = mM.matches;
		int best_match = -1;
		for (int m = 0; m < matches.size(); m++) {
			float cost = cost_fun(matches[m]);
			if (cost < best_cost) {
				best_cost = cost;
				best_match = m;
			}
		}

		// map to best match
		const auto& mt = matches[best_match];
		mM.cellid = mt.cellid;
		mM.is_root = true;
		mM.cost = best_cost;
		mM.best_match = best_match;

		total_cost += best_cost;

		const auto& best_cut = cutMap.at(nid).at(mt.cutindex);
		// print_cut(*ntk, best_cut);
		assert(best_cut.root == nid);
		for (auto xid : best_cut.ands) {
			if (xid != nid) {
				std::cout << "setting is_root to false\n";
				mMap.at(xid).is_root = false;
			}
		}
	}
}

void tech_map_t::print_map() {

	for (auto xid : ntk->inputs())
		std::cout << ntk->to_str(xid) << "\n";

	for (auto yid : ntk->outputs())
		std::cout << ntk->to_str(yid) << "\n";

	for (auto nid : ntk->top_andorder()) {
		std::cout << ntk->to_str(nid);
		const auto& mM = mMap.at(nid);
		if (mM.is_root) {
			std::cout << " -> " << _clib->getccell(mM.cellid).cell_name;
			const auto& best_mt = mM.matches.at(mM.best_match);
			id cutind = best_mt.cutindex;
			const auto& cut = cutMap.at(nid).at(cutind);
			uint num_ins = cut.inputs.size();
			std::cout << " (";
			for (auto xid :  cut.inputs) {
				std::cout << ntk->ndname(xid) << ", ";
			}
			std::cout << ") p:";
			std::cout << utl::to_delstr(npn_enums.at(num_ins-1).varperms.at(best_mt.perm_index), ", ");
			std::cout << "  n:" << dbitset(num_ins, best_mt.neg_index) << "\n";
		}
		else {
			std::cout << "\n";
		}
	}

	std::cout << "total cost: " << total_cost << "\n";
}

void tech_map_t::map_to_lib(int num_iterations) {
	compute_lib_funcs();
	init_map();
	cut_enumerate_local(*ntk, cutMap, max_num_ins, 250, true);
	std::cout << "done with cut enumerating\n";
	match_cuts();
	find_cover();
	print_map();
}

void tech_map_t::to_cir(circuit& cir) {
	std::unordered_map<alit, id, alit_Hash> aig2cir;

	cir.set_cell_library(*_clib);

	for (auto xid : ntk->inputs())
		aig2cir[xid] = cir.add_wire(ntk->ndname(xid), wtype::IN);

	for (auto yid : ntk->outputs())
		aig2cir[yid] = cir.add_wire(ntk->ndname(yid), wtype::OUT);

	for (auto nid : ntk->top_andorder()) {
		//std::cout << ntk->to_str(nid);
		const auto& mM = mMap.at(nid);
		if (mM.is_root) {
			const auto& cl_obj = _clib->getccell(mM.cellid);
			const auto& cell_nm = _clib->getccell(mM.cellid).cell_name;
			const auto& best_mt = mM.matches.at(mM.best_match);
			id cutind = best_mt.cutindex;
			const auto& cut = cutMap.at(nid).at(cutind);
			uint num_ins = cut.inputs.size();
			idvec mp_ins(cut.inputs.size());
			dbitset in_mask(num_ins, best_mt.neg_index);
			const auto& in_perm = npn_enums.at(num_ins-1).varperms[best_mt.perm_index];
			uint i = 0;
			id xout = aig2cir[nid] = cir.add_wire(ntk->ndname(nid), wtype::INTER);

			// deal with inputs and negate
			idvec fanins;
			for (auto xid :  cut.inputs) {
				if (_is_not_in(xid, aig2cir)) {
					std::cout << "missing node " << ntk->ndname(xid) << "\n";
				}
				alit xal = aig2cir.at(xid);
				if (in_mask[i]) {
					id xbid;
					auto it = aig2cir.find(~xal);
					if ( it == aig2cir.end() ) {
						xbid = cir.add_wire(ntk->ndname(xid) + "_b", wtype::INTER);
						aig2cir[~xal] = xbid;
						cir.add_gate({xal.lid()}, xbid, fnct::NOT);
					}
					else {
						xbid = aig2cir.at(~xal);
					}
					id invid = (invs.begin()->second);
					std::cout << "using inverter " << _clib->getccell(invid).cell_name << "\n";
					cir.add_instance(cir.get_new_iname(), invid, {xid, xbid});
					fanins.push_back(xbid);
				}
				else {
					fanins.push_back(xid);
				}
				i++;
			}

			// deal with output
			fanins.push_back(xout);
			//std::cout << "cell id is " << _clib->getccell(mM.cellid) << "\n";
			std::cout << "adding instance " << _clib->getccell(mM.cellid).cell_name << "\n";
			cir.add_instance(cir.get_new_iname(), mM.cellid, fanins);
		}
	}

}

} // namespace aig_ns
