
#include "main.h"
#include "enc/enc.h"

namespace mnf {

using namespace enc;

int enc_main(const main_data& mn) {

	bool failed = false;
	
	if ( mn.help ) {
		enc_manager_t enc_mgr;
		std::cout << "usage: neos -e <lock_method> <input_bench> <output_bench> [args]\n";
		for (auto& p : enc_mgr.encryptor_map) {
			std::cout << "   " << std::left << std::setw(14) << p.first << " : " << p.second.description << "\n";
		}
		return 0;
	}

	if (mn.pos_args.size() < 2) {
		neos_error("usage: neos -e <method> <input file> <output file>\n"
				"try neos -e -h for help");
	}

	std::string enc_ckt_name = _remove_extention(
			_base_name( mn.pos_args[0] ) );

	srand(static_cast<uint>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

	/*
	std::cout << "Encryption module invoked \n";
	std::cout << "encvalue : " << mn.encvalue << "\n";
	std::cout << " arg[optind] " << pos_args[0] // sim file
			<< "\n arg[optind+1] " << pos_args[1] // enc file
			<< "\n arg[optind+2] " << pos_args[2] << "\n"// width
			//<< "\n arg[optind+3] " << pos_args[3] << "\n"
			;//<< "\n arg[optind+4] " << pos_args[4] << "\n";

	*/
	//enc_cmd cmdstruct = read_cmd_file(pos_args[1]);

	circuit sim_ckt;
	circuit enc_ckt;

	if (mn.use_verilog)
		sim_ckt.read_verilog(mn.pos_args[0]);
	else
		sim_ckt.read_bench(mn.pos_args[0]);

	sim_ckt.error_check();

	enc_ckt = sim_ckt;
	std::string enc_method = mn.enc_method;
	std::transform(enc_method.begin(), enc_method.end(),
			enc_method.begin(), ::tolower);

	enc_manager_t enc_mgr;
	encryptor* enctor;
	try {
		enctor = &enc_mgr.encryptor_map.at(mn.enc_method);
		bool correct_args = enctor->num_params.empty() ? true:false;
		uint np = mn.pos_args.size() - 2;
		for (uint sz : enctor->num_params) {
			if (np == sz) {
				correct_args = true;
			}
		}
		if (!correct_args) {
			neos_error(enctor->usage_message);
		}
	}
	catch(std::out_of_range& e) {
		neos_error("unknown encryption scheme " << mn.enc_method << "\n");
	}

	auto args = subvec(mn.pos_args, 2, mn.pos_args.size() - 1);

	boolvec corrkey;
	enctor->enc_fun(args, enc_ckt, corrkey);

	if (failed) {
		neos_error("could not encrypt " << mn.pos_args[0] << "\n");
	}
	std::cout << "encryption complete for " << mn.pos_args[0] << "\n";

	std::cout << "number of inserted key-bits: "
					<< enc_ckt.nKeys() << "\n";
	std::cout << std::setprecision(2) << _COLOR_RED("overhead: "
			<< (((float)enc_ckt.nGates()
			/ (float)sim_ckt.nGates()) - 1)*100 << "%\n");

	if (mn.use_verilog) {
		enc_ckt.top_name = enc_ckt_name;
		enc_ckt.write_verilog(mn.pos_args[1]);
	}
	else
		enc_ckt.write_bench(mn.pos_args[1]);

	if (!corrkey.empty()) {
		string corrkey_message;
		if (mn.use_verilog) {
			corrkey_message = fmt("// correct key=%1%", utl::to_delstr(corrkey));
		}
		else {
			corrkey_message = fmt("# correct key=%1%", utl::to_delstr(corrkey));
		}
		utl::insert_in_file_atline(mn.pos_args[1], 0, corrkey_message);
	}

	return 0;
}

void main_enc_antisat (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int ant_width = stoi(pos_args[0]);
	int xor_obf_tree = 0, ntrees = 1, pre_xor_width = 0, mux_obf_width = 0;
	bool pick_internal_ins = 0, pick_internal_outs = 1;
	if (pos_args.size() >= 2)
	    xor_obf_tree = stoi(pos_args[1]);
	if (pos_args.size() >= 3)
	    ntrees = stoi(pos_args[2]);
	if (pos_args.size() >= 4)
		pick_internal_ins = stoi(pos_args[3]);
	if (pos_args.size() >= 5)
		pick_internal_outs = stoi(pos_args[4]);
	if (pos_args.size() >= 6)
		pre_xor_width = stoi(pos_args[5]);
	if (pos_args.size() >= 7)
		mux_obf_width = stoi(pos_args[6]);

	if (pre_xor_width) {
		std::cout << "pre xor encryption with " << pre_xor_width << " keys\n";
		encrypt::enc_xor_rand(enc_ckt, boolvec(pre_xor_width, 0));
	}

	std::cout << "width: " << ant_width << "\n";
    std::cout << "num trees: " << ntrees << "\n";
	encrypt::enc_antisat(enc_ckt, ant_width, ntrees, pick_internal_ins, pick_internal_outs, xor_obf_tree);

}

void main_enc_sfll_point (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int width = stoi(pos_args[0]);
	std::cout << "width: " << width << "\n";
	int xor_obf_tree = 0, ntrees = 1, pre_xor_width = 0;
	bool pick_internal_ins = 0, pick_internal_outs = 1;
	if (pos_args.size() >= 2)
	    xor_obf_tree = stoi(pos_args[1]);
	if (pos_args.size() >= 3)
	    ntrees = stoi(pos_args[2]);
	if (pos_args.size() >= 4)
		pick_internal_ins = stoi(pos_args[3]);
	if (pos_args.size() >= 5)
		pick_internal_outs = stoi(pos_args[4]);
	if (pos_args.size() >= 6)
		pre_xor_width = stoi(pos_args[5]);

	if (pre_xor_width) {
		std::cout << "pre xor encryption with " << pre_xor_width << " keys\n";
		encrypt::enc_xor_rand(enc_ckt, boolvec(pre_xor_width, 0));
	}

	encrypt::enc_sfll_point(enc_ckt, width, ntrees, pick_internal_ins, pick_internal_outs, xor_obf_tree);

}

void main_enc_dtl_antisat (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int ant_width = stoi(pos_args[0]);
	int xor_obf_tree = 0, ntrees = 1, pre_xor_width = 0;
	bool pick_internal_ins = 0, pick_internal_outs = 1;
	int num_first_layer_flips = 0;
	fnct flip_type = fnct::XOR;

	if (pos_args.size() >= 2)
	    xor_obf_tree = stoi(pos_args[1]);
	if (pos_args.size() >= 3)
	    ntrees = stoi(pos_args[2]);
	if (pos_args.size() >= 4)
		pick_internal_ins = stoi(pos_args[3]);
	if (pos_args.size() >= 5)
		pick_internal_outs = stoi(pos_args[4]);
	if (pos_args.size() >= 6)
		pre_xor_width = stoi(pos_args[5]);
	if (pos_args.size() >= 7)
		num_first_layer_flips = stoi(pos_args[6]);
	if (pos_args.size() >= 8)
		flip_type = funct::string_to_enum(pos_args[7]);

	if (num_first_layer_flips == 0)
		num_first_layer_flips = (ant_width/2);

	if (pre_xor_width) {
		std::cout << "pre xor encryption with " << pre_xor_width << " keys\n";
		encrypt::enc_xor_rand(enc_ckt, boolvec(pre_xor_width, 0));
	}

	std::cout << "width: " << ant_width << "\n";
    std::cout << "num trees: " << ntrees << "\n";
	encrypt::enc_antisat_dtl(enc_ckt, ant_width, num_first_layer_flips, flip_type,
			ntrees, pick_internal_ins, pick_internal_outs, xor_obf_tree);

}


void main_enc_xor (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint keysize = stoi(pos_args[0]);
	std::cout << "#width xor: " << keysize << "\n";
	boolvec ckey = main_get_keyvec(pos_args, 1, keysize);
	corrkey = encrypt::enc_xor_rand(enc_ckt, ckey);

}

void main_enc_lut (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint rounds = stoi(pos_args[0]);
	uint widthlut = stoi(pos_args[1]);
	std::cout << "num_luts: " << rounds  << "\n";
	std::cout << "width_lut: " << widthlut << "\n";
	encrypt::enc_lut(rounds, widthlut, enc_ckt);

}


boolvec main_get_keyvec(const std::vector<std::string>& pos_args, int def_args_size, int width) {

	boolvec ckey;
	if (pos_args.size() == (uint)def_args_size + 1) {
		std::cout << "picking key : " << pos_args[1] << "\n";
		if (pos_args[1] == "r")
			ckey = utl::random_boolvec(width);
		else if (pos_args[1] == "0")
			ckey = boolvec(width, false); // utl::_random_boolvec(widthxor);
		else if (pos_args[1] == "1")
			ckey = boolvec(width, true);
		else
			neos_error("bad key option " << pos_args[1] << ". options [r/0/1]");
	}
	else {
		ckey = utl::random_boolvec(width);
	}

	return ckey;

}

void main_enc_xorf (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint keysize = stoi(pos_args[0]);
	uint32_t num_patts = stoi(pos_args[1]);
	std::cout << "num key gates: " << keysize  << "\n";
	std::cout << "num patterns: " << num_patts  << "\n";
	boolvec ckey = main_get_keyvec(pos_args, 4, keysize);
	corrkey = encrypt::enc_xor_fliprate(enc_ckt, ckey, num_patts);

}

void main_enc_xorprob (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint keysize = stoi(pos_args[0]);
	std::cout << "#width xor: " << keysize  << "\n";
	boolvec ckey = main_get_keyvec(pos_args, 3, keysize);
	encrypt::enc_xor_prob(enc_ckt, ckey);

}

void main_enc_and (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	std::cout << "AND/OR insertion\n";
	uint keysize = stoi(pos_args[0]);
	boolvec ckey = main_get_keyvec(pos_args, 3, keysize);
	encrypt::enc_xor_rand(enc_ckt, ckey, fnct::AND);

}


void main_enc_shfk (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	encrypt::enc_shfk(enc_ckt);

}

void main_enc_manymux (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	enc_wire::enc_wire_options opt;

	opt.num_muxs = stof(pos_args[0]);
	opt.mux_size = stoi(pos_args[1]);
	opt.selection_method = pos_args[2];

	opt.print_options();
	encrypt::enc_manymux(enc_ckt, opt);

}

void main_enc_manymuxcg (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	enc_wire::enc_wire_options opt;

	opt.intensity = stof(pos_args[0]);
	opt.keep_acyclic = (bool)stoi(pos_args[1]);
	opt.selection_method = "cg";
	opt.print_options();
	encrypt::enc_manymux(enc_ckt, opt);

}

void main_enc_interconnect(const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	enc_wire::enc_wire_options opt;
	opt.parse_command_line(pos_args);
	opt.print_options();
	encrypt::enc_interconnect(enc_ckt, opt);

}

void main_enc_wirecost (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int num_bufs = stoi(pos_args[0]);
	int method = stoi(pos_args[1]);
	std::cout << "num_bufs: " << num_bufs << "\n";
	encrypt::enc_tbuflock(enc_ckt, num_bufs, method);

}

void main_enc_degree (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int num_mux = stoi(pos_args[0]);
	int num_xor = stoi(pos_args[1]);
	int num_steps = stoi(pos_args[2]);
	std::cout << "num_mux: " << num_mux << "\n";
	std::cout << "num_xor: " << num_xor << "\n";
	encrypt::enc_metric_adhoc(enc_ckt, num_steps, num_mux, num_xor);

}

void main_enc_univ (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	encrypt::univ_opt opt;
	opt.width = stoi(pos_args[0]);
	opt.depth = stoi(pos_args[1]);
	opt.indeg = -1;
	if (pos_args.size() == 5)
		opt.indeg = stoi(pos_args[2]);

	if (pos_args[3] == "cone")
		opt.profile = encrypt::univ_opt::CONE;
	else if (pos_args[3] == "square")
		opt.profile = encrypt::univ_opt::SQUARE;

	std::cout << "width: " << opt.width << "\n";
	std::cout << "depth: " << opt.depth << "\n";

	encrypt::enc_universal(enc_ckt, opt);

}


void main_enc_xmux (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	int num_muxes = stoi(pos_args[0]);
	int num_xors = stoi(pos_args[1]);
	std::cout << "num_muxs: " << num_muxes << "\n";
	std::cout << "num_xors: " << num_xors << "\n";
	encrypt::enc_xmux(enc_ckt, num_muxes, num_xors);

}

void main_enc_crosslock (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	enc_wire::enc_wire_options opt;
	opt.parse_command_line(pos_args);
	opt.method = "crlk";
	opt.print_options();
	encrypt::enc_interconnect(enc_ckt, opt);

}


void main_enc_cyclic (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint loopCount = stoi(pos_args[0]);
	uint loopLen = stoi(pos_args[1]);
	std::cout << "loop-count: " << loopCount  << "\n";
	std::cout << "loop-lentgh: " << loopLen << "\n";
	encrypt::enc_loopy_wMUX(loopCount, loopLen, enc_ckt);

}

void main_enc_cyclic2 (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	uint cyclifyEdgeCount = stoi(pos_args[0]);
	uint loopCount = stoi(pos_args[1]);
	uint loopLen = stoi(pos_args[2]);
	std::cout << "cyclify edges: " << cyclifyEdgeCount  << "\n";
	std::cout << "loop-count: " << loopCount  << "\n";
	std::cout << "loop-lentgh: " << loopLen << "\n";
	encrypt::cyclify(enc_ckt, cyclifyEdgeCount);
	encrypt::enc_loopy_wMUX(loopCount, loopLen, enc_ckt);

}


void main_enc_parity(const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey) {

	std::cout << "parity locking\n";
	uint tree_size = stoi(pos_args[0]);
	uint keys_per_tree = stoi(pos_args[1]);
	uint num_trees = stoi(pos_args[2]);
	std::cout << "#tree_size: " << tree_size  << "\n";
	std::cout << "#keys_per_tree: " << keys_per_tree << "\n";
	std::cout << "#num_trees: " << num_trees << "\n";
	encrypt::enc_parity(enc_ckt, tree_size, keys_per_tree, num_trees);

}

} // namespace mnf

namespace enc {

// all encryption schemes should be registered here
enc::enc_manager_t::enc_manager_t() {

	srand(static_cast<uint>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

	encryptor_map["ant"] = encryptor("Anti-SAT locking",
			"usage: neos -e ant <input file> "
				"<output file> <key_lentgh/2> [tree_xor_obf_keylen]",
			{1, 2, 3, 4, 5, 6}, encryptor::enc_fun_t(mnf::main_enc_antisat));


	encryptor_map["sfll_point"] = encryptor("Corrected output locking",
			"usage: neos -e co <input file> <output file>"
			 " <key_lentgh/2>",
			{1, 2, 3, 4, 5, 6}, encryptor::enc_fun_t(mnf::main_enc_sfll_point));

	encryptor_map["ant_dtl"] = encryptor("Anti-SAT DTL locking",
			"usage: neos -e ant_dtl <incir> <outcir> <ant_width> "
				"<xor_obf_tree> <num_trees> <pick_internal_ins> <pick_internal_outs> <pre_rll_width> "
				"<num_first_layer_flips> <flip_gate_type>",
			{1, 2, 3, 4, 5, 6, 7, 8}, encryptor::enc_fun_t(mnf::main_enc_dtl_antisat));

	encryptor_map["lut"] = encryptor("Random k-LUT insertion",
			"usage: neos -e lut <input file>"
			" <output file> <num_luts> <width_luts>",
			{2}, encryptor::enc_fun_t(mnf::main_enc_lut));

	encryptor_map["xor"] = encryptor("Random XOR/XNOR insertion",
			"usage: neos -e xor <input file>"
			" <output file> <key_lentgh> [r/0/1]",
			{1, 2}, encryptor::enc_fun_t(mnf::main_enc_xor));

	encryptor_map["xorflip"] = encryptor("fliprate-driven XOR/XNOR insertion",
			"usage: neos -e xorflip <input file>"
			" <output file> <key_lentgh> <num_test_patterns> [r/0/1]",
			{2, 3}, encryptor::enc_fun_t(mnf::main_enc_xorf));

	encryptor_map["xorprob"] = encryptor("XOR/XNOR insertion in points with first-level symbolic probability closest to 0.5",
			"usage: neos -e xorpb <input file>"
			" <output file> <key_lentgh>",
			{1}, encryptor::enc_fun_t(mnf::main_enc_xorprob));

	encryptor_map["shfk"] = encryptor("input-output permutation insertion",
			"usage: neos -e shfk <input file> <output file>\n",
			{0}, encryptor::enc_fun_t(mnf::main_enc_shfk));

	encryptor_map["mmux"] = encryptor("insert N muxes of size M",
			"usage: neos -e mmux <input file> <output file>"
			" <num_muxes> <mux_size> <pick_method>\n",
			{3}, encryptor::enc_fun_t(mnf::main_enc_manymux));

	encryptor_map["mmux_cg"] = encryptor("graph-density MUX insertion",
			"usage: neos -e cg <input file> <output file>"
			" <intensity> <keep_acyclic/{0,1}>\n",
			{2}, encryptor::enc_fun_t(mnf::main_enc_manymuxcg));

	encryptor_map["aor"] = encryptor("AND/OR insertion",
			"usage: neos -e aor <input file> <output file>"
			" <num AND/OR gates>\n",
			{1}, encryptor::enc_fun_t(mnf::main_enc_and));

	encryptor_map["int"] = encryptor("interconnect obfuscation suite",
			"",
			{}, encryptor::enc_fun_t(mnf::main_enc_interconnect));

	encryptor_map["cstwr"] = encryptor("cost-function based interconnect obfuscation",
			"usage: neos -e cstwr <input file> <output file>"
			" <num_bufs> <method>\n",
			{2}, encryptor::enc_fun_t(mnf::main_enc_wirecost));

	encryptor_map["deg"] = encryptor("degree-driven insertion",
			"usage: neos -e deg <input file> <output file>"
			" <num_mux> <num_xor> <steps>\n",
			{3}, encryptor::enc_fun_t(mnf::main_enc_degree));

	encryptor_map["univ"] =  encryptor("Universal circuit locking",
			"usage: neos -e univ <input file> <output file>"
			" <widht> <depth> <indeg> <profile>",
			{2, 3}, encryptor::enc_fun_t(mnf::main_enc_univ));

	encryptor_map["crlk"] =  encryptor("Cross-bar locking (crosslock)",
			"",
			{}, encryptor::enc_fun_t(mnf::main_enc_crosslock));

	encryptor_map["cyc"] =  encryptor("Cyclic locking (N loops of length L)",
			"usage: neos -e cyc <input file> <output file>"
			" <number of loops> <loop lentgh>\n",
			{2}, encryptor::enc_fun_t(mnf::main_enc_cyclic));

	encryptor_map["cyc2"] =  encryptor("Cyclic locking (N loops of length L) + cyclify original",
			"usage: neos -e cyc2 <input file> <output file>"
			" <num cyclify edges> <number of loops> <loop lentgh>\n",
			{3}, encryptor::enc_fun_t(mnf::main_enc_cyclic2));

	encryptor_map["par"] =  encryptor("Cross-bar locking (crosslock)",
			"usage: neos -e par <input file> <output file>"
			" <tree_size> <keys_per_tree> <num_trees>\n",
			{2}, encryptor::enc_fun_t(mnf::main_enc_parity));

}

}
