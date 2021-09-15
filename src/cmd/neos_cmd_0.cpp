/*
 * neos_cmd_0.cpp
 *
 *  Created on: Jan 2, 2021
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
#include "main/main_util.h"
#include "cmd/cmddefs.h"

namespace cmd {

extern std::unique_ptr<cmdln_manager_t> _cmdmgr_ptr;

int neos_command_draw_graph(int argc, char** argv) {

	using namespace cktgph;

	static int num_graphs = 0;

	if (_cmdmgr_ptr->data.empty()) {
		neos_println("no design available");
		return CMD_FAILURE;
	}

	auto& c = _cmdmgr_ptr->data[0];

	circuit g_cir;

	if (c.is_patt()) {
		neos_println("not a design");
		return CMD_FAILURE;
	}
	else if (c.is_aig()) {
		g_cir = c.as_aig().to_circuit();
	}
	else if (c.is_cir()) {
		g_cir = c.as_cir();
	}

	vector<string> args(argv + 1, argv + argc);

	string dot_file;
	if (args.empty()) {
		_cmdmgr_ptr->create_temp_dir();
		dot_file = "dot_file" + std::to_string(num_graphs++) + ".dot";
	}
	else {
		dot_file = args[1];
	}

	cktgraph gc(g_cir);
	gc.writeVizFile(dot_file);

	neos_system("xdot " + dot_file + " &");

	return CMD_SUCCESS;
}


int neos_command_list_context(int argc, char** argv) {
	uint i = 0;
	for (const auto& c : _cmdmgr_ptr->data) {
		if (c.is_cir()) {
			std::cout << "[" << i << "]: circuit : " << c.as_cir().top_name;
			c.as_cir().print_stats();
		}
		else if (c.is_aig()) {
			std::cout << "[" << i << "]: aig : " << c.as_aig().top_name;
			c.as_aig().print_stats();
		}
		else {
			neos_under_construction();
		}
		i++;
	}
	return CMD_SUCCESS;
}

int neos_command_list_directory(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	if (args.empty())
		neos_system("ls");
	else
		neos_system("ls " + args.at(1));

	return CMD_SUCCESS;
}


int neos_command_fraig(int argc, char** argv) {

	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (!c->is_aig()) {
		neos_println("design must be in AIG format for fraig");
		return CMD_FAILURE;
	}
	else {
		opt_ns::aig_satsweep(c->as_aig());
		return CMD_SUCCESS;
	}
}

int neos_command_prove0(int argc, char** argv) {
	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	aig_t miter;
	if (!c->is_aig()) {
		neos_println("converting miter to aig");
		miter = c->to_aig();
	}
	else {
		miter = c->as_aig();
	}

	id2boolHmap cex;
	if ( prove::prove_fraig0(miter, miter.outputs(), cex) )
		return CMD_SUCCESS;
	else
		return CMD_FAILURE;

}


int neos_command_to_aig(int argc, char** argv) {

	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (c->is_patt()) {
		neos_println("not a design");
		return CMD_FAILURE;
	}
	else if (c->is_aig()) {
		neos_println("design already an AIG");
		return CMD_FAILURE;
	}
	else {
		c->vr = c->to_aig();
		return CMD_SUCCESS;
	}

}

int neos_command_hamm(int argc, char** argv) {

	circuit cir = _cmdmgr_ptr->get_cir_design();
	if (cir.nWires() == 0) {
		return CMD_FAILURE;
	}

	if (cir.nKeys() == 0) {
		neos_println("circuit has no keys.");
		return CMD_FAILURE;
	}

	return CMD_FAILURE;
}

int neos_command_rewrite(int argc, char** argv) {

	if (_cmdmgr_ptr->data.empty()) {
		neos_println("no design available");
		return CMD_FAILURE;
	}

	auto& c = _cmdmgr_ptr->data.at(_cmdmgr_ptr->selected_context);

	if (!c.is_aig()) {
		neos_println("design must be in AIG format for rewrite");
		return CMD_FAILURE;
	}
	else {
		opt_ns::rewrite_manager_t rwr_mgr;
		rwr_mgr.rewrite_aig(c.as_aig());
		return CMD_SUCCESS;
	}
}

int neos_command_read_bench(int argc, char** argv) {
	vector<string> argvec(argv + 1, argv + argc);
	std::cout << "reading bench " << argvec.at(0) << "...\n";
	circuit cir;
	cir.read_bench(argvec.at(0));
	_cmdmgr_ptr->data.push_back(cntx_obj());
	_cmdmgr_ptr->data.back().vr = std::move(cir);
	return 0;
}

int neos_command_read_verilog(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	std::cout << "reading gate-level verilog " << args.at(0) << "...\n";
	circuit cir;
	cir.read_verilog(args.at(0));
	_cmdmgr_ptr->data.push_back(cntx_obj());
	_cmdmgr_ptr->data.back().vr = std::move(cir);
	return CMD_SUCCESS;
}

int neos_command_read_genlib(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	std::cout << "reading genlib " << args.at(0) << "...\n";
	_cmdmgr_ptr->cell_lib = std::make_shared<cell_library>();
	int res = prs::genlib::parse_genlib_file(args.at(0), *_cmdmgr_ptr->cell_lib);
	if (res == 0) {
		std::cout << "succesfully read genlib with " << _cmdmgr_ptr->cell_lib->cells.size() << " cells\n";
		return CMD_SUCCESS;
	}
	else
		return CMD_FAILURE;
}

int neos_command_read_liberty(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	std::cout << "reading liberty file " << args.at(0) << "...\n";
	_cmdmgr_ptr->cell_lib = std::make_shared<cell_library>();
	int res = prs::liberty::parse_liberty_file(args.at(0), *_cmdmgr_ptr->cell_lib);
	if (res == 0) {
		std::cout << "succesfully read liberty with " << _cmdmgr_ptr->cell_lib->cells.size() << " cells\n";
		return CMD_SUCCESS;
	}
	else
		return CMD_FAILURE;
}


int neos_command_print_lib(int argc, char** argv) {
	if (_cmdmgr_ptr->cell_lib) {
		_cmdmgr_ptr->cell_lib->print_lib();
	}
	else {
		std::cout << "no available cell library to print\n";
	}
	return CMD_SUCCESS;
}

int neos_command_map(int argc, char** argv) {
	if (!_cmdmgr_ptr->cell_lib) {
		std::cout << "no available cell library \n";
	}
	else {
		cntx_obj* ptr = _cmdmgr_ptr->get_design_from_context();
		if (!ptr->is_aig()) {
			std::cout << "design must be in aig format\n";
			return CMD_FAILURE;
		}
		else {
			tech_map_t tmp(ptr->as_aig(), *(_cmdmgr_ptr->cell_lib));
			tmp.map_to_lib(10);
			circuit cir;
			tmp.to_cir(cir);
			cir.write_verilog();
			return CMD_SUCCESS;
		}
	}
	return CMD_SUCCESS;
}

int neos_command_help(int argc, char** argv) {
	std::cout << "registered commands: \n";
	for (auto& pr : _cmdmgr_ptr->cmd_map) {
		std::cout << "  " << std::left << std::setw(20) << pr.first << pr.second.description << "\n";
	}
	return CMD_SUCCESS;
}

int neos_command_select(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	if (args.size() == 1) {
		neos_println_l1("current selected object index: " << _cmdmgr_ptr->selected_context);
		return CMD_SUCCESS;
	}
	else if (args.size() == 2) {
		int i = stoi(args.at(1));
		if (i >= _cmdmgr_ptr->data.size()) {
			neos_println("cannot select non-existent context object " << i);
			return CMD_FAILURE;
		}
		_cmdmgr_ptr->selected_context = i;
		return CMD_SUCCESS;
	}

	neos_println_l1("usage: select [context_index]");
	return CMD_FAILURE;
}

int neos_command_write_bench(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	const auto& d = _cmdmgr_ptr->data.at(_cmdmgr_ptr->selected_context);
	if (d.is_cir()) {
		if (args.size() != 0)
			d.as_cir().write_bench(args.at(0));
		else
			d.as_cir().write_bench();
	}
	else if (d.is_aig()) {
		if (args.size() != 0)
			d.as_aig().write_bench(args.at(0));
		else
			d.as_aig().write_bench();
	}
	return CMD_SUCCESS;
}

int neos_command_write_verilog(int argc, char** argv) {
	vector<string> args(argv + 1, argv + argc);
	const auto& d = _cmdmgr_ptr->data.at(_cmdmgr_ptr->selected_context);
	if (d.is_cir()) {
		if (args.size() != 0)
			d.as_cir().write_verilog(args.at(0));
		else
			d.as_cir().write_verilog();
	}
	else if (d.is_aig()) {
		if (args.size() != 0)
			d.as_aig().write_verilog(args.at(0));
		else
			d.as_aig().write_verilog();
	}
	return CMD_SUCCESS;
}


int neos_command_print_stats(int argc, char** argv) {

	if (_cmdmgr_ptr->data.empty()) {
		neos_println_l1("no design loaded");
		return CMD_SUCCESS;
	}

	if (argc != 1) {
		neos_println_l1("usage: pring_stats");
		return CMD_FAILURE;
	}
	vector<string> args(argv + 1, argv + argc);

	auto c = _cmdmgr_ptr->data.at(_cmdmgr_ptr->selected_context);

	if (c.is_cir()) {
		c.as_cir().print_stats();
	}
	else if (c.is_aig()) {
		c.as_aig().print_stats();
	}
	else {
		neos_println_l1("selected context is not a design");
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}


int neos_command_clear(int argc, char** argv) {
	if (argc != 1) {
		neos_println_l1("usage: pring_stats");
		return CMD_FAILURE;
	}
	_cmdmgr_ptr->data.clear();
	return CMD_SUCCESS;
}

int neos_command_error_check(int argc, char** argv) {
	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (c->is_aig()) {
		c->as_aig().error_check();
	}
	else if (c->is_cir()) {
		c->as_cir().error_check();
	}

	return 0;
}

int neos_command_rename_wires(int argc, char** argv) {
	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (c->is_aig()) {
		neos_println_l1("not supported for aigs");
		return CMD_FAILURE;
	}
	else if (c->is_cir()) {
		c->as_cir().rename_wires();
	}

	return 0;
}

int neos_command_randsim(int argc, char** argv) {
	if (argc != 2) {
		neos_println_l1("usage: randsim <num_patterns>");
		return CMD_FAILURE;
	}
	vector<string> args(argv + 1, argv + argc);

	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (c->is_aig()) {
		neos_println_l1("not supported for aigs");
		return CMD_FAILURE;
	}
	else if (c->is_cir()) {
		circuit::random_simulate(c->as_cir(), stoi(args[0]));
	}

	return 0;
}

int neos_command_make_combinational(int argc, char** argv) {
	cntx_obj* c = nullptr;
	if ( !(c = _cmdmgr_ptr->get_design_from_context()) ) {
		return CMD_FAILURE;
	}

	if (c->is_aig()) {
		neos_println_l1("not supported for aigs");
		return CMD_FAILURE;
	}
	else if (c->is_cir()) {
		circuit::make_combinational(c->as_cir());
	}

	return 0;
}



int neos_command_enc(int argc, char** argv) {

	using namespace enc;

	vector<string> args(argv, argv + argc);

	bool failed = false;
	//std::cout << args.size() << "\n";
	// check for help
	if (_is_in("-h", args) || args.size() <= 2) {
		enc_manager_t enc_mgr;
		std::cout << "usage: enc <lock_method> [args] \n";
		for (auto& p : enc_mgr.encryptor_map) {
			std::cout << "   " << std::left << std::setw(14) << p.first << " : " << p.second.description << "\n";
		}
		return CMD_FAILURE;
	}

	// check context
	if (!_cmdmgr_ptr->has_context()) {
		neos_println_l1("no available design");
		return CMD_FAILURE;
	}

	// get target context
	auto& ct = _cmdmgr_ptr->data.at(_cmdmgr_ptr->selected_context);

	// translate to circuit since all encryptors work on circuits only
	circuit sim_cir = ct.to_cir();

	// error check
	sim_cir.error_check();

	string enc_method = args[1];
	auto enc_specs = subvec(args, 2, args.size() - 1);
	std::cout << "enc method: " << enc_method << "\n";
	for (auto& spec : enc_specs) {
		std::cout << "spec: " << spec << "\n";
	}

	// find encryption command
	enc_manager_t enc_mgr;
	encryptor* enctor;
	try {
		enctor = &enc_mgr.encryptor_map.at(enc_method);
		bool correct_args = enctor->num_params.empty() ? true:false;
		uint asz = enc_specs.size();
		for (uint sz : enctor->num_params) {
			if (asz == sz) {
				correct_args = true;
			}
		}
		if (!correct_args) {
			neos_println(enctor->usage_message);
			return CMD_FAILURE;
		}
	}
	catch(std::out_of_range& e) {
		neos_println_l1("unknown encryption scheme " << enc_method << "\n");
		return CMD_FAILURE;
	}

	boolvec corrkey;
	circuit enc_cir = sim_cir;
	enctor->enc_fun(enc_specs, enc_cir, corrkey);

	if (failed) {
		neos_println_l1("could not encrypt " << enc_cir.top_name << "\n");
		return CMD_FAILURE;
	}


	std::cout << "encryption complete for " << enc_cir.top_name << "\n";
	std::cout << "number of inserted key-bits: "
					<< enc_cir.nKeys() << "\n";
	std::cout << "overhead: "
			<< ((float)enc_cir.nGates()
			/ (float)sim_cir.nGates()) - 1 << "\n";

	std::cout << "correct key is : " << utl::to_delstr(corrkey) << "\n";

	// replace current context with enc_cir
	ct.vr = enc_cir;
	// push sim_cir to back of context
	_cmdmgr_ptr->data.push_back(cntx_obj(sim_cir));

	return CMD_SUCCESS;
}

int neos_command_dec(int argc, char** argv) {

	vector<string> args(argv + 1, argv + argc);

	using namespace mnf;

	main_data mn;
	mnf::parse_main_options(mn, argc, argv);
	mn.dec_method = args.at(1);

	circuit enc_ckt, sim_ckt;
	boolvec corrkey;

	// check context
	if (!_cmdmgr_ptr->has_context()) {
		if (argc < 3) {
			neos_println_l1("usage: dec -d <method> <sim_cir> <enc_cir> [args]");
			neos_println_l1("usage: dec -d <method> <enc_cir> key=<corr_key> [args]");
			return CMD_FAILURE;
		}
		else {
			mnf::read_sim_enc_key_args(sim_ckt, enc_ckt, corrkey, mn.pos_args,
					mn.use_verilog, !mn.bin_oracle.empty());
		}
	}
	else {
		if (_cmdmgr_ptr->data.size() < 2) {
			enc_ckt = _cmdmgr_ptr->data.at(0).to_cir();
			sim_ckt = _cmdmgr_ptr->data.at(1).to_cir();
		}
		else {
			neos_println_l1("context does not include two circuits as enc_cir and sim_cir");
			return CMD_FAILURE;
		}
	}


	//std::cout << "selected decryption mode\n";

	sim_ckt.error_check();
	enc_ckt.error_check();

	argv++; argc--;

	if (!enc_ckt.latches().empty()) {
		mnf::main_dec_seq(argc, argv);
	}
	else {
		mnf::main_dec_comb(argc, argv);
	}
	return CMD_SUCCESS;

}


} // namespace cmd


