/*
 * misc_util.h
 *
 *  Created on: Dec 14, 2020
 *      Author: kaveh
 */

#ifndef SRC_MISC_MISC_UTIL_H_
#define SRC_MISC_MISC_UTIL_H_

#include <vector>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>

namespace ckt {
	class circuit;
}

namespace mnf {

std::vector<bool> parse_key_arg(const std::string& arg);

void read_sim_enc_key_args(ckt::circuit& sim_ckt, ckt::circuit& enc_ckt,
		std::vector<bool>& corrkey, const std::vector<std::string>& pos_args, bool use_verilog, bool bin_oracle);
int parse_args(int argc, char** argv, const boost::program_options::options_description& op);

}


#endif /* SRC_MISC_MISC_UTIL_H_ */
