/*
 * mc.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: kaveh
 */

#include "dec/mcdec.h"
#include "main/main_util.h"

namespace dec_ns {

mcdec_nuxmv::mcdec_nuxmv(circuit& sim_ckt,
		circuit& enc_cir, const boolvec& corrkey) :
				mcdec_bmc(sim_ckt, enc_cir, corrkey) {

	// create temp directory. tie to PID for multiprocess safety
	pid_t pid = getpid();
	std::cout << pid << "\n";
	temp_dir = "./_temp_" + std::to_string(pid);
	std::string cmd = "mkdir " + temp_dir;
	system_cl(cmd.c_str());
	dec_smv_file = temp_dir + "/dec.smv";
	dec_res_file = temp_dir + "/res.txt";
	dec_term_file = temp_dir + "/term.smv";
	sim_smv_file = temp_dir + "/sim.smv";
	nuXmv_cmd = "nuXmv ";

};

void mcdec_nuxmv::_clean_up() {

	unlink(dec_smv_file.c_str());
	unlink(dec_res_file.c_str());
	unlink(dec_term_file.c_str());
	unlink(sim_smv_file.c_str());

	// hope this doesn't delete the world
	std::string cmd = "rm -rf " + temp_dir;
	system_cl(cmd.c_str());
}


int mcdec_nuxmv::parse_args(int argc, char** argv) {

	boost::program_options::options_description op;
	mcdec_bmc::get_opt_parser(*this, op);

	if ( mnf::parse_args(argc, argv, op) || print_help) {
		std::cout << op << "\n";
		return 1;
	}

	read_sim_enc_key_args(pos_args, oracle_binary.empty());

	if (pos_args.size() != 1) {
		neos_error("usage: neos -d nuxmv <sim_cir/enc_cir> <sim_cir/corrkey> [iteration_limit]");
		return 1;
	}
	iteration_limit = stoi(pos_args[0]);

	return 0;

}


void mcdec_nuxmv::solve_nuxmv() {

	_build_mitter();

	while (true) {

		std::cout << "\nmitter dis\n";
		dis_t dis;
		if (_solve_for_dis(dis)) {
			V1(std::cout << "//-------ITERATION:   " << iter << "----------\n");
			dis.ys = query_seq_oracle(dis.xs);
			_print_dis(dis);
			V1(_simulate_enc(dis));
			_add_dis_constraint(dis);
			if (_check_termination())
				break;
		}
		else {
			//bmc_len += mcopts.bmc_bound_steps;
			//std::cout << "increasing bmc bound to " << bmc_len << "\n";
			if (bmc_len < 33) {
				bmc_len *= 2;
				std::cout << "increasing bmc bound to " << bmc_len << "\n";
			}
			else if (use_ic3) {
				std::cout << "unbounded model checking succeeded\n";
				break;
			}
			else {
				std::cout << "switching to ic3\n";
				use_ic3 = true;
			}
		}
		if (iter++ == iteration_limit) {
			break;
		}
	}

	_extract_key();
	std::cout << "\ndone\n";

	_clean_up();

}

template <typename Ext>
int mcdec_nuxmv::_run_nuXmv(
		const std::string& cmd, const smv_model& smv_mdl,
		const std::string& smv_file, const std::string& res_file,
		Ext& ext) {

	// using named pipes to communicate with nuXmv to avoid disk during the
	// decryption process
	mkfifo(smv_file.c_str(), 0666);
	mkfifo(res_file.c_str(), 0666);

	// this thread will wait on the pipe-read until nuXmv begins outputing
	auto extractor_thread = std::async(std::launch::async, ext);

	// run nuXmv which waits for dec_smv_file pipe to be written
	FILE* nx_fd = popen(cmd.c_str(), "r");


	// writing dec_smv_file pipe gets nuXmv working
	std::ofstream fout(smv_file, std::ofstream::out);
	fout << smv_mdl;
	fout.close();

	std::ofstream fout_tmp(smv_file + ".temp", std::ofstream::out);
	fout_tmp << smv_mdl;
	fout_tmp.close();


	// nuXmv and dis_extractor should be working together at this point

	// lets wait for nuXmv to finish
    int status = pclose(nx_fd);
	if (WIFEXITED(status)) {

		// nuXmv exited. wait for dis_extractor
		int extractor_ret_value = extractor_thread.get();

		// we're done and we can clean up
		unlink(smv_file.c_str());
		unlink(res_file.c_str());

		return extractor_ret_value;
	}

	neos_error("nuXmv waitpid failed");
	return -1;
}

/*
 * check termination conditions using nuxmv
 */
int mcdec_nuxmv::_check_termination() {

	V1(std::cout << "\n//----- checking termination -----//\n");

	if (++term_checks == 1) {
		term_cond_str = "\nVAR\n";
		term_cond_str += _define_array("xt", num_ins);
		term_cond_str += _define_array("yt1", num_outs);
		term_cond_str += _define_array("yt2", num_outs);

		term_cond_str += "cet1 : ce_open(xt, k1, yt1);\n";
		term_cond_str += "cet2 : ce_open(xt, k2, yt2);\n";

		term_cond_str += "\nDEFINE _seq := ";
		_delim_list_coll(term_cond_str, p, enc_lpairs,
				"(cet1.%1% = cet2.%1%)", _model_wire_name(p.first), "&");
		term_cond_str += ";\nDEFINE _nseq := ";
		_delim_list_coll(term_cond_str, p, enc_lpairs,
				"(cet1.%1% = cet2.%1%)", _model_wire_name(p.second), "&");
		term_cond_str += ";\nDEFINE _ct_yteq := ";
		_delim_list_range(term_cond_str, x, 0, num_outs,
				"(yt1[%1%] = yt2[%1%])", x, "&");
		term_cond_str += ";\nDEFINE _term :=  _seq & _ct_yteq;\n";

		term_cond_str += "INVAR (_nseq);\n";
		term_cond_str += opened_ce;
	}


	auto start = utl::_start_wall_timer();

	//auto ext = std::bind(&mcdec::_check_termination_extractor, this);
	auto ext = [&]{ return this->_check_termination_extractor(); };

	std::string nuxmv_cmd =
		(format("echo \"read_model -i %s\\n go_bmc \\n"
				" set bmc_length %d \\n "
				"check_ltlspec_bmc_inc -P \\\"termination\\\" \\nquit\\n\" "
				"| %s -int | grep '' > %s &")
			% dec_term_file % (max_dis_len-1) % nuXmv_cmd % dec_res_file).str();

	//dec_smv.term_cond = term_cond_str;


	term_cond_spec_str = fmt("LTLSPEC NAME termination := G[%1%,%1%] (_term);\n\n\n", (max_dis_len-1));
	dec_smv.term_cond = term_cond_spec_str + term_cond_str;

	V2(std::cout << dec_smv)
	int ret_code = _run_nuXmv(nuxmv_cmd, dec_smv, dec_term_file, dec_res_file, ext);

	V1(std::cout << "\nterm ret code : " << ret_code << "\n")

	V1(std::cout << "term time : " << utl::_stop_wall_timer(start) << "\n")

	return ret_code;
}

int mcdec_nuxmv::_check_termination_extractor() {

	std::ifstream res_fin(dec_res_file, std::ifstream::in);

	if (!res_fin.good())
		neos_error("can't open " << dec_res_file);

	std::string line;

	int ret_code = 1;
	while (std::getline(res_fin, line)) {

		boost::trim(line);
		if (_is_in("is false", line)) {
			V1(std::cout << "\ntermination is false\n");
			ret_code = 0;
		}
		V2(std::cout << line << "\n";)
	}

	if (ret_code == 1)
		std::cout << "\ncombinational termination detected\n";

	res_fin.close();

	return ret_code;
}


bool mcdec_nuxmv::_solve_for_dis(dis_t& dis) {
	V1(std::cout << "//--------Extracting DIS--------//\n");

	V2(std::cout << dec_smv);

	dis.clear();

	auto ext = [&]{ return this->_dis_extractor(dis); };

	//std::string invar_cmd = (use_ic3) ? "check_invar_bmc_ic3 -a \\\"avy\\\"":"check_invar_bmc_inc";
	std::string invar_cmd = (use_ic3) ? "check_invar_ic3":"check_invar_bmc_itp -a falsification2";

	std::string nuxmv_cmd =
		(format("echo \"read_model -i %s\\n go_bmc \\n"
				" set bmc_length %d \\n"
				" %s -P \\\"_no_dis\\\" \\nquit\\n\" "
				"| %s -int | grep \'=\\|<-\' > %s &")
				//"| %s -int > %s &")
			% dec_smv_file % bmc_len % invar_cmd % nuXmv_cmd % dec_res_file).str();

	auto start = _start_wall_timer();

	int ret_code = _run_nuXmv(nuxmv_cmd, dec_smv, dec_smv_file, dec_res_file, ext);
	V1(std::cout << "dis ret_code : " << ret_code << "\n");

	V1(std::cout << "dis time : " << _stop_wall_timer(start) << "\n");

	if (ret_code == 1) {
		V1(std::cout << "\nno counter example found\n");
		return false;
	}
	return true;
}

int mcdec_nuxmv::_dis_extractor(dis_t& dis) {

	std::ifstream res_fin(dec_res_file, std::ifstream::in);

	if (!res_fin.good())
		neos_error("can't open " << dec_res_file);

	std::string line;

	uint state_count = 0;
	while (std::getline(res_fin, line)) {

		std::vector<std::string> strs;
		boost::trim(line);
		V2(
		if (1) //line[0] == 'x' || line[0] == 'k'
				//|| line[0] == 'y' || line.substr(0,2) == "->")
				{
			std::string ln1 = line;
			boost::replace_all(ln1, "FALSE", "0");
			boost::replace_all(ln1, "TRUE", "1");
			std::cout << ln1 << "\n";
		}
		)


		if (line.substr(0,4) == "-> S") {
			if (state_count == 0) {
				dis.xs.push_back(boolvec(num_ins, true));
				dis.ks[0] = boolvec(num_keys, true);
				dis.ks[1] = boolvec(num_keys, true);
			}
			else {
				dis.xs.push_back(dis.xs[state_count-1]);
			}
			state_count++;
		}
		else if (state_count != 0) {
			// std::cout << line << "\n";
			boost::split(strs, line, boost::is_any_of("= []"), boost::token_compress_on);
			if (strs.size() == 3) {
				// std::cout << strs[0] << " " << strs[1] << " " << strs[2] << "\n";

				bool value = (strs[2] == "FALSE") ? 0:1;

				if (strs[0] == "x") {
					uint x_index = atoi(strs[1].c_str());
					dis.xs[state_count-1][x_index] = value;
				}
				else if (strs[0][0] == 'k') {
					uint k_vecnum = atoi(strs[0].substr(1).c_str());
					uint k_index = atoi(strs[1].c_str());
					dis.ks[(k_vecnum-1)][k_index] = value;
				}
			}
		}
	}

	// no counter example found
	if (state_count == 0) {
		res_fin.close();
		return 1;
	}

	max_dis_len = MAX(max_dis_len, dis.length());

	V1(std::cout << "max dis len: " << max_dis_len << "\n");

	res_fin.close();
	return 0;
}

void mcdec_bmc::_print_dis(dis_t& dis) {

	std::cout << "DIS iter: " << stats.dis_count << "\n";
	for (uint i = 0; i < dis.length(); i++) {
		std::cout << "  DIP clk_" << i << " -> ";
		for (auto x : dis.xs[i]) {
			std::cout << x;
		}
		if (dis.ys.size() > i) {
			std::cout << "  ";
			for (auto x : dis.ys[i]) {
				std::cout << x;
			}
		}
		std::cout << "\n";
	}

}


mcdec_nuxmv* context_ptr;
uint _Num_Sig_Interrupts = 0;
/*
 * undefined behavior can occur if
 * interrupt occurs at the wrong time
 * FIXME: implement a last_key system
 */
void sigint_handler_mc(int signum, siginfo_t* u1, void* u2) {
	_Num_Sig_Interrupts++;
	std::cout << "num interrupts: " << _Num_Sig_Interrupts << "\n";
	if (signum == SIGINT && _Num_Sig_Interrupts == 1) {
		std::cout << "\nFirst interrupt. trying to extract current key\n";

		unlink(context_ptr->dec_smv_file.c_str());
		unlink(context_ptr->dec_res_file.c_str());

		context_ptr->_extract_key();
		context_ptr->_clean_up();
		exit(1);
	}

	if (signum == SIGINT && _Num_Sig_Interrupts == 2) {
		std::cout << "\nSecond interrupt. exiting.\n";
		exit(1);
	}
}


void mcdec_bmc::_push_all_lits(std::vector<slit>& litvec1, const std::vector<slit>& litvec2) {
	for (int i = 0; i < (int)litvec2.size(); i++) {
		litvec1.push_back(litvec2[i]);
	}
}

void mcdec_nuxmv::_extract_key() {

	V1( std::cout << "//------ extracting key-----//\n" );

	dec_smv.top_mod += fmt("LTLSPEC NAME _ext_key := G[%1%,%1%]( FALSE )\n", max_dis_len);

	std::string nuxmv_cmd =
		(format("echo \" "
				" read_model -i %s\\n"
				" go_bmc \\n"
				" set bmc_length %d \\n"
				" check_ltlspec_bmc_inc \\n"
				" quit\\n\" "
				"| %s -int | grep '' > %s &")
			% dec_smv_file % max_dis_len % nuXmv_cmd % dec_res_file).str();

	dis_t dis;
	auto ext = [&]{ return this->_dis_extractor(dis); };

	int ret_code = _run_nuXmv(nuxmv_cmd, dec_smv, dec_smv_file, dec_res_file, ext);
	assert(ret_code == 0);

	boolvec key(num_keys, false);
	std::cout << "\nkey=";
	uint i = 0;
	for (auto& kid : enc_cir->keys()) {
		bool key_val = dis.ks[0][ewid2ind.at(kid)];
		key[i++] = key_val;
		std::cout << key_val;
	}
	std::cout << "\n";

	check_key(key);
}


/*
 * this function is entirely for testing
 * as the deobfuscation need not simulate the
 * encrypted circuit
 */
void mcdec_bmc::_simulate_enc(dis_t& dis) {

	//V1(std::cout << "\n//-------simulation of enc--------//\n");

	uint num_clks = dis.length();
	std::vector<id2boolmap> inputmaps(num_clks, id2boolmap());
	std::vector<id2boolmap> simmaps;

	// fill the input map
	for (uint clk = 0; clk < dis.length(); clk++) {
		foreach_inid(*enc_cir, inid) {
			V1(std::cout << enc_cir->wname(inid) << " : " << dis.xs[clk][ewid2ind.at(inid)] << "\n");
			inputmaps[clk][inid] = dis.xs[clk][ewid2ind.at(inid)];
		}
	}

	for (auto kvc : {0, 1}) {

		for (uint clk = 0; clk < dis.length(); clk++) {
			foreach_keyid(*enc_cir, kid) {
				V1(std::cout << enc_cir->wname(kid) << " : " << dis.ks[kvc][ewid2ind.at(kid)] << "\n");
				inputmaps[clk][kid] = dis.ks[kvc][ewid2ind.at(kid)];
			}
		}

		//simulate sequentially
		enc_cir->simulate_seq(inputmaps, simmaps);

		//retrieve output
		for (uint clk = 0; clk < dis.length(); clk++) {
			foreach_outid(*enc_cir, oid) {
				V1(std::cout << enc_cir->wname(oid) << " : " << simmaps[clk].at(oid) << "\n");
			}
		}
	}


}

void mcdec_bmc_int::_simulate_enc_int(dis_t& dis) {

	V1(std::cout << "\n//-------simulation of enc--------//\n");

	uint num_clks = dis.length();
	std::vector<id2boolmap> inputmaps(num_clks, id2boolmap());
	std::vector<id2boolmap> simmaps;

	// fill the input map
	for (uint clk = 0; clk < dis.length(); clk++) {
		foreach_inid(*enc_cir, inid) {
			V1(std::cout << enc_cir->wname(inid) << " : " << dis.xs[clk][ewid2ind.at(inid)] << "\n");
			inputmaps[clk][inid] = dis.xs[clk][ewid2ind.at(inid)];
		}
	}

	for (auto kvc : {0, 1}) {

		for (uint clk = 0; clk < dis.length(); clk++) {
			int i = 0;
			for (auto kp : enc2mitt_kmap[kvc] ) {
				V1(std::cout << enc_cir->wname(kp.first) << " : " << dis.ks[kvc][i] << "\n");
				inputmaps[clk][kp.first] = dis.ks[kvc][i++];
			}
		}

		//simulate sequentially
		enc_cir->simulate_seq(inputmaps, simmaps);

		//retrieve output
		for (uint clk = 0; clk < dis.length(); clk++) {
			foreach_outid(*enc_cir, oid) {
				V1(std::cout << enc_cir->wname(oid) << " : " << simmaps[clk].at(oid) << "\n");
			}
		}
	}

}


std::string mcdec_nuxmv::_bool_str(bool b) {
	return (b) ? "TRUE" : "FALSE";
}


std::string mcdec_nuxmv::_define_array(const std::string& basename, uint length) {
	return (format("%s : array 0..%d of boolean;\n") % basename % (length-1)).str();
}

std::string mcdec_nuxmv::_x_str(int64_t iteration, int64_t clk, int64_t index) {
	std::string clkstr = (clk == -1 || clk == 0) ? "":fmt("_%d", clk);
	std::string iterstr = (iteration == -1) ? "":fmt("_%d", iter);
	std::string indexstr = (index == -1) ? "":fmt("[%d]", index);

	return "x" + iterstr + clkstr + indexstr;
}

std::string mcdec_nuxmv::_y_str(int64_t z, int64_t iteration, int64_t clk, int64_t index) {
	std::string clkstr = (clk == -1) ? "":fmt("_%d", clk);
	std::string iterstr = (iteration == -1) ? "":fmt("_%d", iter);
	std::string indexstr = (index == -1) ? "":fmt("[%d]", index);

	return fmt("y%d", z) + iterstr + clkstr + indexstr;
}

void mcdec_nuxmv::_build_mitter() {

	V1(std::cout << "\n//----------MITTER SMV-------------//\n");

	// file header
	dec_smv.top_mod += "\n\n--generated by seqneos on "
			+ utl::time_and_date() + "\n\n";


	//------ mitter circuit top module ----//

	dec_smv.top_mod += "MODULE main\n\n";

	// ---- x and y1 and y2 ----- //
	dec_smv.top_mod += "\nVAR\n";
	dec_smv.top_mod += _define_array("x", num_ins);
	dec_smv.top_mod += _define_array("y1", num_outs);
	dec_smv.top_mod += _define_array("y2", num_outs);

	// ------- key vairbales ----//
	dec_smv.top_mod += "\nFROZENVAR\n";
	dec_smv.top_mod += _define_array("k1", num_keys);
	dec_smv.top_mod += _define_array("k2", num_keys);


	dec_smv.top_mod += "\nVAR\n";
	dec_smv.top_mod += "   ce1 : ce(x, k1, y1);\n";
	dec_smv.top_mod += "   ce2 : ce(x, k2, y2);\n";

	// ----- mitter conditions -----//
	dec_smv.top_mod += "\nDEFINE\n";
	for (uint i = 0; i < num_outs; i++) {
		dec_smv.top_mod += fmt("   _mitt_%1% := (y1[%1%] = y2[%1%]);\n", i);
	}
	dec_smv.top_mod += "   _mitter := (_mitt_0";
	for (uint i = 1; i < num_outs; i++) {
		dec_smv.top_mod += fmt(" & _mitt_%d", i);
	}
	dec_smv.top_mod += ");\n";


/*
	dec_smv.top_mod += "\nDEFINE\n";
	uint si = 0;
	for (auto p : enc_lpairs) {
		dec_smv.top_mod += fmt("   _smitt_%1% := (ce1.%2% = ce2.%2%);\n", (si) % _model_wire_name(p.first));
		si++;
	}
	dec_smv.top_mod += "   _smitter := (_smitt_0";
	for (uint i = 1; i < num_outs; i++) {
		dec_smv.top_mod += fmt(" & _smitt_%d", i);
	}
	dec_smv.top_mod += ");\n";
*/


	// ---- unique key condition ---- //
	/*
	dec_smv.top_mod << "\nDEFINE\n";
	for (uint i = 0; i < num_keys; i++) {
		dec_smv.top_mod << format("   _keq_%d := (k1[%d] = k2[%d]);\n")
				% i % i % i;
	}
	dec_smv.top_mod << "   _keq := (_keq_0";
	for (uint i = 1; i < num_keys; i++) {
		dec_smv.top_mod << format(" & _keq_%d") % i;
	}
	dec_smv.top_mod << ");\n";
 	 */


	//---- add invarinat specification------//
	dec_smv.top_mod += "INVARSPEC NAME _no_dis := ( _mitter );\n\n";

	dec_smv.top_mod += "\n\n";

	//------ encrypted circuit module -------//
	ce += "MODULE ce(x, k, y)\n";
	opened_ce += "MODULE ce_open(x, k, y)\n";

	//module state variables
	ce += "VAR\n";
	opened_ce += "VAR\n";

	foreach_latch(*enc_cir, dffid, dffout, dffin) {
		if (!enc_cir->isOutput(dffout)) {
			ce += "   " + _model_wire_name(dffout) + ": boolean;\n";
			opened_ce += "   " + _model_wire_name(dffout) + ": boolean;\n";
		}
	}
	// gates
	ce += "\nASSIGN\n";
	foreach_latch(*enc_cir, dffid, dffout, dffin) {
		ce += fmt("   next(%s) := %s;\n",
				_model_wire_name(dffout) % _model_wire_name(dffin) );
	}

	ce += "\nASSIGN\n";
	foreach_latch(*enc_cir, dffid, dffout, dffin) {
		ce += fmt("   init(%s) := FALSE;\n",
				_model_wire_name(dffout));
	}

	ce += "\nASSIGN\n";
	opened_ce += "\nASSIGN\n";
	idset output_gates;
	for (auto oid : enc_cir->outputs()) {
		add_all(output_gates, enc_cir->wfanins(oid));
	}
	for (auto& gid : output_gates) {
		if (!enc_cir->isLatch(gid)) {
			ce += fmt("   %s;\n",  _gate_expr(enc_cir->getcgate(gid)) );
			opened_ce += fmt("   %s;\n",  _gate_expr(enc_cir->getcgate(gid)) );
		}
	}

	ce += "\nDEFINE\n";
	opened_ce += "\nDEFINE\n";

	foreach_gate(*enc_cir, g, gid) {
		if ( g.gfun() != fnct::DFF
				&& !enc_cir->isOutput(g.fo0()) ) {
			ce += fmt("   %s;\n",  _gate_expr(g));
			opened_ce += fmt("   %s;\n",  _gate_expr(g));
		}
	}

	ce += "\n\n";
	opened_ce += "\n\n";

	dec_smv.sub_mods += ce;

	V2(std::cout << dec_smv);
	V2(std::cout << opened_ce);

}

void mcdec_nuxmv::_convert_to_smv(const circuit& cir,
		const oidset& frozens, std::string& smv) {

	// file header
	smv += "\n\n--generated by seqneos on "
			+ utl::time_and_date() + "\n\n";


	//------ mitter circuit top module ----//
	smv += "MODULE main\n\n";

	// ---- x and y1 and y2 ----- //
	smv += "\nIVAR\n";
	foreach_inid(cir, xid) {
		if (_is_not_in(xid, frozens))
			smv += fmt("%s : boolean;\n", cir.wname(xid));
	}
	foreach_keyid(cir, kid) {
		if (_is_not_in(kid, frozens))
			smv += fmt("%s : boolean;\n", cir.wname(kid));
	}

	/*
	smv += "\nVAR\n";
	foreach_outid(cir, oid) {
		smv += fmt("%s : boolean;\n", cir.wname(oid));
	}
 	 */

	if (!frozens.empty()) {
		smv += "\nFROZENVAR\n";
		for (auto xid : frozens) {
			smv += fmt("%s : boolean;\n", cir.wname(xid));
		}
	}

	//module state variables
	smv += "VAR\n";

	foreach_latch(cir, dffid, dffout, dffin) {
		if (!cir.isOutput(dffout)) {
			smv += "   " + cir.wname(dffout) + ": boolean;\n";
		}
	}
	// gates
	smv += "\nASSIGN\n";
	foreach_latch(cir, dffid, dffout, dffin) {
		smv += fmt("   next(%s) := %s;\n",
				cir.wname(dffout) % cir.wname(dffin) );
		smv += fmt("   init(%s) := FALSE;\n",
				cir.wname(dffout));
	}

	/*
	smv += "\nASSIGN\n";
	idset output_gates;
	for (auto& out : cir.outputs()) {
		add_all(output_gates, cir.wfanins(out));
	}
	for (auto& gid : output_gates) {
		if (!cir.isLatch(gid)) {
			smv += fmt("   %s;\n",  _gate_expr(cir.getcgate(gid), cir) );
		}
	}
 	 */

	smv += "\nDEFINE\n";
	foreach_gate(cir, g, gid) {
		if ( g.gfun() != fnct::DFF ) {
				//&& !cir.isOutput(g.fanout) ) {
			smv += fmt("   %s;\n",  _gate_expr(g, cir));
		}
	}

	smv += "\n\n";
	foreach_outid(cir, oid) {
		smv += "\nINVARSPEC NAME gspec_out := !" + cir.wname(oid) + "\n";
	}

	smv += "\n\n";
}


std::string mcdec_nuxmv::_define_comp(
		const std::string& base_str1,
		const std::string& base_str2,
		const std::string& out,
		uint len) {

	std::string retstr;

	retstr += "\nDEFINE\n";
	for (uint i = 0; i < len; i++) {
		retstr += fmt("   %s_%d := (%s[%d] = %s[%d]);\n",
				out % i % base_str1 % i % base_str2 %i );
	}
	retstr += "DEFINE " + out + " := (";
	for (uint i = 0; i < len; i++) {
		retstr += fmt("(%s_%d)", out % i);
		if (i != len-1)
			retstr += " & ";
	}
	retstr += ");\n";

	return retstr;
}

void mcdec_nuxmv::_add_dis_constraint(dis_t& dis) {

	// add comparator
	dec_smv.top_mod += "\nVAR\n";
	dec_smv.top_mod += _define_array(_x_str(iter), num_ins);
	dec_smv.top_mod += _define_array(_y_str(1, iter), num_outs);
	dec_smv.top_mod += _define_array(_y_str(2, iter), num_outs);

	dec_smv.top_mod +=
			fmt("\nVAR\n   cio1_%1% : ce(x_%1%, k1, y1_%1%);\n", iter);
	dec_smv.top_mod +=
			fmt("   cio2_%1% : ce(x_%1%, k2, y2_%1%);\n", iter);


	dec_smv.top_mod += _define_comp(
			_y_str(1, iter), _y_str(1, iter, 0),
			fmt("_y1_comp_%d", iter), num_outs);

	dec_smv.top_mod += _define_comp(
			_y_str(2, iter), _y_str(2, iter, 0),
			fmt("_y2_comp_%d", iter), num_outs);

	dec_smv.top_mod += fmt("INVAR ( _y1_comp_%1% & _y2_comp_%1%);\n", iter);

	for (uint clk = 0; clk < dis.length(); clk++) {

		dec_smv.top_mod += "\nVAR\n";
		if (clk != 0)
			dec_smv.top_mod += _define_array(_x_str(iter, clk), num_ins);
		dec_smv.top_mod += _define_array(_y_str(1, iter, clk), num_outs);
		dec_smv.top_mod += _define_array(_y_str(2, iter, clk), num_outs);
	}

	for (uint clk = 1; clk < dis.length(); clk++) {

		dec_smv.top_mod += "\nASSIGN \n";
		for (uint i = 0; i < num_ins; i++) {
			dec_smv.top_mod += fmt("    next(%s) := %s;\n",
					_x_str(iter, clk-1, i) % _x_str(iter, clk, i) );
		}

		for (auto z : {1, 2})
			for (uint i = 0; i < num_outs; i++) {
				dec_smv.top_mod += fmt("    next(%s) := %s;\n",
						_y_str(z, iter, clk-1, i) % _y_str(z, iter, clk, i) );
			}
	}

	for (uint clk = 0; clk < dis.length(); clk++) {
		dec_smv.top_mod += "\nASSIGN \n";
		for (uint i = 0; i < num_ins; i++) {
			dec_smv.top_mod += fmt("    init(%s) := %s;\n",
					_x_str(iter, clk, i)
					% _bool_str(dis.xs[clk][i]) );
		}

		for (auto z : {1, 2}) {
			for (uint i = 0; i < num_outs; i++) {
				dec_smv.top_mod += fmt("    init(%s) := %s;\n",
						_y_str(z, iter, clk, i)
						% _bool_str(dis.ys[clk][i]) );
			}
		}
	}

}

std::string mcdec_nuxmv::_gate_expr(const gate& g, const circuit& cir) {

	std::stringstream ret_ss;
	ret_ss << cir.wname(g.fo0());

	if (g.gfun() == fnct::NAND
			|| g.gfun() == fnct::NOR || g.gfun() == fnct::NOT)
		ret_ss << " := !(";
	else ret_ss << " := (";

	uint i = 0;
	for (auto& in : g.fanins) {
		ret_ss << cir.wname(in);
		if (++i == g.fanins.size()) {
			break;
		}
		else {
			switch (g.gfun()) {
			case fnct::AND:
			case fnct::NAND:
				ret_ss << " & ";
				break;
			case fnct::OR:
			case fnct::NOR:
				ret_ss << " | ";
				break;
			case fnct::XOR:
				ret_ss << " xor ";
				break;
			case fnct::XNOR:
				ret_ss << " xnor ";
				break;
			case fnct::MUX:
				if (i == 1)
					ret_ss << " ? ";
				else if (i == 2)
					ret_ss << " : ";
				break;
			default:
				assert(g.gfun() != fnct::DFF);
				assert(g.fanins.size() == 1);
				break;
			}

		}
	}

	ret_ss << ")";

	return ret_ss.str();

}



std::string mcdec_nuxmv::_gate_expr(const gate& g) {

	std::stringstream ret_ss;
	ret_ss << _model_wire_name(g.fo0());

	if (g.gfun() == fnct::NAND
			|| g.gfun() == fnct::NOR || g.gfun() == fnct::NOT)
		ret_ss << " := !(";
	else ret_ss << " := (";

	uint i = 0;
	for (auto& in : g.fanins) {
		ret_ss << _model_wire_name(in);
		if (++i == g.fanins.size()) {
			break;
		}
		else {
			switch (g.gfun()) {
			case fnct::AND:
			case fnct::NAND:
				ret_ss << " & ";
				break;
			case fnct::OR:
			case fnct::NOR:
				ret_ss << " | ";
				break;
			case fnct::XOR:
				ret_ss << " xor ";
				break;
			case fnct::XNOR:
				ret_ss << " xnor ";
				break;
			default:
				assert(g.gfun() != fnct::DFF);
				assert(g.fanins.size() == 1);
				break;
			}

		}
	}

	ret_ss << ")";

	return ret_ss.str();

}


/*
 * get name of model variable per wire index
 */
std::string mcdec_nuxmv::_model_wire_name(id wr, bool array) {
	std::string retname;

	if ( enc_cir->isInput(wr) ) {
		std::string index_str = std::to_string(ewid2ind.at(wr));
		if (!array)
			retname = "x[" + index_str + ":" + index_str + "]";
		else
			retname = "x[" + index_str + "]";
	}
	else if ( enc_cir->isKey(wr) ) {
		std::string index_str = std::to_string(ewid2ind.at(wr));
		if (!array)
			retname = "k[" + index_str + ":" + index_str + "]";
		else
			retname = "k[" + index_str + "]";
	}
	else if ( enc_cir->isOutput(wr) ) {
		std::string index_str = std::to_string(ewid2ind.at(wr));
		if (!array)
			retname = "y[" + index_str + ":" + index_str + "]";
		else
			retname = "y[" + index_str + "]";
	}
	else
		retname = enc_cir->wname(wr);

	return retname;
}

}
