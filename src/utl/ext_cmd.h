/*
 * ckt_sat.h
 *
 *  Created on: Apr 30, 2016
 *      Author: kaveh
 */

#ifndef SRC_EXT_CMD_H_
#define SRC_EXT_CMD_H_

#include <boost/algorithm/string/replace.hpp>
#include "aig/aig.h"
#include "base/circuit.h"
#include <cstdlib>
#include <iomanip>

namespace ext {

namespace paths {
	extern std::string DUMP_DIR;
	extern std::string TEST_DIR;
	extern std::string ABC_CMD;
	extern std::string NUXMV_CMD;
	extern std::string ATPG_CMD;
	extern std::string ABC_SYNTH_LIB0;
	extern std::string ABC_SYNTH_LIB1;
	extern std::string SCRIPT_DIR;
}

using namespace ckt;
using namespace aig_ns;

extern bool ext_paths_initialized;

void get_rand_circuit(circuit& cir, int num_ins, int num_outs, float activity = 0.2);
int abc_resynthesize(circuit& ckt, int aiglib = 0, bool exit_on_crash = true, bool quiet = false);
std::string rename_luts(std::string& str);
void rename_capitalize(std::string& str);
void rename_decapitalize(std::string& str);

std::string create_temp_dir();
void remove_temp_dir();
int check_equivalence_abc(const aig_t& ntk1, const aig_t& ntk2, bool fill_missing_ports = true, bool quiet = false);
int check_equivalence_abc(const circuit& cir1, const circuit& cir2, bool fill_missing_ports = true, bool quiet = false);
void unroll_cir_abc(const circuit& cir, circuit& uncir, int numFrames);
void abc_run_commands(const std::string& cmd, const std::string& run_dir );
int32_t abc_simplify(circuit& cir, bool quiet = false);
void abc_simplify(aig_t& ntk, bool quiet = false);

std::vector<boolvec> atalanta(const circuit& ckt);

}

#endif /* SRC_EXT_CMD_H_ */
