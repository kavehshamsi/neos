/*
 * stat_util.cpp
 *
 *  Created on: Apr 29, 2021
 *      Author: kaveh
 */

#include "stat/stat.h"

namespace stat_ns {

using namespace utl;

void sig_controllability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map) {
	// perform topological sort
	const idvec& gate_ordering = cir.get_topsort_gates();

	for (auto xid : cir.allins()) {
		if (_is_not_in(xid, cc0_map)) {
			cc0_map[xid] = 1;
			cc1_map[xid] = 1;
		}
	}

	//signal probability propagation
	for (auto& x : gate_ordering){
		sig_controllability(cir.getcgate(x), cc0_map, cc1_map, co_map);
	}
}

void sig_observability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map) {

	// perform topological sort
	const idvec& gate_ordering = cir.get_topsort_gates();

	for (auto xid : cir.allins()) {
		if (_is_not_in(xid, cc0_map)) {
			cc0_map[xid] = 1;
			cc1_map[xid] = 1;
		}
	}

	for (auto yid : cir.outputs()) {
		if (_is_not_in(yid, co_map)) {
			co_map[yid] = 0;
		}
	}

	//observability propagation
	for (auto rit = gate_ordering.rbegin();
			rit != gate_ordering.rend(); ++rit) {
		sig_observability(cir.getcgate(*rit), cc0_map, cc1_map, co_map);
	}

}


void sig_observability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map){

	id z = gt.fo0();

	real sum_other_ccs = 0;
	switch (gt.gfun()){
	case fnct::AND:
	case fnct::NAND: {
		for (auto& x : gt.fanins){
			for (auto& y : gt.fanins) {
				if (x != y) {
					sum_other_ccs += cc1_map[y];
				}
			}
			if (co_map.find(x) != co_map.end())
				co_map[x] = MIN(co_map[x], co_map[z] + sum_other_ccs + 1);
			else
				co_map[x] = co_map[z] + sum_other_ccs + 1;
		}
		break;
	}
	case fnct::OR:
	case fnct::NOR: {
		for (auto& x : gt.fanins){
			real sum_other_ccs = 0;
			for (auto& y : gt.fanins) {
				if (x != y) {
					sum_other_ccs += cc0_map[y];
				}
			}
			if (co_map.find(x) != co_map.end())
				co_map[x] = MIN(co_map[x], co_map[z] + sum_other_ccs + 1);
			else
				co_map[x] = co_map[z] + sum_other_ccs + 1;
		}
		break;
	}
	case fnct::XOR:
	case fnct::XNOR: {
		id a = gt.fanins[0];
		id b = gt.fanins[1];

		if (co_map.find(a) == co_map.end())
			co_map[a] = co_map[z] + MIN(cc0_map[b], cc1_map[b]) + 1;
		else
			co_map[a] = MIN(co_map.at(a), co_map[z] + MIN(cc0_map[b], cc1_map[b]) + 1);

		if (co_map.find(b) == co_map.end())
			co_map[b] = co_map[z] + MIN(cc0_map[a], cc1_map[a]) + 1;
		else
			co_map[b] = MIN(co_map.at(b), co_map[z] + MIN(cc0_map[a], cc1_map[a]) + 1);

		break;
	}
	case fnct::BUF: {
		id in = gt.fanins[0];
		if (co_map.find(in) == co_map.end())
			co_map[gt.fanins[0]] = co_map[z] + 1;
		else
			co_map[gt.fanins[0]] = MIN(co_map[in], co_map[z] + 1);

		break;
	}
	case fnct::NOT: {

		id in = gt.fanins[0];
		if (co_map.find(in) == co_map.end())
			co_map[gt.fanins[0]] = co_map[z] + 1;
		else
			co_map[gt.fanins[0]] = MIN(co_map[in], co_map[z] + 1);

		break;
	}
	default:
		std::cout << "cannot propagate observability for gate type "
		<< funct::enum_to_string(gt.gfun()) << "\n";
		exit(EXIT_FAILURE);
	}

	return;
}


void sig_controllability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map){

	real cc0_fanout;
	real cc1_fanout;

	const real REALMAX = std::numeric_limits<real>::max();

	switch (gt.gfun()){
	case fnct::AND: {
		cc0_fanout = REALMAX;
		cc1_fanout = 0;
		for (auto& x : gt.fanins){
			cc0_fanout = MIN(cc0_fanout, cc0_map[x]);
			cc1_fanout += cc1_map[x];
		}
		break;
	}
	case fnct::NAND: {
		cc0_fanout = 0;
		cc1_fanout = REALMAX;
		for (auto& x : gt.fanins){
			cc0_fanout += cc1_map[x];
			cc1_fanout = MIN(cc0_fanout, cc0_map[x]);
		}
		break;
	}
	case fnct::OR: {
		cc0_fanout = 0;
		cc1_fanout = REALMAX;
		for (auto& x : gt.fanins){
			cc0_fanout += cc0_map[x];
			cc1_fanout = MIN(cc1_fanout, cc1_map[x]);
		}
		break;
	}
	case fnct::NOR: {
		cc0_fanout = REALMAX;
		cc1_fanout = 0;
		for (auto& x : gt.fanins){
			cc0_fanout = MIN(cc1_fanout, cc1_map[x]);
			cc1_fanout += cc0_map[x];
		}
		break;
	}
	case fnct::XOR: {
		id a = gt.fanins[0];
		id b = gt.fanins[1];
		cc0_fanout = MIN(cc0_map[a] + cc0_map[b], cc1_map[a] + cc1_map[b]);
		cc1_fanout = MIN(cc1_map[a] + cc0_map[b], cc0_map[a] + cc1_map[b]);
		break;
	}
	case fnct::XNOR: {
		id a = gt.fanins[0];
		id b = gt.fanins[1];
		cc0_fanout = MIN(cc1_map[a] + cc0_map[b], cc0_map[a] + cc1_map[b]);
		cc1_fanout = MIN(cc0_map[a] + cc0_map[b], cc1_map[a] + cc1_map[b]);
		break;
	}
	case fnct::BUF: {
		id in = gt.fanins[0];
		cc0_fanout = cc0_map[in];
		cc1_fanout = cc1_map[in];
		break;
	}
	case fnct::NOT: {
		id in = gt.fanins[0];
		cc0_fanout = cc1_map[in];
		cc1_fanout = cc0_map[in];
		break;
	}
	default:
		std::cout << "cannot propagate controllability for gate type "
		<< funct::enum_to_string(gt.gfun()) << "\n";
		exit(EXIT_FAILURE);
	}

	id z = gt.fo0();

	cc0_map[z] = cc0_fanout + 1;
	cc1_map[z] = cc1_fanout + 1;

	return;
}

void propagate_prob(const circuit& cir, id2realmap& probmap) {

	// perform topological sort
	const idvec& gate_ordering = cir.get_topsort_gates();

	for (auto xid : cir.allins()) {
		if (_is_not_in(xid, probmap)) {
			probmap[xid] = 0.5;
		}
		else {
			assert(probmap.at(xid) <= 1 && probmap.at(xid) >= 0);
		}
	}

	//signal probability propagation
	for (auto gid : gate_ordering) {
		probmap[cir.gfanout(gid)] = propagate_prob(cir.getcgate(gid), probmap);
	}

}

void sig_prob_dupuis(const circuit& cir, uint num_patts, id2realmap& probmap) {

	// perform topological sort
	const idvec& gate_ordering = cir.get_topsort_gates();

	id2realmap simprobmap = probmap;
	sig_prob_sim(cir, num_patts, simprobmap);

	idmap<idset> support_sets;

	for (auto xid : cir.allins()) {
		if (_is_not_in(xid, probmap)) {
			probmap[xid] = 0.5;
		}
		else {
			assert(probmap.at(xid) <= 1 && probmap.at(xid) >= 0);
		}
		support_sets[xid].insert(xid);
	}

	uint accurate = cir.nAllins();
	//signal probability propagation
	for (auto& gid : gate_ordering) {
		const auto& g = cir.getcgate(gid);
		idset intersect = (g.fanins.size() == 1) ? idset() : support_sets.at(g.fanins[0]);
		idset union_set = support_sets.at(g.fanins[0]);
		for (int i = 1; i < g.fanins.size(); i++) {
			intersect = utl::set_intersect(intersect, support_sets.at(g.fanins[i]));
			union_set = utl::set_union(union_set, support_sets.at(g.fanins[i]));
		}
		if (intersect.empty()) {
			// std::cout << "accurate node " << cir.wname(g.fo0()) << "\n";
			real prb = propagate_prob(g, probmap);
			probmap[g.fo0()] = prb;
		}
		else {
			probmap[g.fo0()] = simprobmap.at(g.fo0());
		}
		support_sets[g.fo0()] = union_set;
		//std::cout << "wire : " << cir.wname(g.fo0()) << " fanins -> " << cir.to_wstr(g.fanins) << " -> {" << cir.to_wstr(intersect, ", ") << "}  union: " << cir.to_wstr(union_set) << "\n";
	}
	std::cout << "accurate : " << accurate << " / " << cir.nWires() << "\n";

}

real propagate_prob(const gate& gt, id2realmap& probMap) {

	real prbfanout = 1.0;
	switch (gt.gfun()){
	case fnct::AND:
	case fnct::NAND:
		for (auto& x : gt.fanins) {
			prbfanout *= probMap.at(x);
		}
		if (gt.gfun() == fnct::NAND)
			prbfanout = 1 - prbfanout;
		break;
	case fnct::OR:
	case fnct::NOR:
		for (auto& x : gt.fanins) {
			prbfanout *= (1-probMap.at(x));
			prbfanout = 1 - prbfanout;
		}
		if (gt.gfun() == fnct::NAND)
			prbfanout = 1 - prbfanout;
		break;
	case fnct::NOT:
		prbfanout = 1 - probMap.at(gt.fanins[0]);
		break;
	case fnct::XOR:
	case fnct::XNOR:
		prbfanout = (1 - probMap.at(gt.fanins[0])) * probMap.at(gt.fanins[1]);
		prbfanout += (1 - probMap.at(gt.fanins[1])) * probMap.at(gt.fanins[0]);
		if (gt.gfun() == fnct::XNOR)
			prbfanout = 1 - prbfanout;
		break;
	case fnct::BUF:
		prbfanout = probMap.at(gt.fanins[0]);
		break;
	default:
		break;
	}

	return prbfanout;;
}


}
