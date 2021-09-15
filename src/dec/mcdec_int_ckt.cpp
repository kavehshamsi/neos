/*
 * mc.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: kaveh
 */

#include "dec/mcdec.h"
#include "main/main_util.h"

#define foreach_encmitt_xid(enc_xid, mitt_xid) \
		RANGE_INIT_FOR(const auto _macro_mcdec_enc_mitt_xids : enc_mitt_xids, \
				(id enc_xid = _macro_mcdec_enc_mitt_xids.first)\
				(id mitt_xid = _macro_mcdec_enc_mitt_xids.second), ((void)enc_xid)((void)mitt_xid))


namespace dec_ns {


mcdec_bmc::mcdec_bmc(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corrkey)
	: dec(sim_ckt, enc_ckt, corrkey) {

	foreach_latch(*enc_cir, dffid, dffout, dffin) {
		enc_lpairs.push_back({dffin, dffout});
	}

}

void mcdec_bmc::get_opt_parser(mcdec_bmc& dc, boost::program_options::options_description& op) {

	dec::get_opt_parser((dec&)dc, op);
	mcdec_bmc::append_opt_parser(dc, op);

}

void mcdec_bmc::append_opt_parser(mcdec_bmc& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("bmc_bound_limit", po::value<int>(&dc.bmc_bound_limit), "negative key-condition tracking")
		;

}

void mcdec_bmc_int::get_opt_parser(mcdec_bmc_int& dc, boost::program_options::options_description& op) {

	dec::get_opt_parser((dec&)dc, op);
	mcdec_bmc::append_opt_parser(dc, op);
	mcdec_bmc_int::append_opt_parser(dc, op);

}


void mcdec_bmc_int::append_opt_parser(mcdec_bmc_int& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("use_satsimp", po::value<int>(&dc.use_satsimp), "Use Glucouse SAT-solver simplification")
		("kc_sweep", po::value<int>(&dc.kc_sweep), "key-condition sweeping")
		("kc_method", po::value<int>(&dc.kc_method), "key-condition method (used in ic3 solver etc.)")
		("kc_sweep_period", po::value<int>(&dc.kc_sweep_period), "key-condition sweeping period")
		("bmc_sweep", po::value<int>(&dc.bmc_sweep), "sweeping in BMC engine")
		("relative_sweep", po::value<int>(&dc.relative_sweep), "sweeping in BMC engine") // FIXME: is this necessary?
		("prop_limit", po::value<int>(&dc.propagate_limit), "propagation limit in SAT-sweeping")
		("kcbdd", po::value<int>(&dc.kcbdd_option), "BDD based key-condition tracking")
		("kcbdd_size_limit", po::value<int>(&dc.kcbdd_size_limit), "BDD size limit in kc_condition")
		("bdd_sweep_size_limit", po::value<int>(&dc.bdd_sweep_size_limit), "BDD size limit for BDD-sweeping")
		("mitt_sweep", po::value<int>(&dc.mitt_sweep), "mitter sweep")
		("mitt_sweep_period", po::value<int>(&dc.mitt_sweep_period), "mitter sweep period");

}

void mcdec_int_ckt::get_opt_parser(mcdec_int_ckt& dc, boost::program_options::options_description& op) {

	mcdec_bmc_int::get_opt_parser(dc, op);
	mcdec_int_ckt::append_opt_parser(dc, op);

}

void mcdec_int_ckt::append_opt_parser(mcdec_int_ckt& dc, boost::program_options::options_description& op) {}


int mcdec_int_aig::parse_args(int argc, char** argv) {

	boost::program_options::options_description op;
	mcdec_bmc_int::get_opt_parser(*this, op);

	if ( mnf::parse_args(argc, argv, op) || print_help) {
		std::cout << op << "\n";
		return 0;
	}

	read_sim_enc_key_args(pos_args, !oracle_binary.empty());

	if (pos_args.size() != 1) {
		neos_error("usage: neos -d int <sim_cir/enc_cir> <sim_cir/corrkey> [iteration_limit]");
		return 0;
	}
	iteration_limit = stoi(pos_args[0]);

	return 1;
}



int mcdec_int_ckt::parse_args(int argc, char** argv) {

	boost::program_options::options_description op;
	mcdec_int_ckt::get_opt_parser(*this, op);

	if ( mnf::parse_args(argc, argv, op) || print_help) {
		std::cout << op << "\n";
		return 1;
	}

	read_sim_enc_key_args(pos_args, !oracle_binary.empty());

	if (dec_method == "intrfq") {
		if (pos_args.size() != 4) {
			neos_error("usage: neos -d intrfq <sim_cir/enc_cir> <sim_cir/corrkey> [dis_period] [num_rand_patts] [skew_threshold] [iteration_limit]");
			return 1;
		}
		rfq_mode = 1;
		rfq_dis_period = stoi(pos_args[0]);
		rfq_num_rand_patts = stoi(pos_args[1]);
		rfq_skew_threshold = stof(pos_args[2]);
		iteration_limit = stoi(pos_args[3]);
	}
	else {
		if (pos_args.size() != 1) {
			neos_error("usage: neos -d int <sim_cir/enc_cir> <sim_cir/corrkey> [iteration_limit]");
			return 1;
		}
		iteration_limit = stoi(pos_args[0]);
	}

	bmc_config.tran_simp = bmc_sweep;
	bmc_config.sat_simp = use_satsimp;

	if (bmc_sweep > 2) {
		neos_error("invalid BMC-sweep option: " << bmc_sweep);
		return 1;
	}

	bmc_config.relative_simp = relative_sweep;
	bmc_config.propagation_limit = propagate_limit;

	if (kcbdd_option != 0 && kcbdd_size_limit != 0) {
		kcbdd.size_limit = kcbdd_size_limit;
	}

	std::cout << "vebrosity: " << verbose << "\n";
	print_options();

	return 0;

}


mcdec_bmc_int::mcdec_bmc_int(const circuit& sim_ckt, const circuit& enc_cir, const boolvec& corrkey) :
				mcdec_bmc(sim_ckt, enc_cir, corrkey) {

	// initialize bdd manager
	//kbdd.init_bddvars_from_cktins(enc_cir, OPT_KEY);
}


void mcdec_bmc_int::solve() {

	auto tm = _start_wall_timer();

	_safe_set_interkey(utl::random_boolvec(num_keys));

	_init_handlers();
	_build_mitter_int();
	// _build_iockt_int();

	while (true) {

		dis_t dis;
		int solve_res = _solve_for_dis_int(dis);
		if (solve_res == 1) {
			V1(cout << "//-------ITERATION:   " << iter << "----------\n");
			dis.ys = query_seq_oracle(dis.xs);
			_print_dis(dis);
			V2(_simulate_enc_int(dis));
			_add_dis_constraint_int(dis);
			_perdis_opt_routines();
		}
		else if (solve_res == -1) { // may occur at higher bound
			//bmc_len += bmc_bound_steps;
			//cout << "increasing bmc bound to " << bmc_len << "\n";
			_perlen_opt_routines();

			if (bmc_len < bmc_bound_limit) {
				bmc_len += 1;
				cout << "increasing bmc bound to " << bmc_len << "\n";
			}
			else {
				cout << "reached bmc bound limit\n";
				break;
			}
		}

		if (solve_res == 0 || _check_termination_int()) {
			// definite termination
			break;
		}

		if (iter++ == iteration_limit) {
			break;
		}
	}

	stats.total_time += _stop_wall_timer(tm);

	extract_key_int();
	cout << "\ndone\n";
}

void mcdec_bmc_int::print_options() {

	std::cout << "key-condition sweeping mode: " << kc_sweep << "\n";
	std::cout << "BMC sweeping mode: " << bmc_sweep << "\n";
	std::cout << "Relative to kckt SAT-sweeping: " << relative_sweep << "\n";
	std::cout << "with propagation limit: " << propagate_limit << "\n";
	std::cout << "with mitter sweep: " << mitt_sweep << "\n";
	std::cout << "BDD analysis option: " << kcbdd_option << "\n";
	std::cout << "KC BDD size limit: " << kcbdd_size_limit << "\n";
	std::cout << "BMC bound limit: " << bmc_bound_limit << "\n";

}

// optimization routines per single dis
void mcdec_bmc_int::_perdis_opt_routines() {}

void mcdec_int_ckt::_perdis_opt_routines() {}


// optimization steps per depth increase
void mcdec_bmc_int::_perlen_opt_routines() {
	if (mitt_sweep != 0 && (bmc_len % mitt_sweep_period == 0))
		_mitt_sweep();
}

void mcdec_bmc_int::_build_mitter_int() {

	//neos_println("called mcdec_int mitter build");

	mitt_ckt = *enc_cir;
	circuit ckt2 = *enc_cir;

	mitt_ckt.add_circuit(OPT_INP, ckt2, "_$1");

	// add output comparator
	oidset output_set = enc_cir->outputs();
	idvec mitt_outs, ioc_outs;
	int i = 0;
	for (auto oid_enc : output_set) {
		std::string oname = enc_cir->wname(oid_enc);
		id oid0 = mitt_ckt.find_wcheck(oname);
		id oid1 = mitt_ckt.find_wcheck(_primed_name(oname));
		mitt_outs.push_back(mitt_ckt.join_outputs({oid0, oid1}, fnct::XOR, "mittXor" + std::to_string(i)));

		id ioy = mitt_ckt.add_wire(oname + "_$io", wtype::IN);
		id xor_out0 = mitt_ckt.add_wire("io0Xor" + std::to_string(i), wtype::OUT);
		id xor_out1 = mitt_ckt.add_wire("io1Xor" + std::to_string(i), wtype::OUT);
		mitt_ckt.add_gate({oid0, ioy}, xor_out0, fnct::XNOR);
		ioc_outs.push_back(xor_out0);
		mitt_ckt.add_gate({oid1, ioy}, xor_out1, fnct::XNOR);
		ioc_outs.push_back(xor_out1);

		i++;
	}

	ioc_out = mitt_ckt.join_outputs(ioc_outs, fnct::AND, "iocOut");
	mitt_out = bmc_out = mitt_ckt.join_outputs(mitt_outs, fnct::OR, "mittOut");

	// add next state comparator
	i = 0;
	idvec nsXors;
	foreach_latch(mitt_ckt, dffid, dffout, dffin) {
		string nsName = mitt_ckt.wname(dffin);
		if ( _is_primed_name(nsName) ) {
			id dffin_1 = mitt_ckt.find_wcheck( _unprime_name(nsName) );
			nsXors.push_back(mitt_ckt.join_outputs({dffin_1, dffin}, fnct::XOR, "mittNSXor" + std::to_string(i++)));
		}
	}
	term_out = mitt_ckt.join_outputs(nsXors, fnct::OR, "mittNSXor");

	encmitt_xids.clear();
	encmitt_kids.clear();

	for (auto xid : enc_cir->inputs()) {
		encmitt_xids.push_back({xid, mitt_ckt.find_wcheck(enc_cir->wname(xid))});
	}
	for (auto kc : {0, 1}) {
		encmitt_kids.push_back(vector<idpair>());
		for (auto kid : enc_cir->keys()) {
			std::string kname = enc_cir->wname(kid) + ((kc == 1) ? "_$1":"");
			encmitt_kids[kc].push_back( {kid, mitt_ckt.find_wcheck(kname)} );
		}
	}


	// add combinational mitter and its comparator
	//_add_termination_ckt();

	// map enc to mitt wires

	/*
	io_cunr.set_tr(mitt_ckt);
	io_cunr.frozens = mitt_ckt.keys();
	io_cunr.zero_init = true;
	 */

}

void mcdec_int_ckt::_build_mitter_int() {
	// std::cout << "called mcdec_int_ckt mitter build\n";
	mcdec_bmc_int::_build_mitter_int();
	mitt_cunr = CircuitUnroll();

	for (auto kid : mitt_ckt.keys()) {
		string kname = mitt_ckt.wname(kid);
		if ( _is_primed_name(kname) ) { // k1 and k2 maps
			id enckid = enc_cir->find_wcheck( _unprime_name(kname) );
			enc2mitt_kmap[1][enckid] = kid;
		}
		else {
			id enckid = enc_cir->find_wcheck(kname);
			enc2mitt_kmap[0][enckid] = kid;
		}
	}

	for (auto xid : mitt_ckt.inputs()) {
		if ( !_is_in("_$io", mitt_ckt.wname(xid)) ) {
			id encxid = enc_cir->find_wcheck(mitt_ckt.wname(xid));
			enc2mitt_xmap[encxid] = xid;
		}
	}

	V1(cout << "mitter circuit\n";
		mitt_ckt.write_bench());

	mitt_cunr.set_tr(mitt_ckt, mitt_ckt.keys(), false);

	_restart_bmc_engine();
}

void mcdec_bmc_int::_build_iockt_int() {

	assert(0);
/*
	if (!unified_mitt_ioc) {

		iockt = enc_cir;

		idset output_set = iockt.outputs();
		int i = 0;
		for (auto oid : output_set) {

			std::string oname = iockt.wname(oid);
			id ioy = iockt.add_wire(oname + "$io" + std::to_string(i), wtype::IN);

			id xor_out = iockt.add_wire("io1Xor" + std::to_string(i), wtype::OUT);
			iockt.add_gate({oid, ioy}, xor_out, fnct::XNOR);
			iockt.setwiretype(oid, wtype::INTER);

			i++;
		}

		iockt.join_outputs(fnct::AND, "ioend");

		if (use_aig) {
			id2alitmap wid2almap;
			ioaig.init_from_ckt(iockt, wid2almap);
			io_aunr.tran = ioaig;
			io_aunr.tran.write_bench();
			D1(
			ioaig.write_aig();
			std::cout << "checking io-ckt equivalence\n";
			ext::check_equivalence_abc(iockt, ioaig.to_circuit());
			)
		}

		V1(iockt.write_bench());
	}
*/

}

void mcdec_bmc_int::_sweep_wrt_key_conditions(circuit& cir) {

	SATsweep::sweep_params_t sat_param;

	sat_param.S = new sat_solver(*mittbmc->BmcS); // copy the solver
	sat_param.external_solver = true;

	for (auto kid : cir.keys()) {
		auto kname = cir.wname(kid);
		sat_param.vmap[kid] = mittbmc->lmaps[0].at(mitt_ckt.find_wcheck(kname));
	}

	circuit_satsweep(cir, sat_param);

	delete sat_param.S;
}

void mcdec_int_ckt::_mitt_sweep() {

	assert(mitt_sweep != 0 && iter > 1);

	std::cout << "Mitter sweeping\n";

	if (mitt_sweep == 1) {
		_mitt_sweep_1();
	}
	else if (mitt_sweep == 2) {
		_mitt_sweep_2();
	}
}

void mcdec_int_ckt::_mitt_sweep_1() {

	SATsweep::sweep_params_t sat_param;
	sat_param.set_external_solver(new sat_solver(*mittbmc->BmcS)); // copy the solver

	if (mitt_cunr.cur_len() < bmc_len) {
		mitt_cunr.extend_to(bmc_len);
	}

	circuit mtc = mitt_cunr.uncir;

	for (auto kid : mtc.keys()) {
		sat_param.vmap[kid] = _get_keylit(mtc.wname(kid));
	}

	//mtc.write_bench("./bench/testing/mitt_ckt_unroll.bench");

	// tie outputs to one
	for (auto outid : mtc.outputs()) {
		if (_is_in("mittOut", mtc.wname(outid))) {
			slit p = sat_param.S->create_new_var();
			sat_param.vmap[outid] = p;
			sat_param.assumps.push_back(p);
		}
	}
	sat_param.prop_limit = sat_param.mitt_prop_limit = 10;

	SATCircuitSweep ssw(mtc, sat_param);

	ssw.analyze();

	idset settled_nodes;
	for (auto& cl : ssw.classes) {
		if (cl.size() > 1) {
			for (auto wid : cl) {
				std::string wn1 = mtc.wname(wid);
				if (_is_primed_name(wn1)) {
					auto pr = _unframe_name(wn1);
					std::string wn2 = _unprime_name(pr.first);
					// std::cout << "primed: " << wn1 << "  unprime: " << wn2 << "\n";
					id wid2 = mtc.find_wcheck( _frame_name(wn2, pr.second) );
					if (_is_in(wid2, cl)) {
						//std::cout << "found settled node: " << mtc.wname(wid2) << " at frame: " << pr.second << "\n";
						id mitt_ckt_wid = mitt_ckt.find_wcheck(wn2);
						settled_nodes.insert(mitt_ckt_wid);
					}
				}
			}
		}
	}

	_simplify_with_settled_nodes(settled_nodes);

	std::cout << "mitter simplify: ";
	ssw.commit();

	delete sat_param.S;

}

void mcdec_int_ckt::_mitt_sweep_2() {

	std::cout << "mitt sweep 2: (restart to ABC resynthesis with no settled node detection)\n";
	neos_check(kc_sweep != 0, "mitt_sweep not possible with kc_sweep <= 1");

	static bool skip_mitt_sweep = false;

	if (skip_mitt_sweep) {
		std::cout << "skipping mitter-sweep due to diminsihing returns\n";
		return;
	}

	double orig_gates = mitt_cunr.uncir.nGates();

	auto tm = _start_wall_timer();
	double nsaved = mitt_cunr.sweep_unrolled_ckt_abc();
	stats.simp_time += _stop_wall_timer(tm);

	double perc = (nsaved / orig_gates) * 100;
	if (perc < 5) {
		skip_mitt_sweep = true;
	}

}


// returns true if restart occurs
bool mcdec_int_ckt::_simplify_with_settled_nodes(const idset& settled_nodes) {

	std::cout << "circuit simplify with settled nodes: \n";

	idset settled_key;
	idset visited;
	for (auto wid : settled_nodes) {
		if (_is_not_in(wid, visited)) {
			idset trans_fanin;
			mitt_ckt.get_trans_fanin(wid, trans_fanin);
			for (auto wid2 : trans_fanin) {
				visited.insert(wid2);
				if (mitt_ckt.isKey(wid2))  {
					settled_key.insert(wid2);
				}
			}
		}
	}

	// satisfy key conditions
	if (!settled_key.empty()) {

		assert(mittbmc->BmcS->solve());

		id2boolHmap settled_key_map;

		for (auto kid : settled_key) {
			std::cout << "found settled key: " << mitt_ckt.wname(kid) << " -> " << mittbmc->getValue(kid, 0) << "\n";
			id enc_kid = enc_cir->find_wcheck(mitt_ckt.wname(kid));
			settled_key_map[enc_kid] = mittbmc->getValue(kid, 0);
		}

		knownkeys = settled_key_map;
		_restart_with_knownkeys();
		return true;
	}
	return false;
}

void mcdec_bmc_int::_restart_with_knownkeys() {

	std::cout << "restarting with ";
	print_known_keys();

	circuit ecir = *enc_cir;
	ecir.propagate_constants(knownkeys);
	set_enc_cir(ecir);

	_restart();

}

void mcdec_bmc_int::_restart() {

	_build_mitter_int();
	_restart_bmc_engine();

}

// for general internal BMC checking
int mcdec_bmc_int::_solve_for_dis_int(dis_t& dis) {

	auto tm = _start_wall_timer();

	if (mittbmc->bmc_solve(bmc_len, bmc_out)) {

		stats.bmc_time += _stop_wall_timer(tm);

		vector<id2boolmap> mittInmaps(mittbmc->cexLen + 1, id2boolmap());

		dis.xs = vector<boolvec> (mittbmc->cexLen + 1, boolvec(num_ins, false));

		for (int f = 0; f <= mittbmc->cexLen; f++) {
			for (auto xp : enc2mitt_xmap ) {
				bool inVal = mittbmc->getValue(xp.second, f);
				dis.xs[f][ewid2ind.at(xp.first)] = inVal;
				mittInmaps[f][xp.second] = inVal;
			}
		}

		for (int kvc : {0, 1}) {
			for (auto kid : enc_cir->keys() ) {
				id mkid = enc2mitt_kmap[kvc].at(kid);
				bool keyVal = mittbmc->getValue(mkid, 0);
				dis.ks[kvc].push_back(keyVal);
				for (int f = 0; f <= mittbmc->cexLen; f++) {
					mittInmaps[f][mkid] = keyVal;
				}
			}
		}
		_safe_set_interkey(dis.ks[0]);

		if (last_dis.xs == dis.xs) {
			std::cout << "dis error : dis loop detected\n";
			_print_dis(dis);
			get_last_key();
			check_key(interkey);
			exit(EXIT_FAILURE);
		}
		last_dis = dis;

		//all_dises.push_back(dis);

		return 1;
	}

	stats.bmc_time += _stop_wall_timer(tm);

	return -1;
}

void mcdec_bmc_int::_tie_input(circuit& cir, id wid, bool val) {

	idvec wireFanout = cir.wfanout(wid);

	for (auto gid : wireFanout) {
		cir.update_gateFanin(gid, wid, (val) ? cir.get_const1() : cir.get_const0() );
		if (val)
			cir.getwire(cir.get_const1()).addfanout(gid);
		else
			cir.getwire(cir.get_const0()).addfanout(gid);
	}

	cir.remove_wire(wid);
}

void mcdec_bmc_int::_add_dis_constraint_int(dis_t& dis) {

	stats.dis_count++;
	stats.dis_maxlen = MAX(stats.dis_maxlen, dis.length());

	if (kc_sweep != 0) {
		_add_dis_constraint_kc(dis);
	}
	else {
		_add_dis_constraint_int(dis, *enc_cir, *mittbmc->BmcS);
	}
}

void mcdec_bmc_int::_add_dis_constraint_int(dis_t& dis, const circuit& cir, sat_solver& S) {

	assert(mittbmc->BmcS);

	for (int kvc : {0, 1} ) { // for k1 and k2
		vector<id2litmap_t> lmaps;

		for (uint f = 0; f < dis.length(); f++) {
			lmaps.push_back(id2litmap_t());
			// add all wire variables except for k1 and k2

			// first the dffouts
			foreach_latch(cir, dffid, dffout, dffin) {
				if (f == 0) { // init to zero
					lmaps[f][dffout] = S.false_lit();
				}
				else { // init to previous frame dffin
					lmaps[f][dffout] = lmaps[f - 1].at(dffin);
				}
			}

			// the rest of the wires

			foreach_wire(cir, w, wid) {
				if (cir.isConst(wid)) {
					lmaps[f][wid] = (cir.is_const1(wid)) ? S.true_lit():S.false_lit();
				}
				else if (w.isKey()) { // k1 and k2
					lmaps[f][wid] = _get_keylit(wid, kvc);
				}
				else if (w.isInput()) { // disc inputs
					lmaps[f][wid] = dis.xs[f][ewid2ind.at(wid)] ? S.true_lit():S.false_lit();
				}
				else if (w.isOutput()) { // disc outputs
					bool ov = dis.ys[f][ewid2ind.at(wid)];
					if (_is_in(wid, lmaps[f])) {
						slit p = lmaps[f].at(wid);
						S.add_clause(ov ? p:~p);
					}
					else
						lmaps[f][wid] = ov ? S.true_lit():S.false_lit();
				}
				else if (cir.isLatch_output(wid)) {} // skip latch outputs
				else { // internal wires including latch inputs
					lmaps[f][wid] = S.create_new_var();
				}
			}

			assert(lmaps[f].size() == (uint)cir.nWires());

			// add all gate clauses
			add_ckt_gate_clauses(S, cir, lmaps[f]);

		}
	}
	// check solver consistency
	//assert(S.solve());

	std::cout << "  dis: " << stats.dis_count << "  stats -> ";
	mittbmc->BmcS->print_stats();
}


void mcdec_int_ckt::_unroll_ckt(const circuit& cir,
		circuit& uncir, int numFrames, const idset& frozens) {

	uncir = circuit();

	id2idmap cir2uncir_wmap;

	if (cir.has_const0())
		cir2uncir_wmap[cir.get_cconst0()] = uncir.get_const0();
	if (cir.has_const1())
		cir2uncir_wmap[cir.get_cconst1()] = uncir.get_const1();

	for (int f = 0; f < numFrames; f++) {

		// take care of latch inputs and outputs
		foreach_latch(cir, dffgid, dffout, dffin) {
			if (f == 0) {
				cir2uncir_wmap[dffout] = uncir.get_const0();
			}
			else {
				cir2uncir_wmap[dffout] = cir2uncir_wmap.at(dffin);
			}
		}

		foreach_wire(cir, w, wid) {
			if (!cir.isConst(wid)) {
				std::string wname = cir.wname(wid);
				if (_is_in(wid, frozens)) {
					if (f == 0) {
						// cout << "frozen while unr: " << cir.wname(wid) << "\n";
						// add frozen inputs once
						cir2uncir_wmap[wid] = uncir.add_wire(wname, cir.wrtype(wid));
					}
				}
				else {
					if (!cir.isLatch_output(wid)) {
						cir2uncir_wmap[wid] = uncir.add_wire(fmt("%s_%03d", wname % f), cir.wrtype(wid));
					}
				}
			}
		}

		foreach_gate(cir, g, gid) {
			if (!cir.isLatch(gid)) {
				// map wires
				idvec unfanins;
				for (auto fanin : cir.gfanin(gid)) {
					unfanins.push_back(cir2uncir_wmap.at(fanin));
				}
				id unfanout = cir2uncir_wmap.at(cir.gfanout(gid));

				std::string ungname = fmt("%s_%03d", cir.gname(gid) % f);
				// add gate
				uncir.add_gate(ungname, unfanins, unfanout, cir.gfunction(gid));
			}
		}
	}

/*_get_unrolled_dis_ckt
	// propagate constants
	id2boolmap consts_map;
	uncir.simplify(consts_map);
*/

}

void mcdec_bmc_int::_link_keylits(const aig_t& kaig, id2litmap_t& vmap) {
	for (auto kid : kaig.keys()) {
		auto kname = kaig.ndname(kid);
		vmap[kid] = _get_keylit(kname);
	}
}

/*
 * return a single-ended combinational
 * network representing the DIS-condition
 */
void mcdec_int_ckt::_get_unrolled_dis_ckt(dis_t& dis, circuit& unrollediockt) {

	if ( dis.length() > (uint)mitt_cunr.cur_len() ) {

		//std::cout << "iockt extending\n";
		//std::cout << "dis len : " << dis.length() << " mitt unrolling length : " << mitt_cunr.cur_len() << "\n";
		mitt_cunr.extend_to(dis.length());

	}

	// trim to ioc_outs
	circuit ioc_trimed = mitt_cunr.get_zeroinit_uncir();
	idset io_outs;
	for (uint i = 0; i < dis.length(); i++) {
		io_outs.insert(ioc_trimed.find_wcheck(_frame_name("iocOut", i)));
	}
	ioc_trimed.trim_circuit_to(io_outs);
	unrollediockt = ioc_trimed;

	//D2(cout << "\n\nUNROLLED CKT:\n\n";
	//unrollediockt.write_bench();
	//unrollediockt.error_check();)

	// tie inputs
	id2boolHmap inputmap;
	int f = 0;
	// unrollediockt.print_ports();
	for (auto xid : unrollediockt.inputs()) {
		std::string inname = unrollediockt.wname(xid);
		f = _unframe_name(inname).second;
		if (_is_in("_$io", inname)) {
			std::string enc_name = inname.substr(0, inname.find("_$io"));
			// std::cout << "enc_name: " << enc_name << "\n";
			int enc_index = ewid2ind.at(enc_cir->find_wcheck(enc_name));
			//std::cout << unrollediockt.wname(xid) << " : " << dis.ys[f][enc_index] << " frame: " << f << "\n";
			//std::cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
			inputmap[xid] = dis.ys.at(f).at(enc_index);
		}
		else {
			std::string enc_name = inname.substr(0, inname.find("_"));
			int enc_index = ewid2ind.at(enc_cir->find_wcheck(enc_name));
			//std::cout << unrollediockt.wname(xid) << " : " << dis.xs[f][enc_index] << " frame: " << f << "\n";
			//std::cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
			inputmap[xid] = dis.xs.at(f).at(enc_index);
		}
	}

	// propagate constants
	unrollediockt.propagate_constants(inputmap);
	//unrollediockt.tie_to_constant(inputmap);

	// join outputs
	unrollediockt.join_outputs(fnct::AND, "ioend_all" + std::to_string(iter));

	D1(
	assert(unrollediockt.error_check());
	)

}

void mcdec_int_ckt::_cir_simplify(circuit& cir, int level) {

	auto tm = _start_wall_timer();

	if (level == 0) {
		return;
	}
	else if (kc_sweep == 1) {
		// do nothing
	}
	else if (kc_sweep == 2) { // best neos circuit simplification
		opt_ns::SATsweep::sweep_params_t param;
		param.prop_limit = propagate_limit;
		opt_ns::circuit_satsweep(cir, param);
	}
	else if (kc_sweep == 3) { // best abc circuit simplifcation
		ext::abc_simplify(cir);
	}
	else {
		neos_error("invalid kc_sweep option " << kc_sweep);
	}

	stats.simp_time += _stop_wall_timer(tm);

}


/*
 * add key condition as a circuit and perhaps do simplification
 */
void mcdec_int_ckt::_add_dis_constraint_kc(dis_t& dis) {

	static int32_t kc_counter = 0;
	kc_counter++;

	circuit unrollediockt;
	_get_unrolled_dis_ckt(dis, unrollediockt);
	//cout << "\n\nUNROLLED CKT after tying:\n\n";
	//unrollediockt.write_bench();
	assert(unrollediockt.error_check());

	//unrollediockt.rewrite_gates();
	// unrollediockt.write_bench();
	if (kc_sweep_iterative) {
		_cir_simplify(unrollediockt, kc_sweep_iterative);
	}

	if (kc_sweep == 1) {
		_add_kc_ckt_to_solver(unrollediockt);
	}
	else if (kc_sweep > 1) {
		kc_ckt.add_circuit_byname(unrollediockt, "");
		kc_ckt.join_outputs(fnct::AND, kc_ckt.get_new_wname());

		// kc_ckt.write_verilog();
		if (kc_counter == kc_sweep_period) {
			kc_counter = 1;

			_cir_simplify(kc_ckt, kc_sweep);

			/*circuit abc_cir = mitt_ckt;
			abc_cir.trim_circuit_to({mitt_ckt.find_wcheck("mittOut")});
			abc_cir.add_circuit_byname(kc_ckt, "");
			abc_cir.join_outputs(fnct::AND, "abc_out");
			mc::make_frozen_with_latch(abc_cir, abc_cir.keys());
			abc_cir.write_bench("bench/testing/abc_cir.bench");*/

			assert(kc_ckt.error_check());

			_restart_from_ckt(kc_ckt);

		}
		else {
			_add_kc_ckt_to_solver(unrollediockt);
		}
		std::cout << "depth[" << dis.length() <<
			"]: key-condition circuit num gates: " << kc_ckt.nGates() << "\n";
		kc_ckt.write_bench("kc.bench");
	}


	if (kcbdd_option != 0) {
		id out = kcbdd.add_ckt_to_bdd(unrollediockt, fnct::AND);
		if (out == -1) {
			std::cout << "kbdd exceeded size limit. Disabling BDD tracking.\n";
			kcbdd_option = 0;
			neos_abort("exiting");
		}
		std::cout << "kbdd num nodes : " << kcbdd.num_nodes() << "\n";

		if (iter % kcbdd_restart_period == 0) {
			circuit bddckt;
			kcbdd.to_circuit(bddckt);
			_restart_from_ckt(bddckt);
		}
	}

	std::cout << "  dis: " << stats.dis_count << " stats -> ";
	mittbmc->BmcS->print_stats();

}

inline bool _has_lit(slit s, slitset& sset) {
	return (_is_in(s, sset) || _is_in(~s, sset));
}


void mcdec_int_ckt::_restart_bmc_engine() {

	// reinit BMC engine
	oidset frozens = mitt_ckt.keys();

	mittbmc = std::make_shared<BmcCkt>(mitt_ckt, frozens, bmc_config);

	// solve for one round to fill up the
	// mittbmc.lmaps with frozen literals
	mittbmc->bmc_solve(1, bmc_out);

	// add the termination_check link clauses
	_add_termination_links();
}

void mcdec_int_ckt::_restart_to_unrolled_mitt() {

	std::dynamic_pointer_cast<BmcCkt>(mittbmc)->restart_to_unrolled_ckt(mitt_cunr);
	term_initialized = false;

}

void mcdec_bmc_int::_restart_from_ckt(const circuit& cir) {

	std::cout << "restarting using ckt\n";

	_restart_to_unrolled_mitt();
	_add_termination_links();
	_add_kc_ckt_to_solver(cir);

}

/*
 * restart the SAT solver based on the kbdd condition
 */
void mcdec_bmc_int::_restart_from_bdd() {

	// get bdd circuit
	circuit kbdd_ckt;
	kcbdd.to_circuit(kbdd_ckt);

	_restart_from_ckt(kbdd_ckt);

}

// add key-condition-circuit to solver (single-ended circuit over key variables)
void mcdec_bmc_int::_add_kc_ckt_to_solver(const circuit& uncir) {

	assert(uncir.nOutputs() == 1);

	sat_solver& S = *mittbmc->BmcS;

	id2litmap_t wid2lit_map;

	foreach_wire(uncir, w, wid) {
		if (uncir.isKey(wid)) {
			id mittkid = mitt_ckt.find_wcheck(uncir.wname(wid));
			wid2lit_map[wid] = mittbmc->lmaps[0].at(mittkid);
		}
		else if (uncir.isConst(wid)) {
			wid2lit_map[wid] = uncir.is_const0(wid) ?
					S.false_lit():S.true_lit();
		}
		else if (uncir.isOutput(wid)) {
			slit p = S.create_new_var();
			wid2lit_map[wid] = p;
			S.add_clause(p);
		}
		else {
			wid2lit_map[wid] = S.create_new_var();
		}
	}
	add_ckt_gate_clauses(S, uncir, wid2lit_map);

}

// adds clauses to mitter that help connect NS wires for terminationa checking
void mcdec_bmc_int::_add_termination_links() {

	std::cout << "adding termination links\n";

	sat_solver& S = *mittbmc->BmcS;
	term_link = S.create_new_var(true);
	foreach_latch(mitt_ckt, dffid, dffout, dffin) {
		if (!_is_primed_name(mitt_ckt.wname(dffout))) {
			const std::string& oname = mitt_ckt.wname(dffout);
			//std::cout << "termination looking for wire " << oname << "\n";
			std::string oname_prime = _primed_name(oname);
			slit a, b;
			try {
				a = _get_mittlit(0, oname);
				b = _get_mittlit(0, oname_prime);
			}
			catch(std::out_of_range&) {
				//std::cout << "termination link missed " << oname << "\n";
				continue;
			}
			S.setFrozen(a); S.setFrozen(b);
			S.add_clause(a, ~b, ~term_link);
			S.add_clause(~a, b, ~term_link);
		}
	}

	term_initialized = true;
}

int mcdec_bmc_int::_check_termination_int() {

	if (!term_initialized) {
		_add_termination_links();
	}

	int term_frame = MIN(3, mittbmc->checkedBound);

	if (term_frame == 0)
		return 0;

	std::cout << "termination checking up to round: "
			<< term_frame << ". current BMC checked bound: " << mittbmc->checkedBound << "\n";

	slit termOutslit = _get_mittlit(term_frame, "mittNSXor");
	//slit termOutslit = _get_mittlit(MIN(5, mittbmc->checkedBound), "mittOut");

	vector<slit> assumps;

	assumps.push_back(termOutslit);
	assumps.push_back(term_link);

	auto tm = _start_wall_timer();
	if (!mittbmc->BmcS->solve(assumps)) {
		cout << "reached combinational termination!\n";
		stats.term_time += _stop_wall_timer(tm);
		return 1;
	}

	stats.term_time += _stop_wall_timer(tm);

	return 0;
}

void mcdec_bmc_int::_backbone_test() {

	cout << "performing backbone analysis " << std::endl;
	vector<slit> assumps;

	for (auto kid : enc_cir->keys()) {
		if (_is_not_in(kid, backbone_resolved_kids)) {
			slit k1slit = _get_keylit(kid, 0);
			slit k2slit = _get_keylit(kid, 1);

			if (!mittbmc->BmcS->solve(k1slit)) {
				cout << "backbone analysis resolved " << enc_cir->wname(kid) << " : 0\n";
				mittbmc->BmcS->add_clause(~k1slit);
				mittbmc->BmcS->add_clause(~k2slit);
				backbone_resolved_kids.insert(kid);
			}
			else if (!mittbmc->BmcS->solve(~k1slit)) {
				cout << "backbone analysis resolved " << enc_cir->wname(kid) << " : 1\n";
				mittbmc->BmcS->add_clause(k1slit);
				mittbmc->BmcS->add_clause(k2slit);
				backbone_resolved_kids.insert(kid);
			}
		}
	}
}

void mcdec_bmc::mcdec_bmc_stats::print() {

	std::cout << std::fixed;
	std::cout << "\nsat_time:  " << sat_time;
	std::cout << "\nterm_time: " << term_time;
	std::cout << "\ngen_time:  " << gen_time;
	std::cout << "\nbmc_time:  " << bmc_time;
	std::cout << "\nsimp_time: " << simp_time;
	std::cout << "\ntotal_time: " << total_time;

	std::cout << "\n\ndis_count: " << dis_count;
	std::cout << "\ndis_maxlen: " << dis_maxlen;

	if (ngcl_count)
		std::cout << "\n\nngcl_count: " << ngcl_count;

	std::cout << "\n\n";
}

void mcdec_bmc_int::extract_key_int() {

	stats.print();

	boolvec key(enc_cir->nKeys(), 0);
	if (mittbmc->BmcS->solve()) {
		for (auto kid : enc_cir->keys()) {
			bool kVal = getValue(*mittbmc->BmcS, _get_keylit(kid, 0));
			key[ewid2ind.at(kid)] = kVal;
		}
		cout << _COLOR_RED("key=");
		for (auto b : key)
			std::cout << _COLOR_RED(b);
		cout << "\n";

		check_key(key);
	}
	else {
		neos_error("constraints not solvable");
	}
}

void mcdec_bmc_int::get_last_key() {
	stats.print();
	interkey = last_dis.ks[0];
}



}
