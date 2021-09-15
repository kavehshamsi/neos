/*
 * dec.cpp
 *
 *  Created on: May 26, 2020
 *      Author: kaveh
 */

#include "dec/dec.h"
#include "sat/c_sat.h"
#include "base/circuit.h"
#include "utl/ext_cmd.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <main/main_util.h>

namespace dec_ns {

decol::decol(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corr_key) {
	init_circuits(sim_ckt, enc_ckt, corr_key, false);
}

void decol::init_circuits(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corr_key, bool oracle_bin) {

	//initialize ckt
	assert(enc_ckt.nWires());
	set_enc_cir(enc_ckt);
	if (sim_ckt.nWires()) {
		set_sim_cir(sim_ckt);
		create_enc2sim_map();
	}
	else {
		if (!oracle_bin) {
			set_corrkey(corr_key);
			gen_sim_cir_from_corrkey();
			create_enc2sim_map();
		}
		else {
			create_enc2sim_map(*enc_cir, *enc_cir, enc2sim);
		}
	}


}

void decol::gen_sim_cir_from_corrkey() {

	circuit scir = *enc_cir;
	id2boolHmap cmap;
	uint i = 0;
	for (auto kid : scir.keys()) {
		cmap[kid] = corrkey[i++];
	}
	scir.propagate_constants(cmap);
	// restore deleted input/outputs
	for (auto yid : enc_cir->allports()) {
		if (!enc_cir->isKey(yid) && scir.find_wire(enc_cir->wname(yid)) == -1) {
			scir.add_wire(enc_cir->wname(yid), enc_cir->wrtype(yid));
		}
	}
	sim_cir = std::make_shared<const circuit>(scir);

}

void decol::set_sim_cir(const circuit& sim_ckt) {

	sim_cir = std::make_shared<circuit>(sim_ckt);
	create_port_indexmap(*sim_cir, swid2ind);

	auto sim_cir_fb = sim_cir->find_feedback_arc_set();
	if (!sim_cir_fb.empty()) {
		std::cout << "original circuit is structurally cyclic.\n";
		sim_cyclic = true;
	}
}

void decol::set_corrkey(const boolvec& crrk) {

	corrkey = crrk;
	check_key_arg(*enc_cir, corrkey);

}

void decol::set_enc_cir(const circuit& enc_ckt) {

	enc_cir = std::make_shared<circuit>(enc_ckt);
	num_ins = enc_cir->nInputs();
	num_outs = enc_cir->nOutputs();
	num_keys = enc_cir->nKeys();
	num_ins_key = enc_cir->nAllins();
	ewid2ind.clear();
	enc2sim.clear();
	create_port_indexmap(*enc_cir, ewid2ind);

	if (enc_cir->nLatches()) {
		is_sequential = true;
	}

	if (sim_cir) {
		create_enc2sim_map();
	}

	feedback_arc_set = enc_cir->find_feedback_arc_set();
	if (!feedback_arc_set.empty()) {
		std::cout << "encrypted circuit has cycles\n";
		std::cout << "feedback arc set size: " << feedback_arc_set.size() << "\n";
		enc_cyclic = true;
		if (is_sequential) {
			neos_error("structurally cyclic sequential circuits not supported at this time");
		}
	}
	else {
		enc_cyclic = false;
	}

}

decol* c_sat_obj_ptr = nullptr;
int sigint_calls = 0;

void decol::_deadline_handler(int signum) {

	std::cout << "time out!\n";

	assert(c_sat_obj_ptr != nullptr);
	std::cout << "last key: " << utl::to_str(c_sat_obj_ptr->interkey) << "\n";
	c_sat_obj_ptr->get_last_key();
	//std::cout << "key-err: " << c_sat_obj_ptr->get_key_error(c_sat_obj_ptr->interkey) << "\n";
	c_sat_obj_ptr->check_key(c_sat_obj_ptr->interkey);

	exit(EXIT_FAILURE);

}

void decol::_sigint_handler(int signum, siginfo_t* u1, void* u2) {

	std::cout << "\ninterrupted\n";
	sigint_calls++;
	if (signum == SIGINT && sigint_calls == 1) {
		std::cout << "trying to extract last key\n";
		assert(c_sat_obj_ptr);
		std::cout << "last key: " << utl::to_str(c_sat_obj_ptr->interkey) << "\n";
		//c_sat_obj_ptr->get_last_key();
		// std::cout << "key-err: " << c_sat_obj_ptr->get_key_error(c_sat_obj_ptr->interkey) << "\n";
		c_sat_obj_ptr->check_key(c_sat_obj_ptr->interkey);

		exit(EXIT_FAILURE);
	}
	else if (signum == SIGINT && sigint_calls == 2) {
		std::cout << "exiting key extraction due to second interrupt\n";
		exit(EXIT_FAILURE);
	}

}

void decol::_block_signals() {
	signal(SIGINT, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
}

void decol::_unblock_signals() {
	_init_sigint_handler();
	signal(SIGALRM, _deadline_handler);
}

void decol::_safe_set_interkey(const boolvec& key) {
	_block_signals();
	interkey = key;
	_unblock_signals();
}

void decol::_init_handlers() {
	c_sat_obj_ptr = this;
	// setup sigint handler
	_init_sigint_handler();
	_init_deadline_handler();
}

void decol::_init_sigint_handler() {
	struct sigaction sa;
	sa.sa_flags = SA_NODEFER;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = &dec::_sigint_handler;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	   neos_error("problem setting up signlat handler");
}

void decol::_init_deadline_handler() {
	// setup deadline handler.
	if (time_deadline != -1) {
		signal(SIGALRM, _deadline_handler);
		alarm(time_deadline * 60);
	}
}

void decol::read_sim_enc_key_args(vector<std::string>& pos_args, bool bin_oracle) {

	if (pos_args.size() >= 2) {
		corrkey = mnf::parse_key_arg(pos_args[1]);
	}

	circuit sim_ckt, enc_ckt;
	if (  corrkey.empty() ) {
		if (use_verilog) {
			if (!bin_oracle) {
				sim_ckt.read_verilog(pos_args[0]);
				enc_ckt.read_verilog(pos_args[1]);
			}
			else {
				enc_ckt.read_verilog(pos_args[0]);
			}
		}
		else {
			if (!bin_oracle) {
				sim_ckt.read_bench(pos_args[0]);
				enc_ckt.read_bench(pos_args[1]);
			}
			else {
				enc_ckt.read_bench(pos_args[0]);
			}
		}
	}
	else {
		if (use_verilog) {
			enc_ckt.read_verilog(pos_args[0]);
		}
		else {
			enc_ckt.read_bench(pos_args[0]);
		}
	}

	if (bin_oracle) {
		pos_args = utl::subvec(pos_args, 1, pos_args.size()-1);
	}
	else {
		pos_args = utl::subvec(pos_args, 2, pos_args.size()-1);
	}

	init_circuits(sim_ckt, enc_ckt, corrkey, bin_oracle);

}



void decol::get_opt_parser(decol& dc, boost::program_options::options_description& op) {

	decol::append_opt_parser(dc, op);

}

void decol::append_opt_parser(decol& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("help,h", po::bool_switch(&dc.print_help), "help")
		("dec,d", po::value<string>(&dc.dec_method), "decryption/deobfuscation mode: <method> : decryption method")
		("use_verilog", po::value<string>(), "use verilog as input and output")
		("verbose,v", po::value<int>(&dc.verbose), "verbosity level")
		("to", po::value<int>(&dc.time_deadline), "set timeout in minutes")
		("mo", po::value<int>(), "set memory limit in MB")
		("pos_args",  po::value<vector<string>>(&dc.pos_args), "positional arguments");

}

void dec::get_opt_parser(dec& dc, boost::program_options::options_description& op) {

	decol::get_opt_parser((decol&)dc, op);
	dec::append_opt_parser(dc, op);

}


void dec::append_opt_parser(dec& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("bin_oracle", po::value<string>(&dc.oracle_binary), "use binary executable as oracel with format: oracle <binary_input> > <binary_output>");

}

void decol::create_port_indexmap(const circuit& cir, id2idHmap& port2indmap) {

	port2indmap.clear();
	uint i = 0;
	for (auto xid : cir.inputs()) port2indmap[xid] = i++;
	i = 0;
	for (auto kid : cir.keys()) port2indmap[kid] = i++;
	i = 0;
	for (auto oid : cir.outputs()) port2indmap[oid] = i++;

}

void decol::check_key_arg(const circuit& enc_cir, const boolvec& crrk) {

	if (crrk.size() != enc_cir.nKeys()) {
		std::cout << fmt("error: correct key arg size %1% mismatch with number of keys in enc_cir %2% ",
				crrk.size() % enc_cir.nKeys()) << "\n";
		exit(1);
	}

}

void decol::create_enc2sim_map(const circuit& simckt, const circuit& encckt,
		bimap<id, id>& enc2sim) {

	if ((encckt.nKeys()) == 0)
		neos_error("encrypted circuit has no key bits");

	for (auto& x : encckt.inputs()) {
		string enc_wirename = encckt.wname(x);
		id sim_wire = simckt.find_wire(enc_wirename);
		if ( sim_wire == -1) {
			neos_error("\nERROR: input " << enc_wirename
					<< " not found in sim-circuit\n");
		}
		else
			enc2sim.add_pair(x, sim_wire);
	}

	for (auto x : encckt.outputs()) {
		string enc_wirename = encckt.wname(x);

		id sim_wire = simckt.find_wire(enc_wirename);

		if ( sim_wire != -1 )
			enc2sim.add_pair(x, sim_wire);
		else {
			neos_error("output " << enc_wirename
					<< " not found in sim-circuit\n");
		}
	}

}

void decol::create_enc2sim_map(void) {

	create_enc2sim_map(*sim_cir, *enc_cir, enc2sim);

}

void decol::check_key(const circuit& enc_ckt, const boolvec& corr_key,
		const boolvec& hype_key, const id2boolHmap& constant_nodes) {

	circuit sim_ckt = enc_ckt;
	id2boolHmap cmap;

	uint i = 0;
	for (auto kid : enc_ckt.keys()) {
		cmap[kid] = corr_key[i++];
	}
	sim_ckt.propagate_constants(cmap);
	for (auto xid : enc_ckt.inputs()) {
		if (sim_ckt.find_wire(enc_ckt.wname(xid)) == -1) {
			sim_ckt.add_wire(enc_ckt.wname(xid), enc_ckt.wrtype(xid));
		}
	}
	dec::check_key(sim_ckt, enc_ckt, hype_key, constant_nodes);

}

boolvec dec::query_oracle(const boolvec& xvec) {

	boolvec yvec;

	if (!oracle_binary.empty()) {
		if (!utl::_file_exists(oracle_binary)) {
			neos_error("cannot open oracle executable " << oracle_binary);
		}

		if (oracle_subproc.stat == subproc_t::UNINIT) {
			if (!oracle_subproc.init(oracle_binary)) {
				neos_error("failed to initiate oracle process at path " << oracle_binary << "\n");
			}
		}

		string str;
		int i = 0;
		for (auto xid : enc_cir->inputs()) {
			//std::cout << enc_cir->wname(xid) << "\n";
			str += xvec[i++] ? '1':'0';
		}

		oracle_subproc.write_to_in(str);
		// std::cout << "wrote to output " << str << "\n";

		std::string retstr = oracle_subproc.read_line();
		// std::cout << "oracle binary output: " << retstr << "\n";

		for (auto b : retstr) {
			if (b == '0' || b == '1') {
				yvec.push_back(b == '1');
			}
		}
		//std::cout << "read oracle output as: " << utl::to_delstr(yvec) << "\n";
		/*for (auto yid : enc_cir->outputs()) {
			std::cout << enc_cir->wname(yid) << "\n";
		}*/

		if (yvec.size() != enc_cir->nOutputs()) {
			neos_error("length mismatch in output of oracle executable");
		}
	}
	else {
		id2boolmap smap;
		int i = 0;
		for (auto xid : enc_cir->inputs()) {
			smap[enc2sim.at(xid)] = xvec[i++];
		}
		sim_cir->simulate_comb(smap);
		for (auto yid : enc_cir->outputs()) {
			yvec.push_back(smap.at(enc2sim.at(yid)));
		}
	}

	return yvec;
}

vector<boolvec> dec::query_seq_oracle(vector<boolvec>& xvecs) {

	vector<boolvec> yvecs;

	if (!oracle_binary.empty()) {

		if (!oracle_subproc.init(oracle_binary)) {
			neos_error("could not initiate oracle subprocess at " << oracle_binary);
		}

		std::string str;
		for (uint i = 0; i < xvecs.size(); i++) {
			int j = 0;
			for (auto xid : enc_cir->inputs())
				str += xvecs[i][j++] ? '1':'0';
			str += " ";
		}

		oracle_subproc.write_to_in(str);

		string retstr = oracle_subproc.read_line();

		std::cout << "oraclae binary output: " << retstr << "\n";

		auto it = enc_cir->outputs().begin();

		boost::trim(retstr);
		std::stringstream ss(retstr);
		std::string line;
		while ( getline(ss, line, ' ') ) {
			yvecs.push_back(boolvec());
			for (auto b : line) {
				if (b == '0' || b == '1') {
					yvecs.back().push_back(b == '1');
					if (yvecs.back().size() > enc_cir->nOutputs()) {
						neos_error("length mismatch in output of oracle executable");
					}
				}
			}
		}
		if (yvecs.size() != xvecs.size()) {
			neos_error("depth mismatch in output of oracle " << yvecs.size() << " and input " << xvecs.size());
		}
	}
	else {
		vector<id2boolmap> smaps(xvecs.size());
		for (int i = 0; i < smaps.size(); i++) {
			int j = 0;
			for (auto xid : enc_cir->inputs()) {
				smaps[i][enc2sim.at(xid)] = xvecs[i][j++];
			}
		}
		sim_cir->simulate_seq(smaps);
		yvecs = vector<boolvec>(xvecs.size(), boolvec(num_outs));
		for (int i = 0; i < smaps.size(); i++) {
			int j = 0;
			for (auto yid : enc_cir->outputs()) {
				yvecs[i][j++] = smaps[i][enc2sim.at(yid)];
			}
		}
	}

	return yvecs;

}


void decol::check_key(const circuit& sim_ckt, const circuit& enc_ckt,
		const boolvec& key, const id2boolHmap& constant_nodes) {

	decol decobj(sim_ckt, enc_ckt, boolvec());
	decobj.check_key(key, constant_nodes);

}

int decol::check_key_seq(const boolvec& key, const id2boolHmap& constant_nodes) {

	circuit tmp_ckt = *enc_cir;

	if (key.size() != (uint)num_keys)
		neos_error("key lentgh does not match encrypted circuit key-length of "
				<< num_keys << "\n");

	if (!sim_cir && corrkey.empty()) {
		std::cout << "cannot verify key correctness without orignal circuit or correct key\n";
		return -1;
	}

	id2boolmap keyMap;
	uint i = 0;
	for ( auto kid : enc_cir->keys() ) {
		tmp_ckt.tie_to_constant(kid, key[i++]);
	}

	for (auto& kp : constant_nodes) {
		if (tmp_ckt.wire_exists(kp.first)) {
			auto op = tmp_ckt.open_wire(kp.first);
			tmp_ckt.tie_to_constant(op.second, kp.second);
		}
	}
	tmp_ckt.remove_dead_logic(false);


	std::cout << "checking key\n";

	if (ext::check_equivalence_abc(*sim_cir, tmp_ckt)) {
		std::cout << "\nequivalent\n";
		return 1;
	}
	else {
		std::cout << "\ndifferent\n";
		return 0;
	}

}

void decol::print_known_keys() {

	uint i = 0;
	std::cout << "currkey=";
	for (auto kid : enc_cir->keys()) {
		if (_is_in(kid, knownkeys)) {
			std::cout << knownkeys.at(kid);
		}
		else {
			std::cout << "x";
		}
		i++;
	}
	std::cout << "\n";

}


int decol::check_key(const boolvec& key, const id2boolHmap& constant_nodes) {

	using namespace sat;
	using namespace csat;

	if (key.size() != (uint)enc_cir->nKeys()) {
		std::cout << "key lentgh does not match encrypted circuit\n";
		exit(EXIT_FAILURE);
	}

	if (is_sequential) {
		return check_key_seq(key, constant_nodes);
	}
	else {
		return check_key_comb(key, constant_nodes);
	}

}


int decol::check_key_comb(const boolvec& key, const id2boolHmap& constant_nodes) {

	if ((!sim_cir || sim_cir->nWires() == 0) && corrkey.empty()) {
		std::cout << "cannot verify key correctness without orignal circuit or correct key\n";
		return -1;
	}

	assert(sim_cir);

	std::shared_ptr<circuit> scir = std::make_shared<circuit>(*sim_cir);

	assert(scir->nLatches() == 0 && enc_cir->nLatches() == 0);

	circuit tmp_ckt = *enc_cir;

	id2boolHmap keyMap;
	int i = 0;
	std::cout << "checking key ";
	for (auto kid : enc_cir->keys()) {
		//keyMap[kid] = key[i++];
		tmp_ckt.tie_to_constant(kid, key[i]);
		std::cout << key[i++];
	}
	std::cout << "\n";

	for (auto& kp : constant_nodes) {
		auto op = tmp_ckt.open_wire(kp.first);
		tmp_ckt.tie_to_constant(op.second, kp.second);
	}
	tmp_ckt.remove_dead_logic(false);

	foreach_inid(*scir, inid) {
		if (tmp_ckt.find_wire(scir->wname(inid)) == -1) {
			tmp_ckt.add_wire(scir->wname(inid), wtype::IN);
		}
	}

	using namespace csat;

	sat_solver Eq;
	id2litmap_t vmap;
	add_to_sat(Eq, tmp_ckt, vmap);

	/*
	if (!Eq.solve()) {
		std::cout << "circuit with key not satisfiable. Possible oscilation\n";
	}
	else {
		//std::cout << "\ncircuit satisfiable\n";
	}
	 */

	circuit cmp_ckt = tmp_ckt;
	cmp_ckt.add_circuit_byname(OPT_INP, *scir, "_$1");

	vector<idvec> outvecs(2);
	devide_ckt_ports(cmp_ckt, cmp_ckt.outputs(), outvecs, {"_$1"});

	id satend = build_comparator(outvecs[0], outvecs[1], cmp_ckt, "1");
	std::vector<slit> cmp_assumps;

	sat_solver S;
	id2litmap_t cmpmaps;

	add_ckt_to_sat_necessary(S, cmp_ckt, cmpmaps);
	int stat = S.solve_limited({cmpmaps.at(satend)}, 200000000);
	if (stat == sl_False) {
		std::cout << "\nequivalent\n";
		solve_status = 1;
		return true;
	}
	else {
		if (stat == sl_True) {
			std::cout << "\nincorrect key\n";
/*			id2boolmap simMap;
			assert(cmp_ckt.error_check());
			// cmp_ckt.write_bench();

			std::cout << "\nfor input=";
			for (auto inid : cmp_ckt.allins()) {
				simMap[inid] = S.get_value(cmpmaps.at(inid));
				std::cout << simMap.at(inid);
			}
			std::cout << "\n";

			cmp_ckt.simulate_comb(simMap);*/
		}
		else {
			std::cout << "could not verify key correctness due to exceeding SAT propgation budget\n";
		}

		if (!enc_cyclic) {
			dec decobj(*sim_cir, *enc_cir, corrkey);
			double key_error = decobj.get_key_error(key, true);
			std::cout << "\nkey-err: " << key_error << "\n";
		}
		else {
			std::cout << "not able to get key error for cyclic enc-circuit\n";
		}
		// assert(simMap.at(satend) == 1);
		return false;
	}
}

double decol::get_key_error(const boolvec& candkey, const boolvec& corrkey, bool per_output) {

	id2boolmap esimMap0, esimMap1;

	uint64_t hd = 0;
	std::map<id, uint64_t> hdmap;
	foreach_outid(*enc_cir, oid) {
		hdmap[oid] = 0;
	}

	uint num_enc_in_patt = 1000;
	bool full_scan = false;
	if (enc_cir->nInputs() < 10) {
		num_enc_in_patt = pow(2, enc_cir->nInputs());
		full_scan = true;
	}

	for (uint n = 0; n < num_enc_in_patt; n++) {
		// set inputs randomly
		int i = 0;
		for (auto xid : enc_cir->inputs()) {
			bool v = (full_scan) ? ((n >> (i++)) & 1) : (rand() % 2);
			esimMap1[xid] = esimMap0[xid] = v;
		}

		// set keys accordingly
		i = 0;
		for (auto kid : enc_cir->keys()) {
			esimMap0[kid] = candkey[i];
			esimMap1[kid] = corrkey[i];
			i++;
		}

		enc_cir->simulate_comb(esimMap0);
		enc_cir->simulate_comb(esimMap1);

		foreach_outid(*enc_cir, oid) {
			if ( esimMap0.at(oid) != esimMap1.at(oid) ) {
				hdmap[oid]++;
				hd++;
			}
		}
	}

	if (per_output) {
		foreach_outid(*enc_cir, oid) {
			std::cout << enc_cir->wname(oid) << ":" << (double)hdmap.at(oid) / num_enc_in_patt << ";  ";
		}
	}

	return ((double)hd / (double)(num_enc_in_patt * enc_cir->nOutputs()));
}

double dec::get_key_error(const boolvec& key, bool print_outs, int num_patts, int max_depth) {

	if (!is_sequential)
		return get_key_error_comb(key, print_outs, num_patts);
	else
		return get_key_error_seq(key, print_outs, num_patts, max_depth);
}

double dec::get_key_error_seq(const boolvec& key, bool print_outs, int num_patts, int max_depth) {

	std::vector<id2boolmap> enc_maps;

	uint64_t hd = 0;
	std::map<id, uint64_t> hdmap;
	for (auto oid : enc_cir->outputs()) hdmap[oid];

	assert(max_depth >= 0);

	uint64_t total_sims = 0;

	for (uint n = 0; n < num_patts; n++) {
		uint depth = (rand() % max_depth) + 1;
		enc_maps = std::vector<id2boolmap>(depth, id2boolmap());
		vector<boolvec> qxs(depth), qys;

		int i = 0;

		for (uint d = 0; d < depth; d++) {
			// set inputs randomly
			i = 0;
			for (auto xid : enc_cir->inputs()) {
				bool r = (rand() % 2 == 0);
				enc_maps[d][xid] = r;
				qxs[d].push_back(r);
			}

			// set keys accordingly
			int i = 0;
			for (auto kid : enc_cir->keys())
				enc_maps[d][kid] = key[i++];
		}

		total_sims += depth;

		enc_cir->simulate_seq(enc_maps);
		qys = query_seq_oracle(qxs);

		for (uint d = 0; d < depth; d++) {
			i = 0;
			for (auto oid : enc_cir->outputs()) {
				if ( enc_maps[d].at(oid) != qys[d][i++] ) {
					hdmap[oid]++;
					hd++;
				}
			}
		}
	}

	if (print_outs) {
		std::cout << "output-errors: ";
		foreach_outid(*enc_cir, oid) {
			std::cout << enc_cir->wname(oid) << " : " << (double)hdmap.at(oid) / total_sims << ";  ";
		}
		std::cout << "\n";
	}

	return ((double)hd / (double)(total_sims * enc_cir->nOutputs()));
}

double dec::get_key_error_comb(const boolvec& key, bool per_output, int num_patts) {

	id2boolmap esimMap, ssimMap;

	uint64_t hd = 0;
	std::map<id, uint64_t> hdmap;
	foreach_outid(*enc_cir, oid) {
		hdmap[oid] = 0;
	}

	bool full_scan = false;
	if (enc_cir->nInputs() < 10) {
		num_patts = pow(2, enc_cir->nInputs());
		full_scan = true;
	}

	for (uint n = 0; n < num_patts; n++) {
		boolvec qx, qy;
		// set inputs randomly
		if (full_scan) {
			int xindex = 0;
			for (auto xid : enc_cir->inputs()) {
				bool v = (n >> (xindex++)) & 1;
				esimMap[xid] = v;
				qx.push_back(v);
			}
		}
		else {
			for (auto xid : enc_cir->inputs()) {
				bool r = (rand() % 2 == 0);
				esimMap[xid] = r;
				qx.push_back(r);
			}
		}

		// set keys accordingly
		int i = 0;
		for (auto kid : enc_cir->keys())
			esimMap[kid] = key[i++];

		enc_cir->simulate_comb(esimMap);
		qy = query_oracle(qx);

		i = 0;
		for (auto oid : enc_cir->outputs()) {
			if ( esimMap.at(oid) != qy.at(i++) ) {
				hdmap[oid]++;
				hd++;
			}
		}
	}

	if (per_output) {
		foreach_outid(*enc_cir, oid) {
			std::cout << enc_cir->wname(oid) << ":" << (double)hdmap.at(oid) / num_patts << ";  ";
		}
	}

	return ((double)hd / (double)(num_patts * enc_cir->nOutputs()));
}


} // dec_ns


