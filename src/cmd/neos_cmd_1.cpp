/*
 * misc1.cpp
 *
 *  Created on: Nov 24, 2020
 *      Author: kaveh
 */

#include "dec/dec_oless_simil.h"
#include "dec/dec_opt.h"
#include "main/main_util.h"
#include "base/circuit.h"
#include "aig/aig.h"
#include "utl/ext_cmd.h"
#include "base/blocks.h"
#include "aig/aig.h"
#include "opt/opt.h"
#include "opt/rwr.h"
#include "parser/genlib/readgenlib.h"
#include "parser/liberty/readliberty.h"
#include "opt/rwr.h"
#include "opt/bal.h"
#include "utl/npn.h"
#include "mc/unr.h"
#include "mc/bmc.h"
#include "enc/enc.h"
#include "misc/refsm.h"
#include "dec/dec_sat.h"
#include "dec/sail.h"
#include "stat/stat.h"
#include "base/blocks.h"

#include <filesystem>

namespace cmd {

using namespace ckt;
using namespace aig_ns;

int neos_command_aigeq(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	auto sim_ckt2 = sim_ckt;
	aig_ns::aig_t sim_aig(sim_ckt);
	sim_aig.write_aig();
	sim_aig.write_bench("sim.bench");
	ext::check_equivalence_abc(sim_aig.to_circuit(), sim_ckt);
	return 0;
};


int neos_command_blocks(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit bcir = ckt_block::bit_counter(stoi(arg_vec[1]));
	/*
	circuit cir;
	int rhs = std::stoi(arg_vec[1]);
	vector<bool> sgns(std::stoi(arg_vec[2]), 0);
	csat::lin_constr_cir_bdd(cir, sgns, rhs, csat::constr_op_t::CONSTR_GEQ);
	csat::lin_constr_cir_bdd(cir, sgns, rhs, csat::constr_op_t::CONSTR_LEQ);
	csat::lin_constr_cir_bdd(cir, sgns, rhs, csat::constr_op_t::CONSTR_EQ);*/

	circuit cir = ckt_block::bit_odd_even_sorter(stoi(arg_vec[1]));

	return 0;
}


int neos_command_npn(int argc, char** argv) {
    using namespace utl_npn;

    npn_enumerator<4> npt(1);
    using truth_t = npn_enumerator<4>::truth_t;
    uint16_t x = 2;
    npt.init(x);
    uint i = 0;
    for (truth_t ntt = npt.init(x); npt.next_perm(ntt); ) {
        std::cout << ntt << "  ";
        std::cout << ++i << "\n";
    }

    npt.init(x);
    std::unordered_set<npn_enumerator<4>::truth_t> hset;
    uint32_t npn_class = 0;
    for (uint16_t x = 0; x != 1 << 15; x++) {
        if (hset.find(truth_t(x)) == hset.end()) {
            std::cout << "found new table " << ++npn_class << "\n";
            for (npn_enumerator<4>::truth_t ntt = npt.init(x); npt.next_npn(ntt); ) {
                hset.insert(ntt);
            }
        }
    }

    dyn_npn_enumerator dnpt(4, 1);
    using dtruth_t = dyn_npn_enumerator::truth_t;
    x = 2;
    i = 0;
    for (dtruth_t ntt = dnpt.init(dtruth_t(16, x)); dnpt.next_perm(ntt); ) {
        std::cout << ntt << "  ";
        std::cout << i++ << "\n";
    }

    dnpt.init(dtruth_t(16, x));
    std::unordered_set<dtruth_t> dhset;
    dtruth_t xtt(16, x);
    npn_class = 0;
    for (uint16_t x = 0; x != 1 << 15; x++) {
        if (dhset.find(xtt) == dhset.end()) {
            std::cout << "found new table " << ++npn_class << "\n";
            for (dtruth_t ntt = dnpt.init(dtruth_t(16, x)); dnpt.next_npn(ntt); ) {
                dhset.insert(ntt);
            }
        }
    }

    return 0;
}
int neos_command_bal(int argc, char** argv) {
	using namespace opt_ns;
	circuit sim_cir;
	vector<string> arg_vec(argv, argv + argc);
	sim_cir.read_bench(arg_vec[1]);
	aig_t sim_aig = sim_cir;
	balance_manager_t balmgr(sim_aig);
	balmgr.balance_aig();
	return 0;
}

int neos_command_sim(int argc, char** argv) {
	id2boolHmap simmap;
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	sim_ckt.read_bench(arg_vec[1]);
	sim_ckt.add_const_to_map(simmap);
	foreach_inid(sim_ckt, xid)
		simmap[xid] = 0;
	foreach_keyid(sim_ckt, kid)
		simmap[kid] = 0;

	idvec gate_order = sim_ckt.get_topsort_gates();
	for (auto gid : gate_order) {
		for (auto gin : sim_ckt.gfanin(gid)) {
			assert(_is_in(gin, simmap));
		}
		simmap[sim_ckt.gfanout(gid)] = 0;
	}

	return 0;
}

int neos_command_tsort(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_cir;
	sim_cir.read_bench(arg_vec[1]);
	for (auto gid : sim_cir.get_topsort_gates()) {
		std::cout << sim_cir.getcgate(gid).level << "\n";
	}
	return 0;
}

int neos_command_verilog2bench(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	if (argc != 3) {
		neos_error("usage: verilog2bench <input_verilog> <output_bench>");
	}
	sim_ckt.read_verilog(arg_vec[1]);
	sim_ckt.write_verilog();
	sim_ckt.resolve_cell_directions({"ZN", "Y", "Z"});
	sim_ckt.translate_cells_to_primitive_byname();
	sim_ckt.write_bench();
	sim_ckt.write_bench(arg_vec[2]);
	return 0;
}

int neos_command_rwr(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	int f = stoi(arg_vec[2]);
	aig_t sim_aig(sim_ckt);
	opt_ns::rewrite_manager_t mgr(f);
	mgr.rewrite_aig(sim_aig);
	//mgr.rewrite_aig(sim_aig);
	//mgr.rewrite_aig(sim_aig);
	return 0;
}

int neos_command_rwrprac(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	int f = stoi(arg_vec[2]);
	opt_ns::rewrite_manager_t mgr(f);
	mgr.find_practical_classes(arg_vec[1]);
	return 0;
}

int neos_command_unroll(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	int num_rounds = stoi(arg_vec[2]);

	sim_ckt.read_bench(arg_vec[1]);
	aig_t sim_aig(sim_ckt);
	mc::CircuitUnroll cunr;
	cunr.set_tr(sim_ckt, sim_ckt.keys(), false);
	cunr.extend_to(num_rounds);

/*
	for (int i = 0; i < num_rounds; i++) {
		cunr.add_frame();
		cunr.sweep_unrolled_ckt_abc();
		cunr.uncir.write_bench("uncir.bench");
		if (!cunr.uncir.error_check()) {
			exit(0);
		}
	}
*/

	cunr.uncir.write_bench();
	circuit unroll_ckt;
	mc::_unroll_ckt_old(sim_ckt, unroll_ckt, num_rounds, sim_ckt.keys(), 0);
	unroll_ckt.write_bench("uncir_old.bench");
	assert(ext::check_equivalence_abc(cunr.uncir, unroll_ckt));

/*
	mc::AigUnroll aunr(sim_aig);
	aunr.frozen_tr = sim_aig.keys();
	for (int i = 0; i < num_rounds; i++) {
		aunr.add_frame();
		aunr.sweep_unrolled_aig();
		std::cout << "at frame " << i << "\n";
	}
	//cunr.unaig.write_bench();
	circuit unroll_ckt;
	mc::_unroll_ckt_old(sim_ckt, unroll_ckt, num_rounds, sim_ckt.keys(), 0);
	assert(ext::check_equivalence_abc(aunr.unaig.to_circuit(), unroll_ckt));
*/

	/*aig_t sim_aig(sim_ckt);
	//sim_aig.write_bench();
	assert(ext::check_equivalence_abc(sim_aig.to_circuit(), sim_ckt) );
	sim_aig.write_aig();
	aig_t unroll_sim_aig;
	circuit unroll_sim_ckt;
	circuit unroll_sim_ckt_abc;
	mc::_unroll_aig(sim_aig, unroll_sim_aig, num_rounds, sim_aig.keys(), 1);
	mc::_unroll_ckt_old(sim_ckt, unroll_sim_ckt, num_rounds, sim_ckt.keys(), 1);
	//ext::unroll_cir_abc(sim_ckt, unroll_sim_ckt_abc, num_rounds);
	//assert(ext::check_equivalence_abc(unroll_sim_ckt, unroll_sim_ckt_abc));
	unroll_sim_aig.write_aig();
	assert(ext::check_equivalence_abc(unroll_sim_aig.to_circuit(), unroll_sim_ckt));*/
	return 0;
}

int neos_command_bmc(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	auto tm = utl::_start_wall_timer();
	circuit sim_ckt;
	sim_ckt.read_bench(arg_vec[1]);
	mc::BmcCkt bmcsolver(sim_ckt, sim_ckt.keys(), mc::Bmc::Bmc_config_t());
	bmcsolver.bmc_solve(20);
	std::cout << "time: " << utl::_stop_wall_timer(tm) << "\n";
	return 0;
}

int neos_command_is_comb(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	dec_ns::dec_sat::is_combinational(sim_ckt, id2boolmap());
	return 0;
}

int neos_command_simptest(int argc, char** argv) {
	/*int num_patts = stoi(arg_vec[1]);
	int num_cirs = stoi(arg_vec[2]);*/
	//int num_ins = stoi(arg_vec[1]);
	//int num_outs = stoi(arg_vec[2]);
	//int num_cir = 0, num_patt = 0;
	srand(clock());
	/*while (num_cir++ < num_cirs) {
		num_patt = 0;
		circuit cir;
		ext::get_rand_circuit(cir, num_ins, num_outs);
		cir.write_bench();
		while (num_patt++ < num_patts) {
			circuit tie_cir = cir;
			id2boolmap constmap;
			for (auto wid : tie_cir.inputs()) {
				constmap[wid] = (rand() % 2 == 1);
			}
			tie_cir.simplify(constmap);
		}
	}*/
	vector<string> arg_vec(argv, argv + argc);
	circuit cir;
	cir.read_bench(arg_vec[1]);
	//ext::get_rand_circuit(cir, num_ins, num_outs);
	aig_t cir_aig(cir);
	cir_aig.write_bench();

	//aig_t tie_aig = cir_aig;
	id2boolHmap constmap;
	for (auto wid : cir_aig.inputs()) {
		if (rand() % 2 == 1)
			constmap[wid] = (rand() % 2 == 1);
	}
	cir_aig.propagate_constants(constmap);
	std::cout << "done propagation\n";
	/*while (num_cir++ < num_cirs) {
		num_patt = 0;
		circuit cir;
		ext::get_rand_circuit(cir, num_ins, num_outs);
		aig_t cir_aig(cir);
		cir_aig.write_bench();
		assert(ext::check_equivalence_abc(cir_aig.to_circuit(), cir));
		while (num_patt++ < num_patts) {
			aig_t tie_aig = cir_aig;
			id2boolmap constmap;
			for (auto wid : cir_aig.inputs()) {
				if (rand() % 2 == 1)
					constmap[wid] = (rand() % 2 == 1);
			}
			tie_aig.propagate_constants(constmap);
		}
	}*/
	return 0;
}

int neos_command_aigtest(int argc, char** argv) {
	using namespace aig_ns;
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	for (auto arg: arg_vec) {
		std::cout << arg << "\n";
	}
	sim_ckt.read_bench(arg_vec[1]);
	aig_ns::aig_t sim_aig(sim_ckt);
	//sim_aig.write_aig();
	assert(sim_aig.error_check());
	//sim_aig.write_bench();
	ext::check_equivalence_abc(sim_ckt, sim_aig.to_circuit());
	circuit sim_ckt2 = sim_aig.to_circuit();

	sim_aig.write_aig(arg_vec[2]);
	aig_t sim_aig2;
	sim_aig2.read_aig(arg_vec[2]);
	ext::check_equivalence_abc(sim_aig, sim_aig2);
	// sim_ckt2.write_bench();
	//cktgph::cktgraph cktgp_sim1(sim_ckt2);
	//cktgp_sim1.writeVizFile(arg_vec[2]);
	return 0;
}

int neos_command_refsm(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	re::refsm refobj(sim_ckt);
	refobj.enumerate_fsm(sim_ckt.latches(), stoi(arg_vec[2]));
	return 0;
}

int neos_command_bdd(int argc, char** argv) {
	circuit cir;
	vector<string> arg_vec(argv, argv + argc);
	cir.read_bench(arg_vec[1]);
	int szl = stoi(arg_vec[2]);

	std::cout << "size limit is " << szl << "\n";
	dd::circuit_bdd cktbdd(cir, szl);
	if (!cktbdd.isGood()) {
		std::cout << "problem during bdd generation\n";
		return 0;
	}
	std::cout << "bdd statistics: \n";
	for (auto oid : cir.outputs()) {
		std::cout << "for out " << cir.wname(oid) << " num nodes: ";
		std::cout << cktbdd.num_nodes(oid) << "\n";
	}
	cktbdd.write_to_dot("./bench/testing/bdd.dot");
	circuit bddcir;
	cktbdd.to_circuit(bddcir);
	bddcir.print_stats();

	return 0;
}

int neos_command_compose(int argc, char** argv) {
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	circuit ckt1 = sim_ckt;
	circuit ckt2;
	ckt2.read_bench(arg_vec[2]);
	uint i = ckt2.nKeys();
	oidset inputs = ckt2.inputs();
	for (auto inid : inputs) {
		ckt2.setwirename(inid, "keyinput" + std::to_string(i++));
		ckt2.setwiretype(inid, wtype::KEY);
	}
	ckt2.write_bench();
	id2idmap cir2new_wmap;
	ckt1.add_circuit_byname(ckt2, "", cir2new_wmap);
	auto it = sim_ckt.outputs().begin();

	foreach_outid(ckt2, oid) {
		id out1 = ckt1.find_wcheck(ckt2.wname(oid));
		id out2 = cir2new_wmap.at(oid);
		std::cout << ckt1.wname(out1) << " " << ckt1.wname(out2) << "\n";
		ckt1.join_outputs({out2, out1}, fnct::XOR);
		it++;
	}
	ckt1.write_bench(arg_vec[3]);
	return 0;
}


int neos_command_statgraph(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	sim_ckt.read_bench(arg_vec[1]);
	stat_ns::sig_prob_graph(sim_ckt, stoi(arg_vec[3]), arg_vec[2]);
	return 0;
}

int neos_command_propagate_constants(int argc, char** argv) {
	std::cout << "simplify mode\n";
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[1]);
	id2boolHmap inmap;
	foreach_inid(sim_ckt, inid) {
		inmap[inid] = 0;
	}
	sim_ckt.propagate_constants(inmap);
	sim_ckt.write_bench();
	return 0;
}

int neos_command_sweep(int argc, char** argv) {
	std::cout << "sweep selected\n";
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);
	sim_ckt.read_bench(arg_vec[2]);

	if (arg_vec[1] == "sataig") {
		aig_t sim_aig = aig_ns::aig_t(sim_ckt);
		aig_t sim_aig_swept = sim_aig;
		opt_ns::aig_satsweep(sim_aig_swept);
		assert(ext::check_equivalence_abc(sim_aig.to_circuit(), sim_aig_swept.to_circuit()));
	}
	else if (arg_vec[1] == "abc") {
		ext::abc_simplify(sim_ckt);
	}
	else if (arg_vec[1] == "satcir") {
		circuit sim_ckt_swept = sim_ckt;
		opt_ns::circuit_satsweep(sim_ckt_swept);
		assert(ext::check_equivalence_abc(sim_ckt, sim_ckt_swept));
	}
	else if (arg_vec[1] ==  "bdd") {
		circuit sim_ckt_swept = sim_ckt;
		opt_ns::circuit_bddsweep(sim_ckt_swept, 250);
		assert(ext::check_equivalence_abc(sim_ckt_swept, sim_ckt));
	}
	else if (arg_vec[1] == "cut") {
		aig_t sim_aig = aig_ns::aig_t(sim_ckt);
		aig_t sim_aig_swept = sim_aig;
		opt_ns::aig_cutsweep(sim_aig_swept);
		assert(ext::check_equivalence_abc(sim_aig.to_circuit(), sim_aig_swept.to_circuit()));
	}
	return 0;
}

int neos_command_sail(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	sim_ckt.read_bench(arg_vec[1]);
	int num_keys = stoi(arg_vec[2]);
	misc::sail(sim_ckt, num_keys);
	return 0;
}

int neos_command_selfsim_oless(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	if (arg_vec.size() != 6)
		neos_error("usage: oless <sim_ckt> <enc_ckt> <cut_size> <cut_num> <cut_key_size>");

	sim_ckt.read_bench(arg_vec[1]);
	circuit enc_ckt;
	enc_ckt.read_bench(arg_vec[2]);

	dec_ns::dec_oless_simil oldec(sim_ckt, enc_ckt, boolvec());
	oldec.run_1(stoi(arg_vec[3]), stoi(arg_vec[4]), stoi(arg_vec[5]));
	return 0;
}

int neos_command_ulearn(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	using namespace enc;

	if (arg_vec.size() != 8)
		neos_error("usage: ulearn <sim_ckt> <outnum> <width> <depth> <backstep> <indeg> <time_deadline>");

	encrypt::univ_opt opt;
	sim_ckt.read_bench(arg_vec[1]);

	int outnum = stoi(arg_vec[2]);
	opt.width = stoi(arg_vec[3]);
	opt.depth = stoi(arg_vec[4]);
	opt.backstep = stoi(arg_vec[5]);
	opt.indeg = stof(arg_vec[6]);
	int time_deadline = stoi(arg_vec[7]);
	opt.profile = encrypt::univ_opt::CONE;

	opt.num_ins = sim_ckt.nInputs();
	opt.num_outs = 1;

	circuit enc_ckt;
	encrypt::get_universal_circuit(enc_ckt, opt);
	std::cout << "num keys: " << enc_ckt.nKeys() << "\n";

	auto it = enc_ckt.inputs().begin();
	foreach_inid(sim_ckt, inid) {
		enc_ckt.setwirename(*it++, sim_ckt.wname(inid));
	}

	circuit sim_ckt0 = sim_ckt;
	string outname;
	int outind = 0;
	for (auto oid : sim_ckt.outputs()) {
		if (outind++ != outnum) {
			sim_ckt0.remove_wire(oid);
		}
		else {
			outname = sim_ckt0.wname(oid);
		}
	}
	sim_ckt0.remove_dead_logic();
	foreach_inid(sim_ckt, inid) {
		if (!sim_ckt0.wire_exists(inid)) {
			sim_ckt0.add_wire(sim_ckt.wname(inid), wtype::IN);
		}
	}

	enc_ckt.setwirename(*enc_ckt.outputs().begin(), outname);
	dec_ns::dec_opt::hill_climb(sim_ckt0, enc_ckt, time_deadline);
	return 0;
}


int neos_command_univ(int argc, char** argv) {

	using namespace enc;
	encrypt::univ_opt opt;
	circuit sim_ckt;
	vector<string> arg_vec(argv, argv + argc);

	int offset = 0;
	if (arg_vec.size() == 10)
		offset = 1;
	else if (arg_vec.size() == 9)
		offset = 0;
	else {
		neos_error("usage: univ [sim_ckt] <enc_ckt>"
				<< " <num_ins> <num_outs> <width> <depth> <indeg> <backstep> <profile>\n");
	}

	opt.num_ins = stoi(arg_vec[2 + offset]);
	opt.num_outs = stoi(arg_vec[3 + offset]);
	opt.width = stoi(arg_vec[4 + offset]);
	opt.depth = stoi(arg_vec[5 + offset]);
	opt.indeg = stof(arg_vec[6 + offset]);
	opt.backstep = stoi(arg_vec[7 + offset]);
	opt.profile = (stoi(arg_vec[8 + offset]) == 0) ?
			encrypt::univ_opt::SQUARE : encrypt::univ_opt::CONE;

	circuit enc_ckt;
	encrypt::get_universal_circuit(enc_ckt, opt);
	std::cout << "num keys: " << enc_ckt.nKeys() << "\n";

	if (offset == 1) {

		//opt.gate_type = 1;
		uint sim_ckt_size = 0;
		uint trial = 0;
		do {
			sim_ckt = circuit();
			encrypt::get_universal_simckt(sim_ckt, opt);;
			if (ext::abc_resynthesize(sim_ckt, 2, false) == -1) continue;
			sim_ckt_size = sim_ckt.nGates();
			std::cout << "sim ckt size: " << sim_ckt_size << "\n";
		}
		while(sim_ckt_size < 5 && trial++ < 50);

		foreach_inid(enc_ckt, inid) {
			if ( sim_ckt.find_wire(enc_ckt.wname(inid)) == -1 ) {
				sim_ckt.add_wire(enc_ckt.wname(inid), wtype::IN);
			}
		}
		sim_ckt.write_bench(arg_vec[offset]);
	}

	enc_ckt.write_bench(arg_vec[1 + offset]);
	return 0;
}


int neos_command_layers(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;
	sim_ckt.read_bench(arg_vec[1]);
	enc::encrypt::layer_stats(sim_ckt);
	return 0;
}


int neos_command_graph(int argc, char** argv) {
	vector<string> arg_vec(argv, argv + argc);
	circuit sim_ckt;

	if (arg_vec.size() != 3)
		neos_error("usage: graph <input_circuit> <dotfile>");

	sim_ckt.read_bench(arg_vec[1]);
	cktgph::cktgraph cktgp_sim(sim_ckt);
	cktgp_sim.writeVizFile(arg_vec[2]);
	return 0;
}



} // namespace

