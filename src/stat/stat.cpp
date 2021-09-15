/*
 * stat.cpp
 *
 *  Created on: Apr 23, 2018
 *      Author: kaveh
 */

#include <stat/stat.h>
#include "dec/mcdec.h"
#include <random>

namespace stat_ns {

void get_output_prob(const circuit& ckt, id num_patterns) {

	oidset inputs = ckt.inputs();
	add_all(inputs, ckt.keys());

	id2realmap probMap;
	for (auto wid : inputs) {
		probMap[wid] = 0.5;
	}

	if (num_patterns == -1)
		sig_prob_eqn(ckt, probMap);
	else
		sig_prob_sim(ckt, num_patterns, probMap);

	std::cout << "\noutput probabilities\n";
	for (auto oid : ckt.outputs()) {
		std::cout << ckt.wname(oid) << ": " << probMap.at(oid) << "\n";
	}

}

void _get_prob(const circuit& ckt, const id2boolmap& fixedInputs, id2realmap& encprobMap, id num_patterns) {

	for (auto fp : fixedInputs) {
		encprobMap[fp.first] = (double)(uint)fp.second;
	}

	if (num_patterns != -1) {
		sig_prob_sim(ckt, num_patterns, encprobMap);
	}
	else {
		propagate_prob(ckt, encprobMap);
	}
}


void sig_prob_graph(const circuit& cir, uint num_patts, const std::string& filename) {

	// compute probabilities
	id2realmap prmap;
	if (cir.nLatches() == 0)
		sig_prob_sim(cir, num_patts, prmap);
	else
		sig_prob_seq(cir, num_patts, 30, prmap, cir.keys());
	//stat::sig_prob_eqn(cir, prmap);

	using namespace cktgph;
	cktgraph gph(cir);

	node_color_writer ncw(gph);

	for (auto wid : cir.allins()) {
		double prob = 255;
		ncw.wire_labels[wid] = fmt("[fillcolor=\"# %1$02x%1$02x%1$02x \"][style=filled]"
						"[shape=circle][orientation=180][label=", ((int)prob))
						+ cir.wname(wid) + "]";
	}

	foreach_gate(cir, g, gid) {
		id wid = g.fo0();
		double prob = 255 * (1 - pow(fabs(0.5 - prmap.at(wid))*2, 5));
		std::cout << "gate prob is : " << prob << "  " << prmap.at(wid) << "\n";
		std::string gfun_str = funct::enum_to_string(cir.gfunction(gid));
		std::string shape = (!g.isLatch() ? "[shape=triangle]":"[shape=box]");

		ncw.gate_labels[gid] = fmt("[fillcolor=\"# %1$02x%1$02x%1$02x \"][style=filled]"
				"[shape=triangle][orientation=180][label=", ((int)prob))
				+ gfun_str + "]";

		/*ncw.gate_labels[gid] = fmt("[fillcolor=%s][style=filled]" +
						 shape + "[orientation=180][label=", (prob < 10 ? "red":"white"))
						+ gfun_str + "]";*/
	}

	std::ofstream dotfile(filename.c_str());
	write_graphviz(dotfile, gph.gp, ncw);

}

void sig_prob_eqn(const circuit& cir, id2realmap& probMap) {
	propagate_prob(cir, probMap);
}

void sig_prob_seq(const circuit& ckt, uint num_patterns, uint max_depth,
		id2realmap& probMap, const oidset& frozens) {

	auto start = _start_wall_timer();

	std::default_random_engine rand_engine;
	std::uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);

	uint n = 0;
	while (n < num_patterns) {
		uint depth = rand() % max_depth + 1;
		n += depth;

		std::vector<id2boolmap> inputMap(depth, id2boolmap());
		std::vector<id2boolmap> simMap(depth, id2boolmap());

		for (uint d = 0; d < depth; d++) {
			for (auto inid : ckt.allins()) {
				if (d == 0 || _is_not_in(inid, frozens))
					inputMap[d][inid] = _is_in(inid, probMap) ?
							(uniform_zero_to_one(rand_engine) >= probMap.at(inid)) : (rand() % 2 == 0);
				else
					inputMap[d][inid] = inputMap[0][inid];
			}
		}

		ckt.simulate_seq(inputMap, simMap);

		for (uint d = 0; d < depth; d++) {
			foreach_wire(ckt, w, wid) {
				if (w.isInput() || w.isKey())
					continue;
				probMap[wid] += (simMap[d].at(wid) == 1);
			}
		}
	}

	std::cout << "simulation time: " << _stop_wall_timer(start) << "\n";

	foreach_wire(ckt, w, wid) {
		probMap[wid] /= n;
	}

}

void flip_impact_sim(const circuit& cir, uint num_patterns, id2realmap& probMap) {

	foreach_wire(cir, w, wid) {
		if (!w.isOutput()) {
			probMap[wid] = flip_impact_sim(wid, cir, num_patterns);
			std::cout << cir.wname(wid) << " -> " << probMap.at(wid) << "\n";
		}
	}

}

double flip_impact_sim(id root, const circuit& cir, uint num_patterns) {

	id2boolmap simMap;

	circuit cir_i = cir;

	if (cir_i.isInter(root)) {
		auto p = cir_i.open_wire(root);
		cir_i.remove_dead_logic();
		cir_i.setwiretype(p.second, wtype::IN);
		root = p.second;
	}

	real hd_i = 0;
	for (uint np = 0; np < num_patterns; np++) {
		for (auto wid : cir_i.allins()) {
			simMap[wid] = rand() % 2;
		}

		simMap[root] = 0;
		cir_i.simulate_comb(simMap);

		boolvec outs;
		for (auto oid : cir.outputs()) {
			outs.push_back(simMap.at(oid));
		}

		simMap[root] = 1;
		cir_i.simulate_comb(simMap);

		uint i = 0;
		for (auto oid : cir_i.outputs()) {
			if (simMap.at(oid) != outs[i++])
				hd_i++;
		}
	}

	real ret = hd_i / (num_patterns * cir.nOutputs());
	return ret;

}

void sig_prob_sim(const circuit& ckt, uint num_patterns, id2realmap& probMap) {

	id2boolmap smap;

	auto start = _start_wall_timer();

	std::default_random_engine rand_engine;
	std::uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);

	foreach_wire(ckt, w, wid) {
		if (!w.isInput() && !w.isKey())
			probMap[wid] = 0;
	}

	for (uint n = 0; n < num_patterns; n++) {
		for (auto inid : ckt.allins()) {
			if (_is_not_in(inid, probMap))
				smap[inid] = (rand() % 2 == 0);
			else {
				bool sv = (uniform_zero_to_one(rand_engine) < probMap.at(inid));
				// std::cout << probMap.at(inid) << " -> " << sv << "\n";
				smap[inid] = (sv >= probMap.at(inid));
			}
		}

		ckt.simulate_comb(smap);

		foreach_wire(ckt, w, wid) {
			if (w.isKey() || w.isInput())
				continue;
			probMap[wid] += (smap.at(wid) == 1);
		}
	}

	std::cout << "simulation time: " << _stop_wall_timer(start) << "\n";

	foreach_wire(ckt, w, wid) {
		probMap[wid] /= num_patterns;
	}

}


} // namespace st


