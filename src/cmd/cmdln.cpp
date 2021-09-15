/*
 * cmd.cpp
 *
 *  Created on: Aug 7, 2019
 *      Author: kaveh
 */

#include "cmd/cmdln.h"
#include "enc/enc.h"
#include "dec/mcdec.h"
#include "dec/dec_sat.h"
#include "opt/rwr.h"
#include "aig/map.h"
#include "sat/prove.h"
#include "main/main.h"
#include "parser/parser.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <stat/stat.h>
#include <utl/cktgraph.h>
#include <utl/ext_cmd.h>
#include <sstream>

#define NEOS_READLINE

#ifdef NEOS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif


namespace cmd {

using namespace enc;
using namespace aig_ns;
using namespace ext;

#define FATAL_ERROR -1
#define CMD_EXIT 2
#define CMD_FAILURE 1
#define CMD_SUCCESS 0

// global pointer for command-line context
std::unique_ptr<cmdln_manager_t> _cmdmgr_ptr;

void cmdln_manager_t::init_commands() {
	add_command("help", 		 neos_cmd("show help information", neos_command_help) );
	add_command("select", 		 neos_cmd("select object from current context", neos_command_select) );
	add_command("read_bench", 	 neos_cmd("reads in bench file", neos_command_read_bench) );
	add_command("read_verilog",  neos_cmd("reads in bench file", neos_command_read_verilog) );
	add_command("write_bench", 	 neos_cmd("writes out bench file", neos_command_write_bench) );
	add_command("write_verilog", neos_cmd("writes out verilog file", neos_command_write_verilog) );
	add_command("error_check",   neos_cmd("check read netlist for errors", neos_command_error_check) );
	add_command("lsc", 			 neos_cmd("lists current context", neos_command_list_context) );
	add_command("ls", 			 neos_cmd("lists directory", neos_command_list_directory) );
	add_command("print_stats",   neos_cmd("print design stats", neos_command_print_stats) );
	add_command("clear", 		 neos_cmd("clears this session's data", neos_command_clear) );
	add_command("read_liberty",  neos_cmd("reads in liberty cell library", neos_command_read_genlib) );
	add_command("map", 			 neos_cmd("map to cell library", neos_command_map) );
	add_command("print_lib", 	 neos_cmd("print cell library if available", neos_command_print_lib) );
	add_command("to_aig", 		 neos_cmd("convert design to 1-level hashed aig", neos_command_to_aig) );
	add_command("fraig", 		 neos_cmd("get functionally reduced AIG (fast SAT-sweep algorithm)", neos_command_fraig) );
	add_command("rewrite", 		 neos_cmd("DAG-Aware AIG rewriting", neos_command_rewrite) );
	add_command("prove0", 		 neos_cmd("prove combinational mitter with fraig/rewriting algorithm-0", neos_command_prove0) );
	add_command("draw_graph", 	 neos_cmd("create .dot graph and possibly display it", neos_command_draw_graph) );
	add_command("enc", 			 neos_cmd("circuit obfuscation", neos_command_enc) );
	add_command("dec", 			 neos_cmd("circuit deobfuscation", neos_command_dec) );
	add_command("hamm", 		 neos_cmd("test hamming distance", neos_command_hamm) );
	add_command("bdd", 		     neos_cmd("convert circuit to bdd and print stats", neos_command_bdd) );

	add_command("rename_wires",  neos_cmd("", neos_command_rename_wires) );
	add_command("make_combinational",  neos_cmd("", neos_command_make_combinational) );
	add_command("aigeq", 		 neos_cmd("", neos_command_aigeq) );
	add_command("blocks", 		 neos_cmd("", neos_command_blocks) );
	add_command("limit_fanin", 	 neos_cmd("", neos_command_limit_fanin) );
	add_command("gen_block",  	 neos_cmd("", neos_command_gen_block) );
	add_command("sim", 			 neos_cmd("", neos_command_sim) );
	add_command("randsim", 	     neos_cmd("", neos_command_randsim) );
	add_command("bal", 			 neos_cmd("", neos_command_bal) );
	add_command("rwr", 			 neos_cmd("", neos_command_rwr) );
	add_command("verilog2bench", neos_cmd("", neos_command_verilog2bench) );
	add_command("unroll", 		 neos_cmd("", neos_command_unroll) );
	add_command("bmc", 			 neos_cmd("", neos_command_bmc) );
	add_command("is_comb", 		 neos_cmd("", neos_command_is_comb) );
	add_command("simptest", 	 neos_cmd("", neos_command_simptest) );
	add_command("statgraph", 	 neos_cmd("", neos_command_statgraph) );
	add_command("refsm", 		 neos_cmd("", neos_command_refsm) );
	add_command("aigtest", 		 neos_cmd("", neos_command_aigtest) );
	add_command("propagate_constants", neos_cmd("", neos_command_propagate_constants) );
	add_command("symbolic", 	 neos_cmd("", neos_command_symbolic) );
	add_command("sweep", 		 neos_cmd("", neos_command_sweep) );
	add_command("sail", 		 neos_cmd("", neos_command_sail) );
	add_command("selfsim_oless", neos_cmd("", neos_command_selfsim_oless) );
	add_command("ulearn", 		 neos_cmd("", neos_command_ulearn) );
	add_command("univ", 		 neos_cmd("", neos_command_univ) );
	add_command("layers", 		 neos_cmd("", neos_command_layers) );
	add_command("graph", 		 neos_cmd("", neos_command_graph) );

}


#ifndef NEOS_READLINE
void completion(const char *buf, linenoiseCompletions *lc) {
	string str(buf);
	auto *m = _cmdmgr_ptr;

	if (m->cur_num_args == 0) {
		auto it = m->cmd_map.lower_bound(str);
		vector<string> possibs;
		while (it != m->cmd_map.end() && it->first.substr(0, str.size()) == str) {
			possibs.push_back(it->first);
			it++;
		}

		if (possibs.size() == 1) {
			it--;
			linenoiseAddCompletion(lc, it->first.c_str());
		}
		else if (possibs.size() > 1) {
			uint n = 1;
			for (auto& x : possibs) {
				std::cout << x << "   ";
				if (n++ % 5 == 0)
					std::cout << "\n";
			}
			std::cout << "\n\rneos> " << str;
			std::cout.flush();
		}
		std::cout.flush();
	}
}

char* hints(const char *buf, int *color, int *bold) {
    if (!strcasecmp(buf,"hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}
#endif

char *
readline_command_generator (const char *text,
     int state) {

	auto& p = _cmdmgr_ptr;
	static auto it = p->cmd_map.begin();

	/* If this is a new word to complete, initialize now.  This includes
	 saving the length of TEXT for efficiency, and initializing the index
	 variable to 0. */
	if (state == 0) {
	  it = p->cmd_map.begin();
	}

	string text_str(text);
	while (it != p->cmd_map.end()) {
		// std::cout << it->first << " \n";
	  if (it->first.substr(0, text_str.size()) == text_str) {
		  return strdup((char*)it++->first.c_str());
	  }
	  it++;
	}

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

char **readline_completion (const char *text, int start, int end) {

	char **matches;

	matches = (char **)NULL;

	/* If this word is at the start of the line, then it is a command
	 to complete.  Otherwise it is the name of a file in the current
	 directory. */
	if (start == 0)
		matches = rl_completion_matches(text, readline_command_generator);

	return (matches);
}

int command_mode(const mnf::main_data& mn) {

	neos_println("Starting neos in command mode. (Compiled on " << __DATE__ << " at " << __TIME__")\n");

	const auto& p = _cmdmgr_ptr = std::make_unique<cmdln_manager_t>();
	_cmdmgr_ptr->init_commands();

	return p->execute_script(mn.pos_args[0].c_str());

}

int interactive_mode() {

	neos_println("Starting neos in interactive mode. (Compiled on " << __DATE__ << " at " << __TIME__")\n");

	const auto& p = _cmdmgr_ptr = std::make_unique<cmdln_manager_t>();
	p->init_commands();


#ifndef NEOS_READLINE
	char *line;
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    // load possible history
    linenoiseHistoryLoad("neos_history.txt");

    while((line = linenoise("neos> ")) != NULL) {
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            printf("echo: '%s'\n", line);
            int ret = mgr.execute_script(line);
            if (ret == -2) { // fatal error exiting
            	std::cout << "fatal error ending session;\n";
            	break;
            }
            else if (ret == -1) {
            	std::cout << "quiting\n";
            	break;
            }
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave("neos_history.txt"); /* Save the history on disk. */
        } else if ( !strncmp(line,"/historylen",11) ) {
            /* The "/historylen" command will change the history len. */
            int len = atoi(line+11);
            linenoiseHistorySetMaxLen(len);
        } else if (line[0] == '/') {
            printf("Unreconized command: %s\n", line);
        }

        free(line);
    }
#else

    rl_attempted_completion_function = readline_completion;

    char* buf;
    while ((buf = readline("neos> ")) != nullptr) {
		if (strlen(buf) > 0) {
			int ret = p->execute_script(buf);
			if (ret == -2) { // fatal error exiting
				std::cout << "fatal error. ending session;\n";
				break;
			}
			else if (ret == CMD_EXIT) {
				break;
			}

			//printf("[%s]\n", buf);
			add_history(buf);
		}

		// readline malloc's a new buffer every time.
		free(buf);
    }


#endif

    return 0;
}

circuit cntx_obj::to_cir() const {
	if (is_cir())
		return as_cir();
	else if (is_cir())
		return as_aig().to_circuit();
	else {
		neos_println("object is not a design");
		return circuit();
	}
}

aig_t cntx_obj::to_aig() const {
	if (is_aig())
		return as_aig();
	else if (is_cir())
		return aig_t(as_cir());
	else {
		neos_println("object is not a design");
		return aig_t();
	}
}

int cmdln_manager_t::execute_script(const char* scr) {

	string script_str(scr);
	boost::trim(script_str);
	// break line by semicolon
	vector<string> lines;
	boost::split(lines, script_str, boost::is_any_of(";"));

	for (auto& cmd : lines) {

		// trim commands and execute non-empty ones
		boost::trim(cmd);

		if (cmd.empty())
			continue;

		std::cout << cmd << "\n";

		int ret = execute_line(cmd);
		if (ret == CMD_EXIT) {
			std::cout << "exiting\n";
			return CMD_EXIT;
		}
		else if (ret == FATAL_ERROR) {
			std::cout << "\nfatal error during command execution. exiting.\n";
			return CMD_EXIT;
		}
	}

	return CMD_SUCCESS;
}

int cmdln_manager_t::execute_line(const string& cmd) {
	// sepearate args by space and tab
	vector<string> args;
	boost::split(args, cmd, boost::is_any_of("\t "));
	int argc = args.size();
	std::vector<char*> cstrings;
	for (size_t i = 0; i < argc; i++) {
		cstrings.push_back( const_cast<char*>(args[i].c_str()) );
	}
	char** argv = cstrings.data();

	if (args.empty()) {
		std::cout << "problem parsing command arguments";
		return CMD_FAILURE;
	}

	string cmd_name = args[0];
	// args.erase(args.begin());

	// for (auto& arg : args) std::cout << "arg: " << arg << "\n";

	// quit is a special case
	if (args[0] == "quit" || args[0] == "exit")
		return CMD_EXIT;

	auto it = cmd_map.find(cmd_name);
	if (it == cmd_map.end()) {
		std::cout << "command \"" << cmd_name << "\" not found\n";
		return CMD_FAILURE;
	}
	else {
		// call function pointer
		return it->second.cmd_fun(argc, argv);
	}
}


circuit cmdln_manager_t::get_cir_design() {
	circuit retcir;
	if (data.empty()) {
		neos_println("no design available");
		return retcir;
	}
	return data.at(_cmdmgr_ptr->selected_context).to_cir();
}

cntx_obj* cmdln_manager_t::get_object_from_context() {

	cntx_obj* c = nullptr;

	if (data.empty()) {
		neos_println("no design available");
		return c;
	}

	try {
		c = &(data.at(selected_context));
	}
	catch (std::out_of_range&) {
		neos_println("selected context " << selected_context << " does not exist");
		return nullptr;
	}

	return c;
}

cntx_obj* cmdln_manager_t::get_design_from_context() {

	cntx_obj* c = get_object_from_context();
	if (!c) {
		return nullptr;
	}

	if (!c->is_design()) {
		neos_println("selected context " << selected_context << " is not a design");
		return nullptr;
	}

	return c;
}

void cmdln_manager_t::create_temp_dir() {
	if (temp_dir.empty()) {
		temp_dir = ext::create_temp_dir();
	}
}

void cmdln_manager_t::add_command(const string& cmd_nm, neos_cmd cm) {
	cmd_map[cmd_nm] = cm;
}

cmdln_manager_t::~cmdln_manager_t() {
	// remove temp dir
	if (!temp_dir.empty())
		remove_temp_dir();
}


} // namespace mnf


