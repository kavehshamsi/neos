/*
 * cmd.h
 *
 *  Created on: Aug 6, 2019
 *      Author: kaveh
 */

#ifndef SRC_MAIN_CMDLN_H_
#define SRC_MAIN_CMDLN_H_

#include "base/base.h"
#include "base/circuit.h"
#include "aig/aig.h"
#include "main/main_data.h"
#include <functional>
#include <boost/variant.hpp>

namespace cmd {

using namespace ckt;
using namespace aig_ns;

// I hate forward declarations
class cmdln_manager_t;

// neos_cmd object to hold description/help/function-pointer
class neos_cmd {
public:
	string description;

	typedef std::function<int(int argc, char** argv)> neos_cmd_fun_t;
	neos_cmd_fun_t cmd_fun;

	neos_cmd() {}
	neos_cmd(const string& description, neos_cmd_fun_t cmd_fun) :
			description(description), cmd_fun(cmd_fun) {}

};

// the command line context is a list of the following objects
// context_objects have a type and arbitrary data, perhaps can be done better with variant/any
class cntx_obj {
public:
	boost::variant<circuit, aig_t, boolvec> vr;

	cntx_obj() {}
	cntx_obj(const boost::variant<circuit, aig_t, boolvec>& vr) : vr(vr) {}

	const circuit& as_cir() const { return boost::get<circuit>(vr); }
	circuit& as_cir() { return boost::get<circuit>(vr); }
	const aig_t& as_aig() const { return boost::get<aig_t>(vr); }
	aig_t& as_aig() { return boost::get<aig_t>(vr); }
	bool is_cir() const { return vr.which() == 0; }
	bool is_aig() const { return vr.which() == 1; }
	bool is_patt() const { return vr.which() == 2; }
	bool is_design() const { return is_cir() || is_aig(); }
	circuit to_cir() const;
	aig_t to_aig() const;
};

// neos cmd manager which stores all commands
class cmdln_manager_t {
public:
	vector<cntx_obj> data; // persistant context
	Omap<string, neos_cmd> cmd_map;

	std::shared_ptr<cell_library> cell_lib;

	uint cur_num_args = 0;

	uint selected_context = 0;

	string temp_dir;

	bool is_initalized : 1 = 0;

	cmdln_manager_t() {}

	// destructor is rather important as it needs to cleanup context
	~cmdln_manager_t();

	void init_commands();
	void create_temp_dir();
	void add_command(const string& cmd_nm, neos_cmd cm);
	void print_command_possibilities(const string& nm);
	int execute_script(const char* line);
	int execute_line(const string& cmd);
	circuit get_cir_design();
	cntx_obj* get_object_from_context();
	cntx_obj* get_design_from_context();

	bool has_context() { return !data.empty(); }

};

// interactive mode loop instantiates cmdln manager and operates it
// uses linenoise library instead of libreadline
int interactive_mode();

// command mode which will execute the command string on the shell
int command_mode(const mnf::main_data& mn);



// list of command definitions
// add new commands here as well as in the init_command function
int neos_command_help(int argc, char** argv);
int neos_command_select(int argc, char** argv);
int neos_command_read_bench(int argc, char** argv);
int neos_command_read_verilog(int argc, char** argv);
int neos_command_write_bench(int argc, char** argv);
int neos_command_write_verilog(int argc, char** argv);
int neos_command_list_context(int argc, char** argv);
int neos_command_list_directory(int argc, char** argv);
int neos_command_print_stats(int argc, char** argv);
int neos_command_clear(int argc, char** argv);
int neos_command_read_genlib(int argc, char** argv);
int neos_command_read_liberty(int argc, char** argv);
int neos_command_error_check(int argc, char** argv);
int neos_command_rename_wires(int argc, char** argv);
int neos_command_make_combinational(int argc, char** argv);
int neos_command_map(int argc, char** argv);
int neos_command_print_lib(int argc, char** argv);
int neos_command_to_aig(int argc, char** argv);
int neos_command_fraig(int argc, char** argv);
int neos_command_rewrite(int argc, char** argv);
int neos_command_prove0(int argc, char** argv);
int neos_command_draw_graph(int argc, char** argv);
int neos_command_enc(int argc, char** argv);
int neos_command_dec(int argc, char** argv);
int neos_command_hamm(int argc, char** argv);
int neos_command_gen_block(int argc, char** argv);
int neos_command_limit_fanin(int argc, char** argv);
int neos_command_gen_variants(int argc, char** argv);
int neos_command_aigeq(int argc, char** argv);
int neos_command_aig(int argc, char** argv);
int neos_command_aigtest(int argc, char** argv);
int neos_command_symbolic(int argc, char** argv);
int neos_command_blocks(int argc, char** argv);
int neos_command_npn(int argc, char** argv);
int neos_command_bal(int argc, char** argv);
int neos_command_sim(int argc, char** argv);
int neos_command_randsim(int argc, char** argv);
int neos_command_tsort(int argc, char** argv);
int neos_command_verilog2bench(int argc, char** argv);
int neos_command_rwr(int argc, char** argv);
int neos_command_rwrprac(int argc, char** argv);
int neos_command_equ(int argc, char** argv);
int neos_command_unroll(int argc, char** argv);
int neos_command_simptest(int argc, char** argv);
int neos_command_bmc(int argc, char** argv);
int neos_command_sail(int argc, char** argv);
int neos_command_univ(int argc, char** argv);
int neos_command_bdd(int argc, char** argv);
int neos_command_statgraph(int argc, char** argv);
int neos_command_compose(int argc, char** argv);
int neos_command_propagate_constants(int argc, char** argv);
int neos_command_selfsim_oless(int argc, char** argv);
int neos_command_sweep(int argc, char** argv);
int neos_command_ulearn(int argc, char** argv);
int neos_command_refsm(int argc, char** argv);
int neos_command_fourier(int argc, char** argv);
int neos_command_layers(int argc, char** argv);
int neos_command_statgraph(int argc, char** argv);
int neos_command_graph(int argc, char** argv);
int neos_command_is_comb(int argc, char** argv);
int neos_command_enc_absorb(int argc, char** argv);

} // namespace cmd


#endif /* SRC_MAIN_CMDLN_H_ */
