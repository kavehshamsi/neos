/*
 * dec_sat_ucyc.cpp
 *
 *  Created on: Jul 8, 2021
 *      Author: kaveh
 */

#include "dec/dec_sat.h"
#include "mc/unr.h"

namespace dec_ns {

void dec_sat_ucyc::_prepare_sat_attack() {

	using namespace utl;

	if (!oracle_binary.empty()) {
		std::cout << "unroll attack with binary oracle not supported\n";
		exit(1);
	}

	std::cout << "Running IcySAT-II (unrolling-based cyclic attack)\n";

	circuit enc_ckt = *enc_cir;

	auto outs = enc_ckt.outputs();
	for (auto oid : outs) {
		std::string nm = enc_ckt.wname(oid);
		enc_ckt.setwirename(oid, enc_ckt.get_new_wname());
		enc_ckt.setwiretype(oid, wtype::INTER);
		id newoid = enc_ckt.add_wire(nm, wtype::OUT);
		enc_ckt.add_gate({oid}, newoid, fnct::BUF);
	}

	auto feedback_arcs = enc_ckt.find_feedback_arc_set();

	std::cout << "feedback arc set size: " << feedback_arcs.size() << "\n";

	circuit msim_cir = *sim_cir;
	circuit gcir = enc_ckt;
	//uint i = 0;
	bimap<id, id> feedback_pairs;
	for (auto x : feedback_arcs) {
		id w = enc_ckt.gfanout(x.first);
		//std::cout << "feedback arc :" << enc_ckt.wname(w) << "\n";
		idpair p = gcir.open_wire(w);
		//gcir.setwirename(p.first, "fbo" + std::to_string(i++));
		assert(!gcir.isOutput(p.second));
		feedback_pairs.add_pair(p.first, p.second);
		//gcir.setwirename(p.second, "fbi" + std::to_string(i++));
		//gcir.add_gate({p.second}, p.first, fnct::DFF);
		msim_cir.add_wire(gcir.wname(p.second), wtype::IN);
	}

	id2idmap unr_map;
	uint num_rounds = _get_cyclic_depth(enc_ckt);

	for (uint f = 0; f < num_rounds; f++) {
		foreach_wire(gcir, w, wid) {
			if (_is_in(wid, feedback_pairs.getmap()) || _is_in(wid, feedback_pairs.getImap())) { continue; }
			else if (w.isInput()) {
				if (f == 0) unr_map[wid] = un_enc_cir.add_wire(gcir.wname(wid), wtype::IN);
			}
			else if (w.isKey()) {
				if (f == 0) unr_map[wid] = un_enc_cir.add_wire(gcir.wname(wid), wtype::KEY);
			}
			else if (w.isOutput()) {
				if (f == num_rounds - 1) unr_map[wid] = un_enc_cir.add_wire(gcir.wname(wid), wtype::OUT);
				else unr_map[wid] = un_enc_cir.add_wire(gcir.wname(wid) + fmt("_%03d", f), wtype::INTER);
			}
			else {
				unr_map[wid] = un_enc_cir.add_wire(gcir.wname(wid) + fmt("_%03d", f), wtype::INTER);
			}
		}

		for (auto& p : feedback_pairs.getmap()) {
			if (f == 0) {
				id init_wid = unr_map[p.second] = un_enc_cir.add_wire(gcir.wname(p.second), wtype::IN);
				init_wids.insert(init_wid);
			}
			else {
				unr_map[p.second] = unr_map.at(p.first);
			}
		}


		for (auto& p : feedback_pairs.getmap()) {
			unr_map[p.first] = un_enc_cir.add_wire(gcir.wname(p.first) + fmt("_%03d", f), wtype::INTER);
		}


		foreach_gate(gcir, g, gid) {
			un_enc_cir.add_gate("", g, unr_map);
		}
	}


	std::cout << "unrolled circuit pre simplify: ";
	un_enc_cir.print_stats();

	ext::abc_simplify(un_enc_cir, true);

	set_sim_cir(msim_cir);
	set_enc_cir(un_enc_cir);

	// sim_cir->write_bench();
	// enc_cir->write_bench();

	dec_sat::_prepare_sat_attack();

}


} // namespace dec_ns


