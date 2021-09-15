/*
 * enc_seq.cpp
 *
 *  Created on: Apr 11, 2018
 *      Author: kaveh
 */


#include "enc/enc.h"

namespace enc {

void encrypt::enc_seq(circuit& ckt) {


	for (auto oid : ckt.outputs()) {
		idvec faninGates;
		idset faninWires;
		idset faninPis;
		uint numDffs = 0;
		ckt.get_trans_fanin(oid, faninWires, faninPis, faninGates);
		for (auto gid : faninGates) {
			if (ckt.isLatch(gid))
				numDffs++;
		}

		int w = 5;
		std::cout << "out: " << std::setw(w) << ckt.wname(oid) << "  #gates: " << std::setw(w) << faninGates.size()
				<< "  #inputs: " << std::setw(w) << faninPis.size() << std::setw(w) << "  #dffs: " << std::setw(w) << numDffs << "\n";
	}

}

void encrypt::get_universal_simckt(circuit& sim_cir, univ_opt& opt) {

	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	srand(seed);

	circuit enc_cir;
	get_universal_circuit(enc_cir, opt);

	auto keyids = enc_cir.keys();
	for (auto kid : keyids) {
		enc_cir.tie_to_constant(kid, rand() % 2 == 1);
	}


	sim_cir = enc_cir;

/*
	int sim_ckt_size = 0, trials = 0;
	circuit cand_sim_ckt;
	while (sim_ckt_size < (float)enc_ckt.nInputs() * 0.7 && trials++ < 50) {
		cand_sim_ckt = sim_ckt;
		boolvec key = utl::_random_boolvec(enc_ckt.nKeys());
		// st::stat::get_active_key(enc_ckt, key);
		cand_sim_ckt.simplify_wkey(key);
		ext::resynthesize(cand_sim_ckt, 2);
		sim_ckt_size = cand_sim_ckt.nGates();
		std::cout << "sim_ckt size: " << sim_ckt_size << "\n";
	}
*/

}

void encrypt::get_universal_circuit(circuit& ucir, univ_opt& opt) {

	std::vector<int> widths;
	if (opt.profile == univ_opt::SQUARE) {
		std::cout << "profile: square\n";
		for (int i = 0; i < opt.depth - 1; i++) {
			widths.push_back(opt.width);
		}
	}
	else if (opt.profile == univ_opt::CONE) {
		int sign = (opt.width > opt.num_outs) ? 1:-1;
		std::cout << "profile: cone\n";
		int dw = abs(opt.width - opt.num_outs) / (opt.depth - 1); // watch out
		for (int i = 0; i < opt.depth - 1; i++) {
			widths.push_back(opt.width - sign * i * dw);
		}
	}

	// width of last layer is output-width
	widths.push_back(opt.num_outs);

	get_universal_circuit(ucir, opt.gate_type, opt.num_ins, opt.num_outs,
			widths, opt.depth, opt.indeg, opt.backstep);

}

/* backstep : 1 -> immediate layer
 *  0 -> all previous layers
 */


void encrypt::get_univ_muxfanins(idvec& muxfanins, const std::vector<idvec>& layer_ins,
		float indeg, int backstep, int l, int w) {

	int goback = (backstep == 0) ? 0 : l - backstep + 1;
	for (int i = l; i >= MAX(goback, 0); i--) {
		 push_all(muxfanins, layer_ins[i]);
	}

	// pick muxfanins from candidates
	if (indeg != 1) {
		idvec v1;
		uint cyc = w;
		for (int i = 0; i < MAX(2, indeg * muxfanins.size()); i++) {
			v1.push_back(muxfanins[cyc++ % muxfanins.size()]);
		}
		//muxfanins = utl::_rand_from_vec(muxfanins, indeg);
		muxfanins = v1;
	}

}

void encrypt::get_universal_circuit(circuit& ucir, int gate_type, int num_ins, int num_outs,
		std::vector<int>& widths, int depth, float indeg, int backstep) {

	std::cout << "num_ins:" << num_ins << "\n";
	std::cout << "num_outs:" << num_outs << "\n";
	std::cout << "width profile: {";
	for (uint i = 0; i < widths.size(); i++) {
		std::cout << widths[i] << "  " ;
	}
	std::cout << "\n";
	std::cout << "depth:" << depth << "\n";
	std::cout << "indeg:" << indeg << "\n";
	std::cout << "backstep:" << backstep << "\n";

	using namespace ckt_block;

	assert(indeg > 0 && indeg <= 1);

	std::vector<idvec> layer_ins(depth, idvec());

	for (int i = 0; i < num_ins; i++) {
		layer_ins[0].push_back(ucir.add_wire(fmt("in%d", i), wtype::IN));
	}

	for (int l = 0; l < depth; l++) {

		for (int w = 0; w < widths[l]; w++) {

			// add mux outputs
			id muxa = ucir.add_wire(fmt("moA_%d_%d", w % l), wtype::INTER);
			id muxb = ucir.add_wire(fmt("moB_%d_%d", w % l), wtype::INTER);

			for (int muxout : {0, 1}) {
				// pick muxfanin candidates
				idvec muxfanins;
				get_univ_muxfanins(muxfanins, layer_ins, indeg, backstep, l, w);

				// now insert muxes
				mux_t mx = ckt_block::key_nmux_w2mux(muxfanins.size());

				id2idmap added2new;
				// std::cout << "\nmux " << w << " " << l << " with ins:";
				for (uint i = 0; i < mx.in_vec.size(); i++) {
					added2new[mx.in_vec[i]] = muxfanins[i];
					//std::cout << ucir.wname(muxfanins[i]) << " ";
				}
				for (auto km : mx.sel_vec) {
					added2new[km] = ucir.add_key();
				}
				added2new[mx.out] = muxout == 0 ? muxa:muxb;
				ucir.add_circuit(mx, added2new, fmt(muxout == 0 ? "A_%d_%d":"B_%d_%d" , w % l));
			}

			// add gate
			id aout = ucir.add_wire(fmt("lw_%d_%d", w % l), wtype::INTER);;
			if (gate_type == 0) {
				mux_t lut = ckt_block::key_nmux_w2mux(4, false);

				id2idmap added2new;
				added2new[lut.sel_vec[0]] = muxa;
				added2new[lut.sel_vec[1]] = muxb;
				for (uint i = 0; i < lut.in_vec.size(); i++) {
					added2new[lut.in_vec[i]] = ucir.add_key();
				}
				added2new[lut.out] = aout;


				// add AND output
				ucir.add_circuit(lut, added2new, fmt("L_%d_%d" , w % l));
			}
			else if (gate_type == 1) {
				// add inverters
				id inva = ucir.add_wire(fmt("moAinv_%d_%d", w % l), wtype::INTER);
				id invb = ucir.add_wire(fmt("moBinv_%d_%d", w % l), wtype::INTER);

				id kid = ucir.add_key();
				ucir.add_gate(fmt("moAx_%d_%d", w % l), {muxa, kid}, inva, fnct::XOR);
				kid = ucir.add_key();
				ucir.add_gate(fmt("moBx_%d_%d", w % l), {muxb, kid}, invb, fnct::XOR);

				// add AND output
				ucir.add_gate(fmt("an%d_%d", w % l), {inva, invb}, aout, fnct::AND);
			}



			if (l == depth - 1) { // last layer
				id out = ucir.add_wire(fmt("out_%d", w), wtype::OUT);
				if (gate_type == 1) {
					id xkey = ucir.add_key();
					ucir.add_gate(fmt("bo%d", w), {xkey, aout}, out, fnct::XOR);
				}
				else {
					ucir.add_gate(fmt("bo%d", w), {aout}, out, fnct::BUF);
				}
			}
			else {
				layer_ins[l + 1].push_back(aout);
			}
		}
	}

}

bool encrypt::enc_universal(circuit& enc_ckt, univ_opt& opt) {

	encrypt::layer_stats(enc_ckt);

	circuit cir;
	get_universal_circuit(cir, opt);

	auto it = cir.inputs().begin();
	foreach_inid(enc_ckt, inid) {
		cir.setwirename(*it, enc_ckt.wname(inid));
		it++;
	}

	it = cir.outputs().begin();
	foreach_outid(enc_ckt, oid) {
		cir.setwirename(*it, enc_ckt.wname(oid));
		it++;
	}

	enc_ckt = cir;

	return true;

}


void encrypt::layer_stats(circuit& ckt) {

	std::vector<idset> layer_wsets(1, idset());
	std::vector<idset> layer_gsets(1, idset());
	idset wvisited, gvisited;

	foreach_inid(ckt, inid) {
		layer_wsets[0].insert(inid);
		wvisited.insert(inid);
	}

	uint i = 0;

	while (true) {
		bool found = false;
		for (auto wid : layer_wsets[i]) {
			for (auto wid2 : ckt.wfanoutWires(wid)) {
				if ( _is_not_in(wid2, wvisited) && _is_not_in(wid2, layer_wsets[i]) ) {
					found = true;
					if (layer_wsets.size() <= i+1)
						layer_wsets.push_back(idset());
					layer_wsets[i+1].insert(wid2);
					wvisited.insert(wid2);
				}
			}
			for (auto gid : ckt.wfanout(wid)) {
				if ( _is_not_in(gid, gvisited) && _is_not_in(gid, layer_gsets[i]) ) {
					found = true;
					if (layer_gsets.size() <= i+1)
						layer_gsets.push_back(idset());
					layer_gsets[i+1].insert(gid);
					gvisited.insert(gid);
				}
			}
		}

		if (!found)
			break;
		else
			i++;
	}

	std::cout << "\n";
	for (uint i = 0; i < layer_wsets.size(); i++) {
		std::cout << "layer " << i << " : ";
		uint layer_inputs = 0, layer_keys = 0, layer_inters = 0;
		for (auto wid : layer_wsets[i]) {
			if (ckt.isInput(wid))
				layer_inputs++;
			else if (ckt.isKey(wid))
				layer_keys++;
			else if (ckt.isInter(wid))
				layer_inters++;
			// std::cout << ckt.wname(wid) << " ";
		}
		std::cout << "-> #inputs: " << layer_inputs << "  #keys: " << layer_keys << "  #inters: "
				<< layer_inters << " #gates: " << (float)layer_gsets[i].size() / (float)ckt.nGates() << "\n";
	}
}

/*
 * random mux plus xor obfuscation
 */
void encrypt::enc_xmux(circuit& ckt, uint num_muxes, uint num_xors) {

	enc_xor_rand(ckt, boolvec(num_xors, false));
	enc_wire::enc_wire_options opt;
	opt.num_muxs = num_muxes;
	opt.mux_size = 2;
	enc_manymux(ckt, opt);
}


void encrypt::enc_metric_adhoc(circuit& ckt, uint steps, uint target_mux, uint target_xor) {


	uint STEP_LIMIT = 1000;
	uint step = 0;

	circuit best_ckt;
	double deg = 0;
	enc_wire::enc_wire_options opt;
	opt.num_muxs = target_mux;
	opt.mux_size = 2;
	opt.selection_method = "rnd";

	while (true) {

		circuit cand_ckt = ckt;
		enc_manymux(cand_ckt, opt);
		enc_xor_rand(cand_ckt, boolvec(target_xor, 0));
		double cand_deg = propagate_metric_adhoc(cand_ckt, true);
		if (cand_deg > deg) {
			deg = cand_deg;
			best_ckt = cand_ckt;
			std::cout << "best deg : " << deg << "\n";
			if (step++ >= steps) {
				std::cout << "reached goal steps\n";
				break;
			}
		}

		if ( step > STEP_LIMIT) {
			std::cout << "reached limit\n";
			break;
		}
	}

	propagate_metric_adhoc(best_ckt);
	ckt = circuit();
	ckt = best_ckt;
}

double encrypt::structural_tests(const circuit& cir, bool silent) {

	const idvec& gate_order = cir.get_topsort_gates();

	foreach_inid(cir, inid) {

		std::cout << "\n\n" << cir.wname(inid) << "\n";

		std::map<id, id> num_visited_keys;

		idset wtran_fanout;
		cir.get_trans_fanoutset(inid, wtran_fanout);

		for (auto gid : gate_order) {

			if (_is_in(cir.gfanout(gid), wtran_fanout)) {
				id num_keys = 0, min_key = -1;
				for (auto gfanin : cir.gfanin(gid)) {
					if ( _is_not_in(gfanin, wtran_fanout) ) {
						num_visited_keys[gfanin] = 0;

						if (cir.isKey(gfanin)) {
							num_keys++;
						}
						else if (cir.isInput(gfanin)) {
							num_visited_keys[gfanin] = 0;
						}
					}
					else {
						if (min_key == -1)
							min_key = num_visited_keys.at(gfanin);
						else
							min_key = MIN(min_key, num_visited_keys.at(gfanin));
					}
				}
				num_visited_keys[cir.gfanout(gid)] = min_key + num_keys;
			}
		}

		id oindex = 1;
		foreach_outid(cir, oid) {
			std::cout << std::left << std::setw(6) << cir.wname(oid) << ":" << std::setw(4) << std::left << num_visited_keys[oid] << " ";
			if (oindex++ % 7 == 0)
				std::cout << "\n";
		}

	}
	std::cout << "\n";

	return 0;
}

double encrypt::propagate_metric_adhoc(const circuit& ckt, bool silent) {

	const idvec& gate_order = ckt.get_topsort_gates();

	std::map<id, std::map<id, double> > in_degs;
	std::map<id, double> min_in_degs;

	oidset inputs = ckt.allins();

	for (auto xid : inputs) {
		in_degs[xid][xid] = 1;
		min_in_degs[xid] = std::numeric_limits<id>::max();
	}

	// assert(gate_order.size() == ckt.nGates());
	double XOR_FACTOR = 1;
	double NXOR_FACTOR = 0.1;
	//double COMB_FACTOR = 1.1;

	for (auto gid : gate_order) {
		id fanout = ckt.gfanout(gid);
		for (auto fanin : ckt.gfanin(gid)) {
			for (auto kdp : in_degs[fanin]) {
				if (ckt.isKey(fanin)) {
					if (ckt.gfunction(gid) == fnct::XOR || ckt.gfunction(gid) == fnct::XNOR)
						in_degs[fanout][kdp.first] += XOR_FACTOR * kdp.second;
					else
						in_degs[fanout][kdp.first] += NXOR_FACTOR * kdp.second;
				}
				else {
					in_degs[fanout][kdp.first] = in_degs[fanout][kdp.first] + ((kdp.second != 0) ? 1:0);
					// in_degs[fanout][kdp.first] = MAX(in_degs[fanout][kdp.first], COMB_FACTOR * kdp.second);
				}
				// if (!silent) std::cout << kdp.second << " ";
			}
			// if (!silent) std::cout << "\n";
		}
	}

	double overall_key = 0, overall_in = 0;
	double min_in_deg = std::numeric_limits<double>::max();
	double min_key_deg = std::numeric_limits<double>::max();

	// foreach output degree
	foreach_outid(ckt, oid) {
		if (!silent) std::cout << ckt.wname(oid) << ": \n";
		uint kIndex = 0, xIndex = 0;
		double avg_in_deg = 0, avg_key_deg = 0;

		// record inputs/keys degrees
		uint lb = 1;
		for (auto wid : inputs) {
			double in_deg = in_degs[oid][wid];
			// will later be used to print overal minimum
			min_in_degs[wid] = MIN(min_in_degs[wid], in_deg);
			if (ckt.isKey(wid))
				avg_key_deg += in_deg;
			else
				avg_in_deg += in_deg;
			if (!silent) {
				if (ckt.isKey(wid))
					std::cout << std::setprecision(2) << "k" << std::setw(3) << kIndex++ << ":" << std::setw(8) << std::left << in_deg << " ";
				else
					std::cout << std::setprecision(10) << "x" << std::setw(3) << xIndex++ << ":" << std::setw(8) << std::left << in_deg << " ";
				if (lb++ % 7 == 0)
					std::cout << "\n";
			}
		}
		// avg_in_deg /= ckt.nInputs();
		// avg_key_deg /= ckt.nKeys();
		min_in_deg = MIN(avg_in_deg, min_in_deg);
		min_key_deg = MIN(avg_key_deg, min_key_deg);
		if (!silent) std::cout << std::setprecision(10) << "\n avg-deg: {x:" << avg_in_deg << " k:" << avg_key_deg << "}";
		overall_in += avg_in_deg;
		overall_key += avg_key_deg;
		if (!silent) std::cout << "\n\n";
	}

	overall_key /= ckt.nOutputs();
	overall_in /= ckt.nOutputs();

	if (!silent) std::cout << std::setprecision(10) << "overall: {x:" << overall_in << " k:" << overall_key << "}\n";
	if (!silent) std::cout << std::setprecision(10) << "minimum: {x:" << min_in_deg << " k:" << min_key_deg << "}\n";

	if (!silent) {
		std::cout << "\n\nmin degrees:\n";
		uint kIndex = 0;
		foreach_keyid(ckt, kid) {
			std::cout << "k" << kIndex++ << ":" << min_in_degs[kid] << "  ";
		}
		std::cout << "\n";
	}

	return min_in_deg;
}

} // namespace enc
