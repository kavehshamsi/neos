/*
 * mc.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: kaveh
 */

#include "dec/mcdec.h"

namespace dec_ns {


void mcdec_int_aig::_build_mitter_int() {

	// neos_println("mcdec_int_aig called");

	mcdec_bmc_int::_build_mitter_int();

	id2alitmap wid2aidmap;
	mitt_aig.init_from_ckt(mitt_ckt, wid2aidmap);


	assert( ext::check_equivalence_abc(mitt_aig.to_circuit(), mitt_ckt) );

	for (auto kid : mitt_aig.keys()) {
		string kname = mitt_aig.ndname(kid);
		if ( _is_primed_name(kname) ) { // k1 and k2 maps
			id enckid = enc_cir->find_wcheck( _unprime_name(kname) );
			enc2mitt_kmap[1][enckid] = kid;
		}
		else {
			id enckid = enc_cir->find_wcheck(kname);
			enc2mitt_kmap[0][enckid] = kid;
		}
	}

	for (auto xid : mitt_aig.nonkey_inputs()) {
		const string& nm = mitt_aig.ndname(xid);
		if ( !_is_in("_$io", nm) ) {
			id encxid = enc_cir->find_wcheck(nm);
			enc2mitt_xmap[encxid] = xid;
		}
	}

	V1(cout << "mitter circuit\n";
		mitt_aig.write_bench());

	bmc_out = wid2aidmap.at(bmc_out).lid();
	ioc_out = wid2aidmap.at(ioc_out).lid();

	assert(mitt_aig.node_exists(bmc_out));
	assert(mitt_aig.node_exists(ioc_out));

	mitt_aunr.set_tr(mitt_aig, mitt_aig.keys(), false);

	_restart_bmc_engine();
}

void mcdec_int_aig::_restart_to_unrolled_mitt() {
	std::cout << "bmc lit : " << SolverNs::toInt(_get_mittlit(0, mitt_aig.ndname(bmc_out)));

	std::dynamic_pointer_cast<BmcAig>(mittbmc)->restart_to_unrolled(mitt_aunr, "mittOut");

	std::cout << "bmc lit : " << SolverNs::toInt(_get_mittlit(0, mitt_aig.ndname(bmc_out)));
	term_initialized = false;
}

void mcdec_int_aig::_mitt_sweep() {

	assert(mitt_sweep != 0 && iter > 1);

	std::cout << "Mitter sweeping\n";

	if (mitt_sweep == 1) {
		_mitt_sweep_1();
	}
	else if (mitt_sweep == 2) {
		_mitt_sweep_2();
	}
}

void mcdec_int_aig::_mitt_sweep_1() {

	std::cout << "AIG MITTER SWEEP\n";
	using namespace opt_ns;
	assert(mitt_sweep != 0 && iter > 1);

	std::cout << "mitter sweeping\n";

	SATsweep::sweep_params_t sat_param;
	sat_param.set_external_solver(new sat_solver(*mittbmc->BmcS)); // copy the solver

	auto& unrmitt = mitt_aunr.unaig;

	for (auto kid : unrmitt.keys()) {
		sat_param.vmap[kid] = _get_keylit(unrmitt.ndname(kid));
	}

	// unrmitt.write_bench("./bench/testing/mitt_aig_unroll.bench");
	aig_t simp_mitt = unrmitt;
	ext::abc_simplify(simp_mitt);

	// tie outputs to one
	for (auto outid : unrmitt.outputs()) {
		if (_is_in("mittOut", unrmitt.ndname(outid))) {
			slit p = sat_param.S->create_new_var();
			sat_param.vmap[outid] = p;
			sat_param.assumps.push_back(p);
		}
	}
	sat_param.prop_limit = 10;

	aig_t unrmitt_orig = unrmitt;

	SATAigSweep ssw(unrmitt, sat_param);
	mitt_aunr.sweep_unrolled_aig(ssw);

	_restart_to_unrolled_mitt();
	assert(kc_sweep >= 1);
	_add_kc_aig_to_solver(kc_aig);

	delete sat_param.S;

	if (mitt_sweep == 2) {
		idset settled_nodes;
		for (auto& cl : ssw.classes) {
			if (cl.size() > 1) {
				for (auto al : cl) {
					if (unrmitt_orig.node_exists(al) && !unrmitt_orig.isKey(al.lid())) {
						string wn1 = unrmitt_orig.ndname(al.lid());
						if (_is_primed_name(wn1)) {
							auto pr = _unframe_name(wn1);
							string wn2 = _unprime_name(pr.first);
							// std::cout << "primed: " << wn1 << "  unprime: " << wn2 << "\n";
							id wid2 = unrmitt_orig.find_node( _frame_name(wn2, pr.second) );
							if (wid2 != -1 && _is_in(alit(wid2, 0), cl)) {
								std::cout << "settled node detected: " << unrmitt_orig.ndname(wid2) << "\n";
								id mitt_aig_wid = mitt_aig.find_wcheck(wn2);
								settled_nodes.insert(mitt_aig_wid);
								assert(mitt_aig_wid != -1);
							}
						}
					}
				}
			}
		}

		_simplify_with_settled_nodes(settled_nodes);
	}
}

void mcdec_int_aig::_mitt_sweep_2() {

	std::cout << "mitt sweep 2: (restart to ABC resynthesis with no settled node detection)\n";
	neos_check(kc_sweep != 0, "mitt_sweep not possible with kc_sweep <= 1");

	static bool skip_mitt_sweep = false;

	if (skip_mitt_sweep) {
		std::cout << "skipping mitter-sweep due to diminsihing returns\n";
		return;
	}

	double orig_gates = mitt_aunr.unaig.nAnds();

	auto tm = _start_wall_timer();
	//double nsaved = mitt_aunr.sweep_unrolled_ckt_abc();
	double nsaved = 0;
	stats.simp_time += _stop_wall_timer(tm);

	double perc = (nsaved / orig_gates) * 100;
	if (perc < 5) {
		skip_mitt_sweep = true;
	}

}


// returns true if restart occurs
bool mcdec_int_aig::_simplify_with_settled_nodes(const idset& settled_nodes) {

	std::cout << "\naig simplify with settled nodes: \n";

	idset settled_key;
	for (auto wid : settled_nodes) {
		idset trans_fanin;
		trans_fanin = mitt_aig.trans_fanin(wid);
		std::cout << mitt_aig.ndname(wid) << " : ";
		for (auto wid2 : trans_fanin) {
			std::cout << " " << mitt_aig.ndname(wid2) << ", ";
			if (mitt_aig.isKey(wid2))  {
				std::cout << "settled key " << mitt_aig.ndname(wid2) << "\n";
				settled_key.insert(wid2);
			}
		}
		std::cout << "\n";
	}

	// satisfy key conditions
	if (!settled_key.empty()) {

		assert(mittbmc->BmcS->solve());

		id2boolHmap settled_key_map;

		for (auto kid : settled_key) {
			//std::cout << "settled key: " << mitt_ckt.wname(kid) << ": " << mittbmc.getValue(kid, 0) << "\n";
			id enc_kid = enc_cir->find_wcheck(mitt_aig.ndname(kid));
			settled_key_map[enc_kid] = mittbmc->getValue(kid, 0);
		}
		knownkeys = settled_key_map;
		_restart_with_knownkeys();
		return true;
	}
	return false;
}


/*
 * return a single-ended combinational
 * AIG representing a DIS condition
 */
void mcdec_int_aig::_get_unrolled_dis_aig(dis_t& dis, aig_t& unrolledaig) {


	if (dis.length() > (uint)mitt_aunr.cur_len()) {

		//std::cout << "iockt extending\n";
		//std::cout << "dis len : " << dis.length() << " mitt unrolling frame : " << mitt_aunr.cur_len() << "\n";
		mitt_aunr.extend_to(dis.length());

	}

	// trim to ioc_outs
	aig_t ioc_trimed = mitt_aunr.get_zeroinit_unaig();
	oidset io_outs;
	for (uint i = 0; i < dis.length(); i++) {
		io_outs.insert(ioc_trimed.find_wcheck(_frame_name("iocOut", i)));
	}
	ioc_trimed.trim_aig_to(io_outs);
	unrolledaig = ioc_trimed;


	// tie inputs
	//std::cout << "dis len " << dis.length() << " " << mitt_aunr.cur_len() << "\n";
	id2boolHmap inputmap;
	int f = 0;
	for (auto xid : unrolledaig.inputs()) {
		if (!unrolledaig.isKey(xid)) { // is input only
			string inname = unrolledaig.ndname(xid);
			//std::cout << "inname: " << inname << "\n";
			f = _unframe_name(inname).second;

			if (_is_in("_$io", inname)) {
				string base_name = inname.substr(0, inname.find("_$io"));
				int enc_index = ewid2ind.at(enc_cir->find_wcheck(base_name));
				// cout << unrollediockt.wname(xid) << " : " << dis.ys[f][enc_index] << " frame: " << f << "\n";
				// cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
				inputmap[xid] = dis.ys.at(f).at(enc_index);
			}
			else {
				string base_name = _unframe_name(inname).first;
				//std::cout << inname << "\n";
				int enc_index = ewid2ind.at(enc_cir->find_wcheck(base_name));
				// cout << unrollediockt.wname(xid) << " : " << dis.xs[f][enc_index] << " frame: " << f << "\n";
				// cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
				inputmap[xid] = dis.xs.at(f).at(enc_index);
			}
		}
	}

	// propagate constants
	unrolledaig.propagate_constants(inputmap);

/*
	std::cout << "done const proping\n";
	std::cout << "unjoined outputs: \n";
	unrolledaig.write_bench();
*/

	// join outputs
	unrolledaig.join_outputs(fnct::AND, "ioend_all" + std::to_string(iter));

	assert(unrolledaig.error_check());
}

void mcdec_int_aig::_aig_simplify(aig_t& ntk, int level) {

	auto tm = _start_wall_timer();

	if (level == 0) {
		return;
	}
	else if (kc_sweep == 1) {
		//
	}
	else if (kc_sweep == 2) { // best neos simplification
		opt_ns::SATsweep::sweep_params_t param;
		param.prop_limit = propagate_limit;
		opt_ns::aig_satsweep(ntk);
		// opt_ns::aig_cutsweep(ntk);
		// opt_ns::aig_bddsweep(kc_aig, bdd_sweep_size_limit);
	}
	else if (kc_sweep == 3) {
		//aig_t kc_aig_simp;
		ext::abc_simplify(ntk);
		//kc_aig.write_bench("./bench/testing/kc_aig.bench");

		/*aig_t abc_ntk = mitt_aig;
		abc_ntk.add_aig(kc_aig);
		std::cout << "WRITING SMV MODEL\n";
		id out = abc_ntk.join_outputs({abc_ntk.find_wcheck("kc_end"),
			abc_ntk.find_wcheck("mittOut")}, fnct::AND, "Gout");
		abc_ntk.trim_aig_to({out});
		abc_ntk.write_bench("./bench/testing/abc_ntk.bench");
		string nuxmv_smv;
		circuit abc_cir = abc_ntk.to_circuit();
		mcdec_nuxmv::_convert_to_smv(abc_cir, abc_cir.keys(), nuxmv_smv);
		//make_frozen_with_latch(abc_cir, abc_cir.keys());
		abc_cir.write_bench("./bench/testing/abc_cir_nf.bench");
		make_frozen_with_latch(abc_cir, abc_cir.keys());
		abc_cir.write_bench("./bench/testing/abc_cir.bench");
		std::ofstream fl;
		fl.open("./bench/testing/mitt.smv");
		fl << nuxmv_smv;
		fl.close();*/
		// BmcAig bmcaig(abc_ntk, abc_ntk.keys(), bmc_config);
		//bmcaig.bmc_solve(20, out);
	}
	assert(ntk.error_check());
	//_add_unrolled_aig_to_solver(kc_aig);

	stats.simp_time += _stop_wall_timer(tm);

}

void mcdec_int_aig::_add_dis_constraint_kc(dis_t& dis) {

	static int32_t kc_counter = 0;
	kc_counter++;

	aig_t unrolledioaig;
	_get_unrolled_dis_aig(dis, unrolledioaig);

	//cout << "\n\nUNROLLED AIG after tying\n";
	//unrolledioaig.write_aig();

	//unrollediockt.rewrite_gates();
	// unrollediockt.write_bench();
	if (kc_sweep_iterative) {
		_aig_simplify(unrolledioaig, kc_sweep_iterative);
	}

	if (kc_sweep ==  1) {
		_add_kc_aig_to_solver(unrolledioaig);
	}
	else if (kc_sweep > 1) {
		kc_aig.add_aig(unrolledioaig, "");
		kc_aig.join_outputs(fnct::AND, "kc_end");
		assert(kc_aig.error_check());

		// kc_ckt.write_verilog();
		if (kc_counter == kc_sweep_period) {
			kc_counter = 1;

			_aig_simplify(kc_aig, kc_sweep);
			_restart_from_aig(kc_aig);
		}
		else {
			_add_kc_aig_to_solver(unrolledioaig);
		}
		std::cout << "depth [" << dis.length() <<
				"]: key-condition AIG num nodes: " << kc_aig.nNodes() << "\n";
		kc_aig.write_bench("kc.bench");
	}

	if (kcbdd_option) {
		kbdd.add_ckt_to_bdd(kc_aig.to_circuit(), fnct::AND);
		if (kcbdd_size_limit != -1 && kbdd.num_nodes() > kcbdd_size_limit) {
			std::cout << "BDD exceeded size limit. Disabling BDD tracking\n";
			kcbdd_option = 0;
		}
		std::cout << "kbdd num nodes : " << kbdd.num_nodes() << "\n";

		if (iter % kc_sweep_period == 0) {
			circuit bddckt;
			kbdd.to_circuit(bddckt);
			_restart_from_ckt(bddckt);
		}
	}

}

void mcdec_bmc_int::_restart_from_aig(const aig_t& aig) {

	std::cout << "restarting to kc_aig\n";
	aig.write_bench("to_restart.bench");
	assert(aig.error_check());
	//_restart_bmc_engine();
	_restart_to_unrolled_mitt();
	_add_termination_links();
	_add_kc_aig_to_solver(aig);

}


void mcdec_int_aig::_restart_bmc_engine() {

	// reinit BMC engine
	oidset frozens = mitt_aig.keys();

	mittbmc = std::make_shared<BmcAig>(mitt_aig, frozens, bmc_config);

	// solve for one round to fill up the
	// mittbmc.lmaps with frozen literals
	mittbmc->bmc_solve(1, bmc_out);

	// add the termination_check link clauses
	_add_termination_links();
}


// add key-condition-aig to solver (single-ended aig over key variables)
void mcdec_bmc_int::_add_kc_aig_to_solver(const aig_t& uncir) {

	assert(uncir.nOutputs() == 1);

	sat_solver& S = *mittbmc->BmcS;

	id2litmap_t wid2lit_map;

	foreach_node(uncir, nd, nid) {
		if (uncir.isKey(nid)) {
			wid2lit_map[nid] = _get_keylit(uncir.ndname(nid));
		}
		else if (uncir.isConst0(nid)) {
			wid2lit_map[nid] = S.false_lit();
		}
		else if (nd.isOutput()) {
			slit p = S.create_new_var();
			wid2lit_map[nid] = p;
			S.add_clause(p);
		}
		else {
			wid2lit_map[nid] = S.create_new_var();
		}
	}
	add_aig_gate_clauses(S, uncir, wid2lit_map);
}

}
