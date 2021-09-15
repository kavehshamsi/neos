/*
 * misc_util.cpp
 *
 *  Created on: Apr 22, 2021
 *      Author: kaveh
 */

#include "main/main_util.h"
#include "base/circuit.h"

namespace mnf {

std::string parse_simbin_arg(const std::string& arg) {

	std::string simbinstr = arg;
	std::string simbin;
	std::stringstream ss(simbinstr);
	std::getline(ss, simbinstr, '=');
	if (simbinstr != "simbin") {
		return simbin;
	}
	std::getline(ss, simbinstr);
	std::cout << "oracle binary at: \"" << simbinstr << "\" ";
	return simbinstr;

}

int parse_args(int argc, char** argv, const boost::program_options::options_description& op) {

	namespace po = boost::program_options;
	po::variables_map vm;

	po::positional_options_description pos_op;
	pos_op.add("pos_args", -1);

	try {
		auto res = po::command_line_parser(argc, argv).options(op).positional(pos_op).run();
		po::store(res, vm);
		po::notify(vm);
	}
	catch(po::error& e) {
		std::cout << e.what() << "\n";
		return 1;
	}

	return 0;
}

std::vector<bool> parse_key_arg(const std::string& arg) {

	std::string keystr = arg;
	std::stringstream ss(keystr);
	std::vector<bool> key;
	std::getline(ss, keystr, '=');
	if (keystr != "key") {
		return key;
	}
	std::getline(ss, keystr);

	for (auto& x : keystr)
		key.push_back((x == '0') ? 0:1);

	return key;

}

void read_sim_enc_key_args(ckt::circuit& sim_ckt, ckt::circuit& enc_ckt,
		std::vector<bool>& corrkey, const vector<std::string>& pos_args, bool use_verilog, bool bin_oracle) {

	if (pos_args.size() >= 2)
		corrkey = mnf::parse_key_arg(pos_args[1]);
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
		corrkey.resize(enc_ckt.nKeys());
	}

}

} // namespace


