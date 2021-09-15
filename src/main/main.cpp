
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <boost/program_options.hpp>

#include "main/main.h"
#include "base/circuit.h"
#include "enc/enc.h"
#include "base/blocks.h"
#include "dec/dec_sat.h"
#include "dec/mcdec.h"
#include "main/main_util.h"
#include "utl/cktgraph.h"
#include "utl/ext_cmd.h"
#include "cmd/cmdln.h"
#include "stat/stat.h"

int main(int argc, char **argv) {

	using namespace utl;

	mnf::main_data mn;
	parse_main_options(mn, argc, argv);

    if (mn.memlimit_mgbs != -1) {

    	rlimit limit, old;
    	pid_t pid = getpid();

    	limit.rlim_cur = ((rlim_t)mn.memlimit_mgbs) * 1e9;
    	limit.rlim_max = RLIM_INFINITY;
    	//std::cout << "mem limit: " << limit.rlim_cur << "\n";

    	if ( prlimit(pid, RLIMIT_AS, &limit, &old) == -1 )
    		neos_error("could not set rlimit");
    	//printf("Previous limits: soft=%lld; hard=%lld\n",
    			//(long long) old.rlim_cur, (long long) old.rlim_max);

    }

    if ( mn.interactive )
		return cmd::interactive_mode();

	else if ( mn.cmdmode )
		return cmd::command_mode(mn);

	else if ( mn.enc_method != "" )
		return mnf::enc_main(mn);

	else if ( mn.dec_method != "" )
		return mnf::dec_main(argc, argv);

	else if ( mn.equivalence )
		return mnf::equiv_main(mn);

	else if ( mn.pstats )
		return mnf::pstats_main(mn);

	else if ( mn.help ) {
		std::cout << "usage: neos [options] <positional_args>\n";
		std::cout << get_top_options(mn) << "\n";
	    return 0;
	}
	else {
		neos_error("incorrect options. try: neos -h");
	}

    return 0;
}

namespace mnf {

boost::program_options::options_description get_top_options(main_data& mn) {

	namespace po = boost::program_options;
	po::options_description top_parser("Allowed options");
	top_parser.add_options()
	    ("help,h", po::bool_switch(&mn.help), "see help")
		("int,i", po::bool_switch(&mn.interactive), "interactive mode")
		("cmd,c", po::bool_switch(&mn.cmdmode), "script execution mode (open interactive shell and run commands)")
	    ("enc,e", po::value<std::string>(&mn.enc_method)->implicit_value("undef"), "encryption/obfuscation mode")
		("dec,d", po::value<std::string>(&mn.dec_method)->implicit_value("undef"),
				"decryption/deobfuscation mode: <method> : decryption method")
		("eq,q", po::bool_switch(&mn.equivalence), "equivalence checker (key verification)")
		("misc,m", po::bool_switch(&mn.misc), "miscellaneous routines")
		("use_verilog", po::bool_switch(&mn.use_verilog), "use verilog as input and output")
		("verbose,v", po::value<int>(&mn.verbosity), "verbosity level")
		("cir_stats", po::bool_switch(&mn.pstats), "print circuit stats")
		("to", po::value<int>(&mn.timeout_minutes), "set timeout in minutes")
		("mo", po::value<int>(&mn.memlimit_mgbs), "set memory limit in MB")
		("bin_oracle", po::value<string>(&mn.bin_oracle), "use binary executable as oracel with format: oracle <binary_input> > <binary_output>")
		("pos_args",  po::value<std::vector<std::string> >(&mn.pos_args), "positional arguments");

	return top_parser;
}

int parse_main_options(main_data& mn, int argc, char** argv) {

	namespace po = boost::program_options;
	po::variables_map vm;

	po::options_description top_options = get_top_options(mn);

	po::positional_options_description posc;
	posc.add("pos_args", -1);

	try {
		po::store(po::command_line_parser(argc, argv).options(top_options).
				positional(posc).allow_unregistered().run(), vm);

		po::notify(vm);
	}
    catch(po::error& e) {
    	std::cout << e.what() << "\n\n" << get_top_options(mn);
    	return 1;
    }

    return 0;

}

int equiv_main(const main_data& mn) {

	if (mn.pos_args.size() != 3){
		std::cout << "usage : neos -q <orig file> "
				"<enc file> key=\n";
		return 0;
	}
	circuit sim_ckt, enc_ckt;

	if (mn.use_verilog) {
		sim_ckt.read_verilog(mn.pos_args[0]);
		enc_ckt.read_verilog(mn.pos_args[1]);
	}
	else {
		sim_ckt.read_bench(mn.pos_args[0]);
		enc_ckt.read_bench(mn.pos_args[1]);
	}
	//enc_ckt.fix_output();

	sim_ckt.error_check();
	enc_ckt.error_check();

	boolvec key = mnf::parse_key_arg(mn.pos_args[2]);

	dec_ns::decol csobj(sim_ckt, enc_ckt, boolvec());
	csobj.check_key(key);

	return 0;
}

int pstats_main(const main_data& mn) {

	if (mn.pos_args.size() != 1) {
		neos_error("usage: neos -p <circuit_file>");
	}
	circuit statckt;

	if (mn.use_verilog)
		statckt.read_verilog(mn.pos_args[0]);
	else
		statckt.read_bench(mn.pos_args[0]);

	statckt.error_check();

	//statckt.write_verilog();

	statckt.print_stats();

	return 0;
}

void print_key (std::vector<bool>& key) {

	std::cout << "key : ";
	for (auto x : key){
		 std::cout << x ;
	}
	std::cout << "\n";

}

}
