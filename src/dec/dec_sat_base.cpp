
#include "dec/dec_sat.h"
#include "opt/opt.h"
#include "main/main_util.h"

namespace dec_ns {

using namespace utl;

dec_sat::dec_sat(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corrkey) :
	dec(sim_cir, enc_cir, corrkey) {

	// is this necessary?
	assump_vs_clause = false;
	Fi.set_simp(use_satsimp);

}

void dec_sat::get_opt_parser(dec_sat& dc, boost::program_options::options_description& op) {

	dec::get_opt_parser((dec&)dc, op);
	dec_sat::append_opt_parser(dc, op);

}

void dec_sat::append_opt_parser(dec_sat& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("use_aig", po::bool_switch(&(dc.use_aig)), "Use AIG for key-conditions")
		("kc_sweep", po::value<int>(&(dc.kc_sweep)), "key-condition sweeping: {0:(default)None, "
				"1:constant-propagation (cp), 2:cp+sweep, 3:cp+abc-simp}")
		("use_satsimp", po::bool_switch(&(dc.use_satsimp)), "Use Glucouse SAT-solver simplification")
		("settled_keys", po::value<int>(&(dc.settled_keys_detect)), "find settled key bits every kc_sweep_period")
		("settled_keys_period", po::value<int>(&(dc.settled_keys_period)), "find settled key bits every kc_sweep_period")
		("kc_sweep_period", po::value<int>(&(dc.kc_sweep_period)), "key-condition sweeping period")
		("prop_limit", po::value<int>(&(dc.propagate_limit)), "propagation limit in SAT-sweeping")
		("kc_bdd", po::value<int>(&(dc.kc_bdd)), "BDD based key-condition compression")
		("kbdd_size_limit", po::value<int>(&(dc.kc_bdd_size_limit)), "BDD size limit in kc_condition");

}

void dec_sat::print_usage_message() {
	boost::program_options::options_description op;
	dec_sat::get_opt_parser(*this, op);
	std::cout << op << "\n";
}

int dec_sat::parse_args(int argc, char** argv) {

	namespace po = boost::program_options;
	po::options_description op;
	dec_sat::get_opt_parser(*this, op);

	std::cout << "args " << utl::to_delstr(pos_args, " ") << "\n";

	if (mnf::parse_args(argc, argv, op) || print_help) {
		print_usage_message();
		return 1;
	}

	decol::read_sim_enc_key_args(pos_args, !oracle_binary.empty());
	//enc_cir->write_bench();

	if (pos_args.size() >= 1) {
    	iteration_limit = stoi(pos_args[0]);
	}

    return 0;
}

void dec_sat::build_iovec_ckt(const circuit& cir) {

	iovecckt = cir;
	auto outs = iovecckt.outputs();
	uint i = 0;
	idvec io_outs;
	for (auto oid : outs) {
		id io_in = iovecckt.add_wire(_iocir_outname(iovecckt.wname(oid)), wtype::IN);
		id io_out = iovecckt.add_wire(wtype::OUT);
		io_outs.push_back(io_out);
		iovecckt.add_gate({oid, io_in}, io_out, fnct::XNOR);
		iovecckt.setwiretype(oid, wtype::INTER);
		i++;
	}
	iovecckt.join_outputs(io_outs, fnct::AND, "iovec_out");

	//iovecckt.write_bench();

}

void dec_sat::build_mitter(const circuit& cir) {

	// cleanup before
	mitt_ckt = circuit();
	mitter_assumps.clear();
	mitt_maps.clear();

	mitt_out = build_diff_ckt(cir, cir, mitt_ckt);

	// Add the duplckt to the SAT
	add_to_sat(Fi, mitt_ckt, mitt_maps);

	// duplicate-circuit-key-names to id mapping
	mitt_keyid2lit_maps = std::vector<id2litmap_t>(2, id2litmap_t());

	for (auto x : mitt_ckt.keys()){
		// std::cout << mitt_ckt.wname(x) << "\n";
		string kname = mitt_ckt.wname(x);
		if ( _is_in("_$1", kname) ) {
			id found = cir.find_wire(kname.substr(0, kname.length() - 3));
			assert(found != -1);
			mitt_keyid2lit_maps[1][found] = mitt_maps.at(x);
		}
		else {
			id found = cir.find_wire(kname);
			assert(found != -1);
			mitt_keyid2lit_maps[0][found] = mitt_maps.at(x);
		}
		//std::cout << x/cir.getnumber_of_wires() << " "
			//<< y << " " << wname(mitt_ckt, x) << "\n";
		mitt_keynames_map[mitt_ckt.wname(x)] = x;
	}

	mitt_ckt.setwirename(mitt_out, "mitt_out");

	if (!enc_cyclic)
		mitt_aig = aig_t(mitt_ckt);

	mitt_tip = mitt_maps.at(mitt_out);
	io_tip = Fi.true_lit();

	return;
}

int dec_sat::solve_for_dip(iopair_t& dp) {

	slitvec assumps = {mitt_tip, io_tip};
	utl::push_all(assumps, Fi_assumps);
	int stat = Fi.solve(assumps);
	// std::cout << "stat is " << stat << "\n";

	if (stat == 0)
		return 0;

	//std::cout << "disc-in: ";
	dp.x.clear();
	for ( auto xid : enc_cir->inputs() ) {
		dp.x.push_back( Fi.get_value(_get_mitt_lit(xid)) );
		// std::cout << disc_input[enc2sim_inputmap[x.first]];
	}

	return 1;

}

// some methods for external easy call
double dec_sat::solve_exact(const circuit& sim_ckt, const circuit& enc_ckt){

	clock_t cpu_start = utl::_start_cpu_timer();
	dec_sat dec_obj(sim_ckt, enc_ckt, boolvec());
	dec_obj.solve();
	return utl::_stop_cpu_timer(cpu_start);

}




void dec_sat::_prepare_sat_attack() {

	// cleanup sat solver
	Fi.clear();

	//duplicate circuit for DI
	build_mitter(*enc_cir);
	build_iovec_ckt(*enc_cir);

	// runs in O(n) for circuits with tribufs
	_add_no_float_condition();

	if (enc_cyclic)
		_build_anticyc_condition();

	// some randomization for better simulation resutls
	// Fi.reseed_solver();

	sl2name[io_tip] = "io_tip";
	sl2name[Fi.true_lit()] = "vdd";
	sl2name[Fi.false_lit()] = "gnd";

	keylits.clear();
	for (auto kc : {0, 1}) {
		keylits.push_back(slitvec());
		for (auto kid : enc_cir->keys()) {
			keylits[kc].push_back( _get_mitt_lit(enc_cir->wname(kid), kc) );
		}
	}


	add_knownkeys_to_solver();

}

void dec_sat::solve() {

	_init_handlers();
	_prepare_sat_attack();

	//SAT attack Loop
	while ( true ) {

		iopair_t dp;
		int stat = solve_for_dip(dp);

		if (stat == 0) {
			std::cout << "no more dips.\n";
			break;
		}

		iteration++;
		print_stats(iteration);

		dp.y = query_oracle(dp.x);

		if (collect_disagreements)
			record_wdisagreements();

		get_inter_key();

		//Record observations in SAT
		create_ioconstraint(dp, iovecckt);

		if (settled_keys_detect != 0 && iteration % settled_keys_period == 0) {
			if (find_settled_keys() == 1)
				break;
		}

		// check iteration cap
		if (iteration_limit != -1
				&& iteration >= iteration_limit)
			break;
	}

	//Extract key from SAT
	solve_key();
	return;
}

void dec_sat::record_wdisagreements() {
	// assumes Fi has already been solved with a DIP
	id2boolmap smap0, smap1;
	for (auto xid : enc_cir->inputs()) {
		smap0[xid] = smap1[xid] = Fi.get_value(_get_mitt_lit(xid));
	}
	for (auto kid : enc_cir->keys()) {
		smap0[kid] = Fi.get_value(_get_mitt_lit(kid, 0));
		smap1[kid] = Fi.get_value(_get_mitt_lit(kid, 1));
	}

	enc_cir->simulate_comb(smap0);
	enc_cir->simulate_comb(smap1);

	foreach_wire(*enc_cir, w, wid) {
		wdisagreement_map[wid] += (smap0.at(wid) != smap1.at(wid));
	}

	if (iteration % 5 == 0) {
		for (auto wid : enc_cir->get_topsort_wires()) {
			std::cout << "wdiss: " << enc_cir->wname(wid) << " -> " << wdisagreement_map.at(wid) << "\n";
		}
	}

}

void dec_sat::print_stats(int iter, sat_solver& S) {
	std::cout << "iteration: " << std::setw(4) << iter << "; ";
	S.print_stats();
}

void dec_sat::print_stats(int iter) {
	print_stats(iter, Fi);
}


void dec_sat::get_inter_key() {

	id2boolmap testmap;
	//std::cout << "key0=";
	assert(Fi.solve(io_tip));

	interkey.clear();
	for ( auto kid : enc_cir->keys() ){
		bool kval = Fi.get_value(_get_mitt_lit(enc_cir->wname(kid)));
		// std::cout << kval;
		interkey.push_back( kval );
	}
}

/*
 * takes condition that is already input-propagated
 * adds output comparators and AND gate
 * then adds to BDD
 */
void dec_sat::add_ioconstraint_to_bdd(const iopair_t& dp) {

	circuit kc_cir;
	_get_kc_ckt(dp, kc_cir, iovecckt);

	if (!kbdd) {
		kbdd = std::make_unique<circuit_bdd>(kc_cir);
	}
	else {
		kbdd->add_ckt_to_bdd(kc_cir, fnct::AND);
		std::cout << kbdd->get_stats() << "\n";
	}

}

slit dec_sat::create_variable(sat_solver& S, id2litmap_t& varmap, id wireid) {
	slit p = S.create_new_var();
	varmap[wireid] = p;
	return p;
}

void dec_sat::create_ioconstraint(const iopair_t& dp, const circuit& cir, int num_key_vecs) {

	if (kc_sweep == 0) {
		create_ioconstraint(dp, cir, Fi, num_key_vecs, io_tip);
	}
	else {
		create_ioconstraint_ckt(dp, cir);
	}
}

void dec_sat::_add_kc_ckt_to_solver(const circuit& kc_cir) {

	if (!kc_cir.nOutputs() == 1) {
		kc_cir.write_bench();
		neos_error("single out kc needed");
	}

	for (int kc : {0, 1}) {

		id2litmap_t vmap;
		for (auto kid : kc_cir.keys()) {
			vmap[kid] = _get_mitt_lit(kc_cir.wname(kid), kc);
		}

		id kc_out = *kc_cir.outputs().begin();
		slit kc_outlit = vmap[kc_out] = Fi.create_new_var();

		add_ckt_to_sat_necessary(Fi, kc_cir, vmap);

		slit new_out = Fi.create_new_var();
		add_logic_clause(Fi, {io_tip, kc_outlit}, new_out, fnct::AND);

		io_tip = new_out;

	}

}

void dec_sat::_restart_from_ckt(const circuit& cir) {
	_prepare_sat_attack();
	_add_kc_ckt_to_solver(cir);
}

void dec_sat::create_ioconstraint_ckt(const iopair_t& dp, const circuit& cir) {

	circuit cur_kc;
	_get_kc_ckt(dp, cur_kc, cir);

	if (kc_sweep == 1) {
		_add_kc_ckt_to_solver(cur_kc);
	}
	else if (kc_sweep > 1) {

		kc_ckt.add_circuit_byname(cur_kc, "");
		kc_out = kc_ckt.join_outputs(fnct::AND, "kc_out");

		assert(kc_ckt.error_check());

		if (iteration % kc_sweep_period == 0) {
			if (kc_sweep == 2) { // SAT sweep
				opt_ns::SATsweep::sweep_params_t param;
				param.prop_limit = propagate_limit;
				opt_ns::circuit_satsweep(kc_ckt, param);
			}
			else if (kc_sweep == 3) { // ABC simplify
				ext::abc_simplify(kc_ckt, true);
			}
			assert(kc_ckt.error_check());
			std::cout << "kckt num gates: " << kc_ckt.nGates() << "\n";
			_restart_from_ckt(kc_ckt);
		}
		else {
			_add_kc_ckt_to_solver(cur_kc);
		}

	}

}

void dec_sat::_get_kc_ckt_mult(const iopair_t& dp, circuit& kc_cir, const circuit& iocir, int num_copies) {
	_get_kc_ckt(dp, kc_cir, iocir);

	for (int i = 1; i < num_copies; i++) {
		id2idmap conMap;
		kc_cir.add_circuit(kc_cir, conMap, "_$" + std::to_string(i));
	}

	kc_cir.join_outputs(fnct::AND, "kcout");
}

void dec_sat::_get_kc_ckt(const iopair_t& dp, circuit& kc_cir, const circuit& iocir) {

	kc_cir = iocir;

	id2boolHmap consts_map;
	foreach_inaid(iocir, xid) {
		const auto& x = iocir.getcwire(xid);
		std::string xnm = iocir.wname(xid);
		if (x.isInput()) {
			const auto& xnm = iocir.wname(xid);

			id simid;
			if (_is_iocir_outname(xnm)) {
				std::string nm = _uniocir_outname(xnm);
				id ewid = enc_cir->find_wcheck(nm);
				consts_map[xid] = dp.y.at(ewid2ind.at(ewid));
			}
			else {
				id ewid = enc_cir->find_wcheck(xnm);
				consts_map[xid] = dp.x.at(ewid2ind.at(ewid));
			}
		}
	}

	kc_cir.propagate_constants(consts_map);

}

void dec_sat::create_ioconstraint(
		const iopair_t& dp, const circuit& cir,
		sat_solver& S, int num_key_vecs, slit& io_tip_slit) {


	//add circuit with Xids but mask keys
	std::vector<slit> ioends;

	for (id kc = 0; kc < num_key_vecs; kc++) {

		id2litmap_t	iovec_maps;

		foreach_wire(cir, w, wid) {
			if (cir.isConst(wid)) {}
			else if (w.isKey()) {
				// id keywire =
					//	mitt_keynames_map[cir.getwire_names()[x.wireid]];
				iovec_maps[wid] = _get_mitt_lit(cir.wname(wid), kc); // keymaps[kvc].at(enc_ckt->find_wire);
			}
			else if (w.isInput()) {
				const auto& xnm = cir.wname(wid);
				if (_is_iocir_outname(xnm)) {
					std::string nm = _uniocir_outname(xnm);
					// std::cout << "pre " << xnm << "\n";
					// std::cout << "to  " << nm << "\n";
					id eyid = enc_cir->find_wcheck(nm);
					iovec_maps[wid] = dp.y.at(ewid2ind.at(eyid)) ? S.true_lit():S.false_lit();
				}
				else {
					// std::cout << "at " << xnm << "\n";
					id exid = enc_cir->find_wcheck(xnm);
					iovec_maps[wid] = dp.x.at(ewid2ind.at(exid)) ? S.true_lit():S.false_lit();
				}
			} else if (w.isOutput()) {
				ioends.push_back(create_variable(S, iovec_maps, wid));
			}
		}

		//now add the circuit gate clauses to Fi with the map we just made
		add_ckt_to_sat_necessary(S, iovecckt, iovec_maps);
	}

	slit ioends_out = S.create_new_var();
	add_logic_clause(S, ioends, ioends_out, fnct::AND);
	slit new_io_tip = S.create_new_var();
	add_logic_clause(S, {ioends_out, io_tip_slit}, new_io_tip, fnct::AND);
	io_tip_slit = new_io_tip;

	//write to file for debugging
	//write_to_file(Fi, Fi_assump);

	return;
}


void dec_sat::solve_key() {

	boolvec key;

	if (iteration == 0) {
		std::cout << "did not get to iterate!\n";
	}

	extract_key(key);
	//assert(is_combinational(*enc_ckt, keymap));

	if (oracle_binary.empty()) {
		check_key(key);
	}
	else {
		std::cout << "skipping equivalence checking with binary oracle\n";
	}
	interkey = key;
	std::cout << "\n";
}

void dec_sat::get_last_key() {
	// find_settled_keys();
}


void dec_sat::extract_key(boolvec& key) {

	key.clear();
	id2boolHmap keymap;

	if ( !Fi.solve(precond_assumps, io_tip) ) {
		std::cout << "constraints are not satisfiable";
	}
	std::cout << "\nFinished! \niteration=" << iteration;
	std::cout << "\nkey=";

	for ( auto kid : enc_cir->keys() ) {
		slit kl = _get_mitt_lit(enc_cir->wname(kid), 0);
		bool kv = Fi.get_value(kl);
		key.push_back(kv);
		keymap[kid] = kv;
	}

	for (auto b : key)
		std::cout << b;
	std::cout << "\n";

}

/* under the key-condition represented by io_tip mitter by mitt_lit and solver Fi
 * find settled keys for enc_cir */
int dec_sat::find_settled_keys(int force_level) {


	auto tm = utl::_start_wall_timer();

	std::cout << "looking for settled keys\n";

	if (settled_keys_detect == 1 || force_level == 1) {
		find_settled_keys_1();
	}
	else if (settled_keys_detect == 2 || force_level == 1) {
		find_settled_keys_2();
	}

	print_known_keys();

	std::cout << "settled key detection time: " << utl::_stop_wall_timer(tm) << "\n";
	if (knownkeys.size() == num_keys) {
		std::cout << "all keys settled\n";
		return 1;
	}
	return 0;

}

void dec_sat::find_settled_keys_1() {

	sat_solver Sk = Fi;

	id2litmap_t vmap[2];
	for (auto kc : {0, 1}) {
		for (auto kid : enc_cir->keys()) {
			vmap[kc][kid] = _get_mitt_lit(kid, kc);
		}
		for (auto xid : enc_cir->inputs()) {
			vmap[kc][xid] = (kc == 0) ? Sk.create_new_var() : vmap[0].at(xid);
		}
		add_ckt_to_sat_necessary(Sk, *enc_cir, vmap[kc]);
	}

	idset disqualified;

	for (auto wid : enc_cir->get_topsort_wires()) {

		if (enc_cir->isInput(wid) || enc_cir->isKey(wid) || _is_in(wid, knowncones) || _is_in(wid, disqualified))
			continue;

		slit m0 = vmap[0].at(wid);
		slit m1 = vmap[1].at(wid);

		int ret = opt_ns::SATsweep::_test_equivalence_sat(m0, m1, Sk, false, {io_tip}, 1000000000, false);
		// std::cout << "at " << enc_cir->wname(wid) << " " << ret << "\n";

		if (ret == sl_True) {
			assert(Sk.solve({io_tip}));
			knowncones.insert(wid);
			id gid = enc_cir->wfanin0(wid);
			idset mffcgids;
			enc_cir->get_gate_mffc(gid, mffcgids);
			for (auto gid : mffcgids) {
				const auto& g = enc_cir->getcgate(gid);
				id mffcwid = g.gfanout();
				// std::cout << "settled wire " << enc_cir->wname(mffcwid) << "\n";
				knowncones.insert(mffcwid);
				for (auto xid : g.fanins) {
					if (enc_cir->isKey(xid)) {
						bool kv = Sk.get_value(vmap[0].at(xid));
						knownkeys[xid] = kv;
						slit k0 = _get_mitt_lit(xid, 0);
						slit k1 = _get_mitt_lit(xid, 1);
						Fi.add_clause(kv ? k0 : ~k0);
						Fi.add_clause(kv ? k1 : ~k1);
						Sk.add_clause(kv ? k0 : ~k0);
						Sk.add_clause(kv ? k1 : ~k1);
						std::cout << "settled key " << enc_cir->wname(mffcwid) << " -> " << kv << "\n";
					}
				}
			}
		}
		else if (ret == sl_False) { // cex simulate
			id2boolmap smap1, smap2;
			for (auto xid : enc_cir->inputs()) {
				smap1[xid] = smap2[xid] = Sk.get_value(vmap[0].at(xid));
			}
			for (auto kid : enc_cir->keys()) {
				smap2[kid] = Sk.get_value(vmap[0].at(kid));
				smap1[kid] = Sk.get_value(vmap[1].at(kid));
			}
			enc_cir->simulate_comb(smap1);
			enc_cir->simulate_comb(smap2);
			assert(smap1.at(wid) != smap2.at(wid));
			for (auto wid2 : enc_cir->get_topsort_wires()) {
				if ( (enc_cir->isInter(wid2) || enc_cir->isOutput(wid2)) && smap1.at(wid2) != smap2.at(wid2) ) {
					disqualified.insert(wid2);
				}
			}
		}
	}

}

void dec_sat::find_settled_keys_2() {

	sat_solver Sk = Fi;

	id2idmap added2new;
	circuit stmitt = *enc_cir;
	for (auto xid : enc_cir->inputs()) added2new[xid] = xid;
	stmitt.add_circuit(*enc_cir, added2new, "_$1");
	stmitt.write_bench();
	id2litmap_t vmap;
	for (auto kid : stmitt.keys()) {
		std::cout << stmitt.wname(kid) << "\n";
		vmap[kid] = _get_mitt_lit(stmitt.wname(kid), 0);
	}

	using namespace opt_ns;
	SATsweep::sweep_params_t sparam;
	sparam.set_external_solver(&Sk);
	sparam.vmap = vmap;
	SATCircuitSweep ssw(stmitt, sparam);
	ssw.analyze();

	for (auto wid : enc_cir->get_topsort_wires()) {

		if (enc_cir->isInput(wid) || enc_cir->isKey(wid) || _is_in(wid, knowncones))
			continue;

		id m0 = wid;
		id m1 = added2new.at(wid);
		std::cout << "checking " << stmitt.wname(m0) << " ? " << stmitt.wname(m1) << "\n";
		int ret = sl_False;
		if (ssw.id2class.at(m0) == ssw.id2class.at(m1)) {
			std::cout << "became equal\n";
			ret = sl_True;
		}

		if (ret == sl_True) {
			assert(Sk.solve({io_tip}));
			knowncones.insert(wid);
		}
	}

	knowncones_to_knownkeys();

}

void dec_sat::knowncones_to_knownkeys() {

	if (knowncones.empty()) {
		Fi.solve(io_tip);
	}

	for (auto wid : knowncones) {
		id gid = enc_cir->wfanin0(wid);
		idset mffcgids;
		enc_cir->get_gate_mffc(gid, mffcgids);
		for (auto gid : mffcgids) {
			const auto& g = enc_cir->getcgate(gid);
			id mffcwid = g.gfanout();
			// std::cout << "settled wire " << enc_cir->wname(mffcwid) << "\n";
			knowncones.insert(mffcwid);
			for (auto xid : g.fanins) {
				if (enc_cir->isKey(xid) && _is_not_in(xid, knownkeys)) {
					bool kv = Fi.get_value(_get_mitt_lit(xid, 0));
					knownkeys[xid] = kv;
					//std::cout << "settled key " << enc_cir->wname(mffcwid) << " -> " << kv << "\n";
				}
			}
		}
	}

}

void dec_sat::add_knownkeys_to_solver() {

	for (auto& p : knownkeys) {
		if (!enc_cir->wire_exists(p.first))
			continue;
		slit k0 = _get_mitt_lit(p.first, 0);
		slit k1 = _get_mitt_lit(p.first, 1);
		Fi.add_clause(p.second ? k0 : ~k0);
		Fi.add_clause(p.second ? k1 : ~k1);
	}

}


void dec_sat::add_atpg_iovecs(uint numvecs) {

	std::vector<boolvec> all_vecs =
			ext::atalanta(*enc_cir);

	id2boolmap simmap;

	int i = 0;
	for (auto& vec : all_vecs) {
		if (i++ == numvecs) break;
		int b = 0;
		iopair_t dp;
		for (auto xid : enc_cir->inputs()) {
			dp.x.push_back(vec[b++]);
			// std::cout << enc_ckt->getwire_name(x) << "\n";
		}
		dp.y = query_oracle(dp.x);
		create_ioconstraint(dp, iovecckt);
	}

}

// #define TERMDEBUG
bool dec_sat::_build_terminate_condition() {

	id satends[7]; // o1, o2, and o3
	circuit unit_ckt;

	satends[0] = build_diff_ckt(*enc_cir, *enc_cir, unit_ckt);
	// std::cout << unit_ckt.getwire_name(satends[0]) << "\n";
	// unit_ckt.write_verilog();

	// getting the 2 subkeys
	keyvecs = vector<idvec>(2);
	devide_ckt_ports(unit_ckt, unit_ckt.keys(), keyvecs, {"_$1"});

	term_ckt = unit_ckt;

	term_ckt.add_circuit(OPT_KEY, unit_ckt, "$2");
	satends[1] = term_ckt.find_wire("SAT_end_0_$2");
	// term_ckt.write_verilog();

	term_ckt.add_circuit(OPT_KEY, unit_ckt, "$3");
	satends[2] = term_ckt.find_wire("SAT_end_0_$3");

	invecs = vector<idvec>(3);
	devide_ckt_ports(term_ckt, term_ckt.inputs(), invecs, {"_$2", "_$3"});

	satends[3] = inequality_comparator(invecs[0], invecs[1], term_ckt, "4");
	satends[4] = inequality_comparator(invecs[1], invecs[2], term_ckt, "5");
	satends[5] = inequality_comparator(invecs[0], invecs[2], term_ckt, "6");
	satends[6] = inequality_comparator(keyvecs[0], keyvecs[1], term_ckt, "7");

	// term_ckt.write_verilog();

	foreach_wire(term_ckt, w, wid) {
		if ( w.isKey() ){
			id keywire = mitt_keynames_map[term_ckt.wname(wid)];
			term_maps[wid] = mitt_maps.at(keywire);
		}
		else {
			create_variable(Fi, term_maps, wid);
		}
	}

	//now add the circuit gate clauses to Fi with the map we just made
	foreach_gate(term_ckt, g, gid) {
		add_gate_clause(Fi, g, term_maps);
	}

	for (auto& x : satends) {
		// std::cout << term_ckt.getwire_name(x) << "\n";
		term_assumps.push_back(term_maps[x]);
	}

	//	_push_all(Fi_assump, term_assumps);
	//	Fi.solve(Fi_assump);
	//
	//	print_varmap(Fi, term_ckt, term_maps);

	//	exit(0);

	return true;
}

/*
 * add a no_float condition for nets that are
 * driven by only key-controlled tri-state buffers
 */
void dec_sat::_add_no_float_condition() {

	foreach_wire(*enc_cir, w, wid) {
		id non_tbuf_gates = 0, tbuf_gates = 0;
		std::vector<slit> slitvec1, slitvec2;
		for (auto& gid : enc_cir->wfanins(wid)) {
			if (enc_cir->getcgate(gid).gfun() != fnct::TBUF) {
				non_tbuf_gates++;
				break;
			}
			else {
				tbuf_gates++;
				id in = enc_cir->getcgate(gid).fanins[0];
				slitvec1.push_back(mitt_keyid2lit_maps[0].at(in));
				slitvec2.push_back(mitt_keyid2lit_maps[1].at(in));
			}
		}

		if (tbuf_gates > 0 && non_tbuf_gates == 0) {
			Fi.add_clause(slitvec1);
			Fi.add_clause(slitvec2);
		}
	}
}


void dec_sat::write_to_file(sat_solver& S, std::vector<slit>& assumps){
	std::string DUMP_FILE = "dump/cnfdpl.txt";
	S.write_dimacs(DUMP_FILE, assumps);
}

void dec_sat::key_bit_analysis(circuit& ckt){

	id2boolHmap in_map;
	id2boolHmap key_map;
	id2boolHmap out_map1,out_map2;
	std::vector<uint64_t> keyflips(ckt.keys().size(),0);

	id2litmap_t cktmaps;
	sat_solver CktSat;
	add_to_sat(CktSat, ckt, cktmaps);

	std::srand((int)clock());

	for (auto& x : ckt.inputs()){
		in_map[x] = false;
	}
	for (auto& x : ckt.keys()){
		key_map[x] = false;
	}
	for (auto& x : ckt.outputs()){
		out_map1[x] = false;
		out_map2[x] = false;
	}

	uint key_bit = 0;
	for (auto& x : key_map){
		std::cout << "got here\n";
		uint64_t key_trials = 0;
		while ((key_trials++) < 10) {
			_fill_rand(key_map);

			uint64_t input_trials = 0;
			while ((input_trials++) < 10) {
				_fill_rand(in_map);
				x.second = false;
				eval_ckt(CktSat, in_map, key_map, out_map1, cktmaps);
				x.second = true;
				eval_ckt(CktSat, in_map, key_map, out_map2, cktmaps);
				for (auto& y : out_map1){
					if (y.second != out_map2[y.first]) keyflips[key_bit]++;
				}
			}
		}
		key_bit++;
	}
	uint i = 0;
	for (auto& x : ckt.keys()){
		std::cout << ckt.wname(x) << " : "
				<< keyflips[i++] << "\n";
	}
}

bool dec_sat::eval_ckt(sat_solver& Ckt_Sat, id2boolHmap& in_map,
		id2boolHmap& key_map, id2boolHmap& out_map, id2litmap_t& var_mp){

	std::vector<slit> assump;

	for (auto& x : in_map) {
		assump.push_back((x.second) ?
				var_mp[x.first]:~var_mp[x.first]);
	}
	for (auto& x : key_map) {
		assump.push_back((x.second) ?
				var_mp[x.first]:~var_mp[x.first]);
	}

	if (!Ckt_Sat.solve(assump)) {
		std::cout << "external simulation not solvable!\n";
		return false;
	}

	for (auto&& x : var_mp) {
		out_map[x.first] = Ckt_Sat.get_value(x.second);
		//std::cout << outputs[i.first];
	}

	return true;
}


} // namespace c_sat

