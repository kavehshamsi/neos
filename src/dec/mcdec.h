/*
 * mc.h
 *
 *  Created on: Jan 3, 2018
 *      Author: kaveh
 */

#ifndef SRC_MC_MCDEC_H_
#define SRC_MC_MCDEC_H_

#define NEOS_DEBUG_LEVEL 0

#include "sat/sat.h"
#include "sat/c_sat.h"
#include "mc/bmc.h"
#include "base/circuit.h"
#include "dec/dec.h"
#include "utl/ext_cmd.h"
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

// for named pipes
#include <sys/types.h>  // mkfifo
#include <sys/stat.h>   // mkfifo
#include <sys/wait.h>  // for waitpid
#include <fcntl.h>

#include <thread>
#include <future>
#include <csignal>

namespace dec_ns {

using namespace ckt;
using namespace utl;
using namespace sat;
using namespace csat;
using namespace mc;

using std::cout;

#define TIME(code) \
		auto start = _start_wall_timer();\
		code;\
		std::cout << "time: " << _stop_wall_timer(start) << "\n";

#define _delim_list_coll(output_str, elem, collection, format_str, format_args, delim) \
	{\
		uint macro_count = 0;\
		for (auto& elem : collection) { \
			output_str += fmt(format_str, format_args);\
			if (macro_count++ != collection.size() - 1)\
				output_str += fmt(" %s ", delim);\
		}\
	}\

#define _delim_list_range(output_str, x, low, high, format_str, format_args, delim) \
	{\
		for (int64_t x = low; x < high; x++) { \
			output_str += fmt(format_str, format_args);\
			if (x != high - 1)\
				output_str += fmt(" %s ", delim);\
		}\
	}\


void _int_deadline_handler(int n);


/*
 *  a class for dealing with model-checking based deobfuscation
 *  every other sequnetial deobfuscator derives from this
 */
class mcdec_bmc : public dec {

protected:
	bool enc2sim_dff_mapping = true;

	int bmc_len = 2; // current bmc length which is grown in each step
	int checked_bound = -1;
	bool use_ic3 = false;
	bool term_initialized = false;
	uint iter = 1;
	uint dis_iter = 1;
	uint max_dis_len = 1;
	uint term_checks = 0;

	vector<idpair> enc_lpairs, sim_lpairs;

	bimap<id, id> s2ns_sim, s2ns_enc;
	id2litmap_t sim_maps, enc_maps;

public:

	int use_satsimp = 0;
	uint bmc_bound_steps = 1;

	int backbone_period = -1; // period of backbone analysis

	// bdd analysis and restarts
	int kcbdd_option = 0;
	int kcbdd_size_limit = 0; // 0: no limit, x: limit on bdd-size
	int kcbdd_restart_period = 30; // how often to restart from bdd

	/* SAT sweeping on key-conditions
	 * 0: No sweeping
	 * 1: unroll circuit and propagate constants
	 * 2: best neos simplification
	 * 3: best abc simplification
	 */
	int kc_sweep = 0;
	// if 1 will sweep DIP condition on each iteration
	int kc_sweep_iterative = 0;
	// each kc_sweep_period steps will restart SAT solver to total kc_ckt
	int kc_sweep_period = 5;

	// if bdd-sweep is enabled this is the s value in the sweep
	int bdd_sweep_size_limit = 150;

	/* BMC sweeping
	 * 0: no sweeping
	 * 1: transition-relation simplification
	 */
	int bmc_sweep = 0;

	int relative_sweep = 0;

	/* mitter sweeping
	 * 0: no sweeping
	 * 1: mitter sweep
	 * 2: mitter sweep + settled node detection
	 */
	int mitt_sweep = 0;
	int mitt_sweep_period = 3;

	int propagate_limit = -1;

	// important variable
	int bmc_bound_limit = 600;

	// used in ic3 solver
	int kc_method = 0;

	// sequence of discriminating input patterns
	struct dis_t : ioseq_t {
		// 2 keys for testing only
		boolvec ks[2];

		void clear() {
			xs.clear();
			ys.clear();
			ks[0].clear();
			ks[1].clear();
		}

		dis_t() {}
		dis_t(uint len) : ioseq_t(len) {}
	};

	struct mcdec_bmc_stats {
		double sat_time = 0;
		double term_time = 0;
		double gen_time = 0;
		double bmc_time = 0;
		double simp_time = 0;

		double total_time = 0;

		uint64_t dis_count = 0;
		uint64_t dis_maxlen = 0;
		uint64_t ngcl_count = 0;

		void print();
	} stats;


	// keep track of last DIS for interrupts and what not
	dis_t last_dis;
	std::vector<dis_t> all_dises;

public:

	// public constructor
	mcdec_bmc(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corrkey);
	mcdec_bmc() {}
	virtual ~mcdec_bmc() {}

	// option parsing
	static void get_opt_parser(mcdec_bmc& dc, boost::program_options::options_description& op);
	static void append_opt_parser(mcdec_bmc& dc, boost::program_options::options_description& op);


protected:

	void _simulate_enc(dis_t& dis);

	void _create_var2id_mapping();

	void _simulate_rand_dis(dis_t& dis);

	void _add_dis_constraint(dis_t& dis);
	void _print_dis(dis_t& dis);

	void _setup_simulation();

	void _push_all_lits(std::vector<slit>& litvec1, const std::vector<slit>& litvec2);

};


class mcdec_bmc_int : public mcdec_bmc {
	friend class Bmc;
public:
	// generic class will need mitt_ckt
	circuit mitt_ckt;

	// for internal SAT solver based deobfuscation
	std::shared_ptr<Bmc> mittbmc;
	Bmc::Bmc_config_t bmc_config;

	id bmc_out = -1;
	id mitt_out = -1;
	id ioc_out = -1;
	id term_out = -1;
	slit term_link;

	id kc_out = -1;
	slit kc_out_lit;

	id2idHmap enc2mitt_kmap[2]; // for k1 and k2
	id2idHmap enc2mitt_xmap; // for k1 and k2
	id2idHmap enc2io_ymap; // mapp enc_out to io_condition
	id2idHmap encid2disIndex;

	std::vector<idpair> encmitt_xids;
	std::vector< std::vector<idpair> > encmitt_kids;

	sat_solver kc_S;

	// key ids resolved by backbone analysis
	idset backbone_resolved_kids;

	boolvec valid_key; // last valid key used for simulation

	// BDD data for key condition compression
	circuit_bdd kcbdd;

	std::vector< std::vector<int> > negcls;
	slitvec klits[2];


public:

	mcdec_bmc_int(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corrkey);
	mcdec_bmc_int() {}

	virtual ~mcdec_bmc_int() {}

	virtual void solve();
	virtual void extract_key_int();
	virtual void get_last_key();

	static void get_opt_parser(mcdec_bmc_int& dc, boost::program_options::options_description& op);
	static void append_opt_parser(mcdec_bmc_int& dc, boost::program_options::options_description& op);

	void print_options();

// internal methods
protected:
	virtual void _build_mitter_int();

	void _build_iockt_int();
	virtual int  _check_termination_int();
	virtual void _add_dis_constraint_int(dis_t& dis);
	virtual void _add_dis_constraint_int(dis_t& dis, const circuit& cir, sat_solver& S);
	virtual void _add_dis_constraint_kc(dis_t& dis)
		{ neos_abort("kc condition add not implemented"); }
	virtual void _mitt_sweep() { neos_abort("mitt_sweep not implemented"); }

	virtual void _perdis_opt_routines();
	virtual void _perlen_opt_routines();

	virtual void _add_kc_ckt_to_solver(const circuit& uncir);
	void _add_kc_aig_to_solver(const aig_t& uncir);
	virtual int _solve_for_dis_int(dis_t& dis);
	void _sweep_wrt_key_conditions(circuit& cir);
	virtual bool _simplify_with_settled_nodes(const idset& settled_nodes)
		{ neos_abort("simplify settled nodes not implemented"); }
	void _restart_with_knownkeys();
	void _restart();

	virtual void _add_termination_links();
	void _backbone_test();
	void _resolved_node_analysis(int nFrame);

	void _simulate_dis_enc_sweep(const dis_t& dis, std::vector<id2boolmap>& id2enc_dismaps);
	void _add_dis_constraint_sweep(dis_t& dis);
	void _get_live_wires_and_gates(idset& live_wires, idset& live_gates, int f);
	void _SAT_sweep_extract_keys(std::vector<boolvec>& vspace_keys);
	int  _test_equivalence_sat(id wid1, id wid2, sat_solver& S, id2litmap_t& vmap);
	void _merg_equiv_nodes(circuit& cir, const std::map<int, idset>& classes);

	// Solver restarts
	virtual void _restart_bmc_engine() { neos_abort("bad object"); }
	virtual void _restart_to_unrolled_mitt() { neos_abort("bad object"); }
	void _restart_from_bdd();
	virtual void _restart_from_ckt(const circuit& cir);
	void _restart_from_aig(const aig_t& aig);

	// helper
	inline slit _get_mittlit(int f, const std::string& kname);
	inline slit _get_keylit(id enckid, int prime);
	inline slit _get_keylit(const std::string& kname);
	inline void _link_keylits(const aig_t& kaig, id2litmap_t& vmap);

protected:
	void _simulate_enc_int(dis_t& dis);

// static internal methods
protected:
	static void _tie_input(circuit& cir, id wid, bool val);

};


class mcdec_int_ckt : public mcdec_bmc_int {
	friend class Bmc;
public:

	// iockt data
	circuit iockt;

	//CircuitUnroll io_cunr;
	CircuitUnroll mitt_cunr;

	// key condition circuit
	circuit kc_ckt;

	// smq options
	int rfq_mode = 0;
	int rfq_dis_period = 5;
	int rfq_num_rand_patts = 100;
	double rfq_skew_threshold = 0.01;

	// negative key data
	struct miner_data_t {
		double subsumtions = 0;
		double subsume_rate = 0;
	} ngkstats;

public:
	mcdec_int_ckt(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corrkey) :
				mcdec_bmc_int(sim_ckt, enc_ckt, corrkey) {}
	mcdec_int_ckt() {}

	~mcdec_int_ckt() {}

	static void get_opt_parser(mcdec_int_ckt& dc, boost::program_options::options_description& op);
	static void append_opt_parser(mcdec_int_ckt& dc, boost::program_options::options_description& op);
	virtual int parse_args(int argc, char** argv);

// internal methods
protected:
	// virtual implementations
	void _build_mitter_int();
	void _add_dis_constraint_kc(dis_t& dis);
	void _restart_bmc_engine();
	void _restart_to_unrolled_mitt();
	void _mitt_sweep();
	bool _simplify_with_settled_nodes(const idset& settled_nodes);

	void _perdis_opt_routines();

	// non virtual
	void _get_unrolled_dis_ckt(dis_t& dis, circuit& unrollediockt);
	void _mitt_sweep_1();
	void _mitt_sweep_2();
	void _cir_simplify(circuit& cir, int level);


	// rare querying
	void analyze_rfq();
	void get_skewed_wires(idset& skewed_wires, id2boolmap& comm_vals);
	void query_skewed_wires1(const idset& skewed_wires,
			const id2boolmap& orig_simmap, const id2boolmap& common_vals);
	void backtrack_lpws_from_output(vector<boolvec>& possible_lpw_vals,
			const id2boolmap& sim_map, const idset& skewed_wires);
	void query_skewed_wires2(const idset& skewed_wires,
			const id2boolmap& orig_simmap, const id2boolmap& common_vals);
	void query_skewed_wires3(const idset& skewed_wires,
			const id2boolmap& orig_simmap, const id2boolmap& common_vals);

// static internal methods
protected:
	static void _unroll_ckt(const circuit& cir,
				circuit& uncir, int numFrames, const idset& frozens);
};


class mcdec_int_aig : public mcdec_bmc_int {
	friend class Bmc;
public:
	// for internal SAT solver based deobfuscation
	aig_t mitt_aig;

	// iockt data
	aig_t ioaig;

	AigUnroll io_aunr;
	AigUnroll mitt_aunr;

	// key condition circuit
	aig_t kc_aig;

	// BDD data for key condition compression
	circuit_bdd kbdd;
	circuit_bdd negkbdd;

public:

	mcdec_int_aig(circuit& sim_ckt, circuit& enc_ckt, const boolvec& corrkey) :
				mcdec_bmc_int(sim_ckt, enc_ckt, corrkey) {}

	mcdec_int_aig() {}

	~mcdec_int_aig() {}

	virtual int parse_args(int argc, char** argv);

// internal methods
protected:
	void _build_mitter_int();
	void _build_iockt_int();
	void _add_dis_constraint_kc(dis_t& dis);
	void _mitt_sweep();
	void _mitt_sweep_1();
	void _mitt_sweep_2();
	bool _simplify_with_settled_nodes(const idset& settled_nodes);
	void _aig_simplify(aig_t& ntk, int level);

	void _get_unrolled_dis_aig(dis_t& dis, aig_t& unrolledaig);
	void _restart_to_unrolled_mitt();

	void _restart_bmc_engine();


// static internal methods
protected:

};


class mcdec_int_abc : public mcdec_bmc_int {
	friend class Bmc;
public:

	//CircuitUnroll io_cunr;
	CircuitUnroll mitt_cunr;

	// key condition circuit
	circuit kc_ckt;
	circuit bmc_cir;

	// important file/dir paths
	std::string temp_dir;
	std::string mitt_cir_file;
	std::string kc_cir_file;

public:

	mcdec_int_abc(circuit& sim_ckt, circuit& enc_ckt, const boolvec& corrkey);

	~mcdec_int_abc();

// internal methods
protected:
	// virtual implementations
	int _solve_for_dis_int(dis_t& dis);
	void _build_mitter_int();
	void _add_dis_constraint_int(dis_t& dis);
	void _get_unrolled_dis_cir(dis_t& dis, circuit& unrollediockt);
	int _check_termination_int();
	void extract_key_int();
	void _exit_with_cleanup();

	void read_mc_file(const std::string& bmc_log, int& retstat, int& fr);
	void read_cex_file(const std::string& cex_file, dis_t& dis, int f);
};



class mcdec_nuxmv : public mcdec_bmc {
public:

	std::string temp_dir;
	std::string dec_smv_file;
	std::string dec_res_file;
	std::string dec_term_file;
	std::string sim_smv_file;
	std::string nuXmv_cmd;

	/*
	 *  struct for storing smv model
	 *  data. includes simple stringstreams for now
	 */
	struct smv_model {
		std::string dis_cond; // dis constraints
		std::string top_mod; // main module
		std::string term_cond; // termination conditions
		std::string sub_mods; // sub-modules

		friend std::ostream& operator<<(std::ostream& os, const smv_model& mdl) {
			os << mdl.top_mod << mdl.term_cond
					<<  mdl.dis_cond << mdl.sub_mods;
			return os;
		}

	};

	smv_model dec_smv;

	std::string co; // original circuit sub-module smv string
	std::string ce; // encrypted circuit sub-module smv string
	std::string opened_ce; // opened circuit for termination conditions
	std::string term_cond_str; // termination condition smv string
	std::string term_cond_spec_str; // termination condition smv string


public:

	// public constructor
	mcdec_nuxmv(circuit& sim_ckt, circuit& enc_ckt, const boolvec& corrkey);
	mcdec_nuxmv() {}
	virtual ~mcdec_nuxmv() {}

	// exception for handling the signals
	void _extract_key();
	void solve_nuxmv();

	virtual int parse_args(int argc, char** argv);

protected:

	void _build_mitter();

	bool _solve_for_dis(dis_t& dis);
	void _add_dis_constraint(dis_t& dis);

	int _dis_extractor(dis_t& dis);

	int _check_termination();
	int _check_termination_extractor();

	void _write_smv_file(const std::string& dec_smv_file,
			const smv_model& mdl);

	std::string _bool_str(bool b);

	std::string _x_str (int64_t iteration, int64_t clk = -1, int64_t index = -1);
	std::string _y_str (int64_t z, int64_t iteration = -1,
			int64_t clk = -1, int64_t index = -1);

	std::string _define_array(const std::string& basename, uint length);

	std::string _gate_expr(const gate& g);
	static std::string _gate_expr(const gate& g, const circuit& cir);
	std::string _model_wire_name(id wr, bool array = true);

	template <typename Ext>
	int _run_nuXmv(
			const std::string& cmd, const smv_model& smv_mdl,
			const std::string& smv_file, const std::string& res_file,
			Ext& ext);

	std::string _define_comp(const std::string& base_str1,
			const std::string& base_str2,
			const std::string& out, uint len);


public:
	void _clean_up();

	static void _convert_to_smv(const circuit& cir,
			const oidset& frozens, std::string& smv);
};


// inline functions
slit mcdec_bmc_int::_get_mittlit(int f, const std::string& kname) {
	return mittbmc->getlit(f, kname);
}

slit mcdec_bmc_int::_get_keylit(id enckid, int prime) {
	return mittbmc->getlit(0, enc2mitt_kmap[prime].at(enckid));
}

slit mcdec_bmc_int::_get_keylit(const std::string& kname) {
	return mittbmc->getlit(0, kname);
}



} // namespace mc
#endif /* SRC_MC_MCDEC_H_ */
