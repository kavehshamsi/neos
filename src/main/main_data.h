/*
 * main_da.h
 *
 *  Created on: Feb 2, 2019
 *      Author: kaveh
 */

#ifndef SRC_MAIN_MAIN_DATA_H_
#define SRC_MAIN_MAIN_DATA_H_

#include <string>
#include <vector>

namespace mnf {

struct main_data {
	bool help = 0;
	bool interactive = 0;
	bool cmdmode = 0;
	bool encflag = 0;
	bool satmode = 0;
	bool equivalence = 0;
	bool use_verilog = 0;
	bool misc = 0;
	bool pstats = 0;
	int verbosity = 0;

	int timeout_minutes = -1;
	int memlimit_mgbs = 4000;
	int iter_cap = -1;

	std::string bin_oracle;
	std::string enc_method;
	std::string dec_method;
	std::vector<std::string> args;
	std::vector<std::string> pos_args;

};

} // namseapce mnf

#endif /* SRC_MAIN_MAIN_DATA_H_ */
