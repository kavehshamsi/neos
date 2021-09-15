/*
 * misc1.cpp
 *
 *  Created on: Nov 24, 2020
 *      Author: kaveh
 */

#include <armadillo>
#include <filesystem>
#include <regex>

#include "main/main_util.h"
#include "base/blocks.h"
#include "aig/aig.h"
#include "opt/opt.h"
#include "opt/rwr.h"
#include "utl/ext_cmd.h"
#include "misc/sym.h"
#include "enc/enc.h"
#include "sat/qbf.h"

#define FATAL_ERROR -1
#define CMD_EXIT 2
#define CMD_FAILURE 1
#define CMD_SUCCESS 0

namespace cmd {

int neos_command_gen_block(int argc, char** argv) {
	using namespace ckt;
	circuit comp_cir;
	vector<string> arg_vec(argv, argv + argc);
	
	/*int width = 0;
	int capacity = 0;
	if (arg_vec.size() != 4) {
	    neos_error("usage: gen_blocks <width> <capacity> <outfile>");
    }
    std::cout << arg_vec[0] << "\n";
    std::cout << arg_vec[1] << "\n";
    width = stoi(arg_vec[1]);
    capacity = stoi(arg_vec[2]);
    
    circuit kcomp = (capacity == 0) ? ckt_block::key_comparator(width) : ckt_block::rca_lut(capacity, width);
    kcomp.write_verilog(arg_vec[3]);
	*/
	int width = stoi(arg_vec[3]);
    circuit kcomp = ckt_block::key_comparator(width);
	//circuit kcomp = ckt_block::vector_half_comparator(stoi(arg_vec[2]));
	//circuit kcomp = ckt_block::vector_adder(stoi(arg_vec[2]));
	kcomp.write_bench(arg_vec[1]);

	circuit simcir;
	for (int i = 0; i < width; i++) {
		simcir.add_wire("in" + std::to_string(i), wtype::IN);
	}
	id out = simcir.add_wire("compout", wtype::OUT);
	simcir.add_gate({simcir.get_const0()}, out, fnct::BUF);

	simcir.write_bench(arg_vec[2]);

	return CMD_SUCCESS;
}

int neos_command_limit_fanin(int argc, char** argv) {
	using namespace ckt;
	circuit comp_cir;
	vector<string> arg_vec(argv, argv + argc);
	if (arg_vec.size() != 4) {
		std::cout << "usage: limit_fanin <input-bench> <output-bench> <sz-limit>\n";
		exit(EXIT_FAILURE);
	}

	circuit incir(arg_vec[1]);
	incir.limit_fanin_size(stoi(arg_vec[3]));
	incir.write_bench(arg_vec[2]);

	return CMD_SUCCESS;
}


int neos_command_symbolic(int argc, char** argv) {

	using namespace ckt;
	using namespace sym;

	if (argc != 2) {
		neos_error("usage: symbolic <circuit>");
	}

	vector<string> args(argv, argv + argc);

	sym_mgr_t mgr;

	circuit cir(args[1]);

	/*sym_var_boolvec_t x = mgr.new_boolvec_variable(cir.nInputs());
	sym_var_boolvec_t k = mgr.new_boolvec_variable(cir.nKeys());
	sym_var_boolvec_t y = apply_cir(x, k, cir);*/

	sym_var_boolvec_t x0 = mgr.new_boolvec_variable(4);
	sym_var_boolvec_t x1 = mgr.new_boolvec_variable(4);
	sym_var_boolvec_t y = ~(x0 & x1);

	y.write_graph();
	circuit rcir;
	Hmap<id, idvec> vmap;
	y.to_circuit(rcir, vmap);

	rcir.write_bench();

	return 0;
}



} // namespace

