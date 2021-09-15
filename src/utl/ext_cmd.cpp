#include <utl/ext_cmd.h>

namespace ext {

namespace paths {
	string HOME_DIR;
	string NEOS_CMD;
	string NEOS_DIR;
	string DUMP_DIR;
	string TEST_DIR;
	string ABC_CMD;
	string ATPG_CMD;
	string ABC_SYNTH_LIB0;
	string ABC_SYNTH_LIB1;
	string SCRIPT_DIR;
}

bool ext_paths_initialized = false;

void init_ext_paths() {
	const char* home_dir_c = std::getenv("HOME");
	assert(home_dir_c);
	string home_dir(home_dir_c);

	paths::HOME_DIR = home_dir;
	paths::NEOS_CMD = getexepath();
	//std::cout << "executable path: " << paths::NEOS_CMD << "\n";
	paths::NEOS_DIR = _base_path(_base_path(paths::NEOS_CMD));
	//std::cout << "neos dir : " << paths::NEOS_DIR << "\n";
	paths::DUMP_DIR = "tmp/";
	paths::TEST_DIR = "bench/testing/";
	paths::ABC_CMD =  paths::NEOS_DIR + "/bin/abc";
	//std::cout << "abc path : " << paths::ABC_CMD << "\n";
	if (!_file_exists(paths::ABC_CMD)) {
		neos_error("can't find abc executable. Place abc in same directory as neos.");
	}
	paths::ATPG_CMD = home_dir +"/Tools/Atalanta/atalanta";
	paths::ABC_SYNTH_LIB0 = paths::NEOS_DIR +"/cells/simpler.lib";
	paths::ABC_SYNTH_LIB1 = paths::NEOS_DIR +"/cells/xoronly.lib";

	//std::cout << "abc libs: " << paths::ABC_SYNTH_LIB0 << "\n";
	//std::cout << "abc libs: " << paths::ABC_SYNTH_LIB1 << "\n";

	if (!_file_exists(paths::ABC_SYNTH_LIB0) || !_file_exists(paths::ABC_SYNTH_LIB1)) {
		neos_error("can't find abc cell libraries. Place genlibs in neos_dir/cells.");
	}
	paths::SCRIPT_DIR = home_dir + "./script/";

	ext_paths_initialized = true;
}


string create_temp_dir() {
	if (!ext_paths_initialized)
		init_ext_paths();
	pid_t pid = getpid();
	string temp_dir = "./tmp" + std::to_string(pid) + "/";
	string cmd = "mkdir " + temp_dir;
	neos_system(cmd);
	return temp_dir;
}

void remove_temp_dir() {
	pid_t pid = getpid();
	string temp_dir = "./tmp" + std::to_string(pid) + "/";
	string cmd = "rm -rf " + temp_dir;
	neos_system(cmd);
}

void get_rand_circuit(circuit& cir, int num_ins, int num_outs, float activity) {
	string temp_dir = create_temp_dir();
	string cmd = (boost::format("python ./script/getrandnet.py %d %d %s/temp.bench")
			% num_ins % num_outs % temp_dir).str();
	neos_system(cmd);
	cir.read_bench(temp_dir + "temp.bench");
	remove_temp_dir();
}


/*
 * synthlib: 0 -> 12-gate library
 * 			 1 -> xor/and/not library
 * 			 2 -> and/not (AIG without mapping)
 */
int abc_resynthesize(circuit& cir, int synth_lib, bool exit_on_crash, bool quiet) {

	string temp_dir = create_temp_dir();

	string pre_syn = temp_dir + "pre_syn.bench";
	cir.write_bench(pre_syn);
	//rename_capitalize(pre_syn);

	string scrpt = temp_dir + "abc_scrpt.scr";
	std::ofstream fout;
	fout.open(scrpt, std::ofstream::out);

	string post_syn;
	post_syn = temp_dir + "post_syn";


	string lib_path;
	if (synth_lib == 0) {
		lib_path = paths::ABC_SYNTH_LIB0;
	}
	else if (synth_lib == 1) {
		lib_path = paths::ABC_SYNTH_LIB1;
	}

	string cmd;
    cmd += "read_bench  " + pre_syn + ";\n";
	if (synth_lib <= 1)
		cmd += "read_genlib " + lib_path + ";\n";
    cmd += "\n print_stats; strash; print_stats; rewrite; rewrite; rewrite; fraig; balance; rewrite; print_stats;\n";
    if (synth_lib <= 1) {
    	cmd += "\n map -a -s; print_stats;\n";
    	cmd += "write_bench " + post_syn + ".bench;\n";
    }
    else {
    	cmd += "write_bench -l " + post_syn + ".bench";
    }
    //cmd += "\nwrite_verilog " + post_syn + ".v;\n";
    //cmd += "\ncec " + pre_syn;
    cmd += "\nquit";

    fout << cmd;

    fout.close();

	cmd = paths::ABC_CMD + " < " + scrpt;

	if (quiet) cmd += " > /dev/null";

	//std::cout << "RUNNING ABC COMMAND:" << cmd << "\n";

    if(system(cmd.c_str())) {
    	if (exit_on_crash)
    		exit(EXIT_FAILURE);
    	else {
    		remove_temp_dir();
    		return -1;
    	}
    }

	post_syn += ".bench";
	if (synth_lib <= 1) {
		rename_luts(post_syn);
		// rename_decapitalize(post_syn);
	}

	circuit post_cir;
	post_cir.read_bench(post_syn.c_str());
	remove_temp_dir();

	/*post_cir.write_bench();
	cir.write_bench();*/

	/*for (auto xid : post_cir.allins()) {
		std::cout << xid << " -> " << post_cir.wname(xid) << "  " << (int)post_cir.wrtype(xid) << "\n";
	}*/

	for (auto xid : cir.allports()) {
		id pcxid = post_cir.find_wcheck(cir.wname(xid));
		//std::cout << "now at " << pcxid << "  " << xid << " " << cir.wname(xid) << "\n";
		assert(cir.wire_exists(xid) && post_cir.wire_exists(pcxid));
		post_cir.setnodeid(pcxid, xid);
	}

	/*for (auto xid : post_cir.allins()) {
		std::cout << xid << " -> " << post_cir.wname(xid) << "  " << (int)post_cir.wrtype(xid) << "\n";
	}*/

	post_cir.error_check();

	/*if (!ext::check_equivalence_abc(cir, post_cir, quiet)) {
		assert(false);
	}*/

	/*cir.error_check_assert();
	post_cir.error_check_assert();*/

	cir = post_cir;

	return 0;
}

void abc_simplify(aig_t& ntk, bool quiet) {
	circuit simp_cir = ntk.to_circuit();
	aig_t orig_ntk = ntk;
	abc_simplify(simp_cir, quiet);
	ntk = aig_t(simp_cir);
	//assert(ext::check_equivalence_abc(orig_ntk, ntk));
}

int32_t abc_simplify(circuit& simp_cir, bool quiet) {

	std::cout << "calling ABC SIMP\n";
	int orig_ngates = simp_cir.nGates();
	//circuit orig_cir = simp_cir;
	int ret = abc_resynthesize(simp_cir, 0, false, quiet);

	if (ret == -1) {
		std::cout << "abc_simp failed for some reason. circuit not simplified\n";
		return 0;
	}

	int32_t nsaved = orig_ngates - (int32_t)(simp_cir.nGates());
	double dnsaved = nsaved;
	double perc = (dnsaved / orig_ngates) * 100;

	std::cout << " abc_simp saved gates: " << nsaved << "/" << simp_cir.nGates() << "/"
			  << orig_ngates << "  (" << std::setprecision(2) <<  perc << "%)\n";

	// assert(ext::check_equivalence_abc(orig_cir, simp_cir));

	return nsaved;
}

void abc_run_commands(const string& cmd, const string& run_dir ) {

	string log_file = run_dir + "output.log";
	string scrpt = run_dir + "abc_scrpt.scr";
	std::ofstream fout;
	fout.open(scrpt, std::ofstream::out);
	//std::cout << "running ABC commands: " << cmd << "\n";
	fout << cmd;
	fout.close();

	string abc_cmd = paths::ABC_CMD + " < " + scrpt + " | tee " + log_file;
    neos_system(abc_cmd.c_str());
}

void unroll_cir_abc(const circuit& cir, circuit& uncir, int numFrames) {
	string temp_dir = create_temp_dir();

	string cir_file = temp_dir + "cir.bench";
	string uncir_file = temp_dir + "cir_unr.bench";
	cir.write_bench(cir_file);

	string cmd = "read_bench " + cir_file +
			"; strash; zero; frames -i -F " + std::to_string(numFrames) +
			"; write_bench -l " + uncir_file + ";";

	abc_run_commands(cmd, temp_dir);

	uncir.read_bench(uncir_file);

	remove_temp_dir();
}

int check_equivalence_abc(const aig_t& ntk1, const aig_t& ntk2, bool fill_missing_ports, bool quiet) {
	return check_equivalence_abc(ntk1.to_circuit(), ntk2.to_circuit(), fill_missing_ports, quiet);
}

int check_equivalence_abc(const circuit& cir1, const circuit& cir2, bool fill_missing_ports, bool quiet) {

	string temp_dir = create_temp_dir();

	circuit cir1c = cir1, cir2c = cir2;

	if (fill_missing_ports) {
		oidset ports1 = cir1c.allins(), ports2 = cir2c.allins();
		if (ports1.size() != ports2.size()) {
			for (auto inid : ports1) {
				if (cir2c.find_wire(cir1c.wname(inid)) == -1) {
					cir2c.add_wire(cir1c.wname(inid), wtype::IN);
				}
			}
			for (auto inid : ports2) {
				if (cir1c.find_wire(cir2c.wname(inid)) == -1) {
					cir1c.add_wire(cir2c.wname(inid), wtype::IN);
				}
			}
		}
	}


	string cir1file = temp_dir + "cir1.bench";
	cir1c.write_bench(cir1file);
	string cir2file = temp_dir + "cir2.bench";
	cir2c.write_bench(cir2file);

	string log_file = temp_dir + "output.log";
	string scrpt = temp_dir + "abc_scrpt.scr";
	std::ofstream fout;
	fout.open(scrpt, std::ofstream::out);

	string cmd;
	if (cir1c.nLatches() != 0 || cir2c.nLatches() != 0) {
		cmd = "dsec " + cir1file + "  "  + cir2file + "\n";
	}
	else {
		cmd = "cec " + cir1file + "  "  + cir2file + "\n";
	}

    fout << cmd;
    fout.close();

	cmd = paths::ABC_CMD + " < " + scrpt + " > " + log_file;
	if (quiet)
		cmd += " > /dev/null";

    neos_system(cmd.c_str());

    bool equiv = false;

    std::ifstream fin(log_file, std::ifstream::in);
    string line;
    while(std::getline(fin, line)) {
    	std::cout << line << "\n";
    	if ( _is_in("are equivalent", line) ) {
    		//std::cout << "NETWORKS ARE EQUIVALENT\n";
    		equiv = true;
    		break;
    	}
    }
    fin.close();

/*
    if (!equiv) {
    	std::cout << "Unequal circuits\n";
		std::cout << "\n\nCir1\n:";
		cir1c.write_bench();
		std::cout << "\n\nCir2\n:";
		cir2c.write_bench();
    }
*/

	remove_temp_dir();
	return equiv;
}


std::vector<boolvec> atalanta(const circuit& ckt){

	std::vector<boolvec> ret;

	string temp_dir = create_temp_dir();

	string netname = temp_dir + "atpgnet.bench";
	ckt.write_bench(netname);
	rename_capitalize(netname);

	string vec_file;
	vec_file = temp_dir + "atpgnet.vec";

	string cmd;
    cmd = paths::ATPG_CMD + " "  + netname + " > /dev/null";

    std::cout << cmd << "\n";
    if(system(cmd.c_str())) exit(EXIT_FAILURE);

    std::ifstream fin;
    fin.open(vec_file);

    string line;

    std::cout << "\n\ntest vectors\n\n";
    uint i = 0;
    while (std::getline(fin, line) && (line != "END")) {
    	ret.push_back(boolvec());
    	for (auto& b : line) {
    		ret[i].push_back((b == '1') ? 1:0);
    	}
    	i++;
    	std::cout << line << "\n";
    }

    remove_temp_dir();

    return ret;
}


string rename_luts(string& str){

	std::ifstream fin;
	std::ofstream fout;

	string line, newnetlist;

	fin.open(str);

	while ( std::getline(fin, line) ){
		//std::cout << line << "\n";
		boost::replace_all(line, "LUT 0x7 ", "nand ");
        boost::replace_all(line, "LUT 0x7f ", "nand ");
        (line.find(',') == string::npos) ?
        	boost::replace_all(line, "LUT 0x1 ", "not "):
        	boost::replace_all(line, "LUT 0x1 ", "nor ");
        boost::replace_all(line, "LUT 0x01 ", "nor ");
        boost::replace_all(line, "LUT 0x8 ", "and ");
        boost::replace_all(line, "LUT 0xe ", "or ");
        boost::replace_all(line, "LUT 0x6 ", "xor ");
        boost::replace_all(line, "LUT 0x9 ", "xnor ");
        boost::replace_all(line, "LUT 0x2 ", "buf ");
        //std::cout << line << "\n\n";

        assert(!_is_in("LUT ", line));

        newnetlist += line;
        newnetlist += "\n";
	}

	fin.close();

	str.erase (str.end()-6, str.end());
	str += "_rename.b";
	// std::cout << str;
	fout.open(str);
	fout << newnetlist;
	fout.close();

	return str;
}

void rename_capitalize(string& str){

	std::ifstream fin;
	std::ofstream fout;

	string line, newnetlist;

	fin.open(str);

	while ( std::getline(fin, line) ){
		boost::replace_all(line, " nand", "NAND");
		boost::replace_all(line, " nor", "NOR");
        boost::replace_all(line, " not", "NOT");
        boost::replace_all(line, " and", "AND");
        boost::replace_all(line, " or", "OR");
        boost::replace_all(line, " xor", "XOR");
        boost::replace_all(line, " xnor", "XNOR");
        boost::replace_all(line, " buf", "BUF");
        boost::replace_all(line, " mux", "MUX");

        newnetlist += line;
        newnetlist += "\n";
	}

	fin.close();

	fout.open(str);
	fout << newnetlist;
	fout.close();

	return;

}


void rename_decapitalize(string& str){

	std::ifstream fin;
	std::ofstream fout;

	string line, newnetlist;

	fin.open(str);

	while ( std::getline(fin, line) ){
		boost::replace_all(line, " NAND", "nand");
		boost::replace_all(line, " NOR", "nor");
        boost::replace_all(line, " NOT", "not");
        boost::replace_all(line, " AND", "and");
        boost::replace_all(line, " OR", "or");
        boost::replace_all(line, " XOR", "xor");
        boost::replace_all(line, " XNOR", "xnor");
        boost::replace_all(line, " BUF", "buf");

        newnetlist += line;
        newnetlist += "\n";
	}

	fin.close();

	fout.open(str);
	fout << newnetlist;
	fout.close();

	return;

}

}



