/*
 * main_dec.cpp
 *
 *  Created on: Apr 4, 2020
 *      Author: kaveh
 */

#include <dec/dec_opt.h>
#include <main/main_util.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/resource.h>

#include "main.h"
#include "base/circuit.h"
#include "enc/enc.h"
#include "base/blocks.h"
#include "utl/cktgraph.h"
#include "utl/ext_cmd.h"
#include "dec/dec_sat.h"
#include "dec/mcdec.h"
#include "stat/stat.h"
#include "main/main_util.h"

namespace mnf {

int dec_main(int argc, char** argv) {

	main_data mn;
	parse_main_options(mn, argc, argv);

	//std::cout << "selected decryption mode\n";

	circuit sim_ckt, enc_ckt;
	boolvec corrkey;

	if (mn.pos_args.size() < 2 || mn.help) {
		neos_println("usage: neos -d <method> <sim_cir> <enc_cir> [iteration_limit]");
		return 1;
	}

	read_sim_enc_key_args(sim_ckt, enc_ckt, corrkey,
			mn.pos_args, mn.use_verilog, !mn.bin_oracle.empty());

	if (!enc_ckt.latches().empty()) {
		main_dec_seq(argc, argv);
	}
	else {
		main_dec_comb(argc, argv);
	}
	return 0;
}

int main_dec_seq(int argc, char** argv) {

	using namespace dec_ns;
	namespace po = boost::program_options;
	using std::cout;

	cout << "enc-ckt has DFFs. running sequnetial attack...\n";

	main_data mn;
	parse_main_options(mn, argc, argv);

	if ( mn.dec_method == "int" ) {
		dec_ns::mcdec_int_ckt dec_obj;
		if (dec_obj.parse_args(argc, argv)) return 1;
		dec_obj.solve();
	}
	else if ( mn.dec_method == "intaig" ) {
		dec_ns::mcdec_int_aig dec_obj;
		if (dec_obj.parse_args(argc, argv)) return 1;
		dec_obj.solve();
	}
	else if ( mn.dec_method == "nuxmv" ) {
		dec_ns::mcdec_nuxmv dec_obj;
		if (dec_obj.parse_args(argc, argv)) return 1;
		dec_obj.solve_nuxmv();
	}

	else if (mn.dec_method == "hill" || mn.dec_method == "bbo") {
		dec_ns::dec_opt dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else {
		neos_error("bad method " << mn.dec_method << ". options are {int, intaig, (nuxmv), hill}.");
	}

	return 0;

}

int main_dec_comb(int argc, char** argv) {

	auto start_time = utl::_start_wall_timer();

	using namespace mc;
	namespace po = boost::program_options;
	using std::cout;

	main_data mn;
	parse_main_options(mn, argc, argv);

	cout << "called combinatonal attack... with arg " << mn.dec_method << "\n";
	std::cout << utl::to_delstr(mn.pos_args, " ") << "\n";

	if ( mn.dec_method == "ex" ) {
		dec_ns::dec_sat dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if ( mn.dec_method == "app") {
		dec_ns::dec_sat_appsat dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if (mn.dec_method == "frg") {
		dec_ns::dec_sat_fraig dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if ( mn.dec_method == "kdip") {
		dec_ns::dec_sat_kdip dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if (mn.dec_method == "bbo") {
		dec_ns::dec_opt dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if ( mn.dec_method == "ucyc") {
		dec_ns::dec_sat_ucyc dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else if ( mn.dec_method == "besat") {
		dec_ns::dec_sat_besat dc;
		if (dc.parse_args(argc, argv)) return 1;
		dc.solve();
	}
	else {
		neos_error("unknown decryption method " << mn.dec_method);
	}

	std::cout << "time: " << utl::_stop_wall_timer(start_time) << "\n";

	return 0;

}


} // namespace mnf
