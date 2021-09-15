/*
 * ckt_sat.h
 *
 *  Created on: Apr 30, 2016
 *      Author: kaveh
 */

#ifndef SRC_DEC_SAT_H_
#define SRC_DEC_SAT_H_

#include <signal.h>
#include <iomanip>
#include <boost/program_options.hpp>

#include "sat/sat.h"
#include "sat/c_sat.h"
#include "base/circuit.h"
#include "utl/utility.h"
#include "utl/ext_cmd.h"
#include "sat/base_sat.h"
#include "bdd/bdd.h"
#include "aig/aig.h"
#include "base/blocks.h"
#include "dec/dec.h"

namespace dec_ns {

using namespace sat;
using namespace ckt;
using namespace aig_ns;
using namespace utl;
using namespace csat;
using namespace dd;

// forward declarations
struct cyc_cone_t;

class cycsat_cond_builder;

class dec_sat : public virtual dec {
public:
	circuit mitt_ckt;
	aig_t mitt_aig;
	id2litmap_t mitt_maps;
	std::vector<slit> precond_assumps;
	std::vector<slit> mitter_assumps;
	std::map<string,id> mitt_keynames_map;
	std::vector<id2litmap_t> mitt_keyid2lit_maps;
	std::vector<id2idmap> mitt_invecs; // for multi-input mitters
	id2idmap iovec2sim;
	id mitt_out = -1;

	circuit iovecckt;
	slit io_tip, mitt_tip, term_tip;
	std::vector<slitvec> keylits;

	Hmap<slit, std::string> sl2name;

	// for termination conditions
	circuit term_ckt;
	id2litmap_t term_maps;
	std::vector<slit> term_assumps;
	vector<idvec> invecs; // i1, i2, and i3
	vector<idvec> keyvecs; // k1 and k2

	sat_solver Fi;

	//for epsilon
	boolvec truekey;

	idset key_only_logic;

	bool assump_vs_clause = true;

	// KC condition for sweeping
	circuit kc_ckt;
	slit kc_tiplit;
	id kc_out = -1;
	aig_t kc_aig;
	id2litmap_t kmap;

	// BDD condition engine data
	std::unique_ptr<circuit_bdd> kbdd;

	// CycSAT attack structures
	std::unique_ptr<cycsat_cond_builder> cycptr;

	id2boolHmap constant_wires;

	// wire disagreement map
	bool collect_disagreements = false;
	id2idmap wdisagreement_map;

public:
	// options
	enum satmode {APP, ACC, KDIP, CYC, BESAT} mode = ACC;

	bool use_aig = false;
	bool use_satsimp = 0;

	int settled_keys_detect = 0;
	int settled_keys_period = 5;

	int kc_sweep = 0;
	int kc_sweep_period = 20;

	int kc_bdd = 0;
	int kc_bdd_size_limit = -1;

	int propagate_limit = -1;
	int bdd_sweep_size_limit = -1;

	void get_parser(boost::program_options::options_description& op_parser);
	void print_options();

public:

	int verbose = 0;

	//AppSAT attack data
	iopair_t cur_dip, last_dip;
	uint banned_keys = 0;
	id converge = 0;

	int32_t iteration = 0;
	uint64_t queries = 0;

	std::vector<slit> Fi_assumps;

public:

	// constructor performs important initializiation
	dec_sat() : dec() {}
	dec_sat(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corrkey);

	virtual ~dec_sat() {}

	// SAT methods
	slit create_variable(sat_solver& S, id2litmap_t& varmap, id wireid);

	// mitter circuits
	virtual void build_mitter(const circuit& ckt);
	virtual void build_iovec_ckt(const circuit& cir);
	void build_sum_mitter(const circuit& ckt, uint num_ins);
	void build_ddip_mitter(const circuit& cir);

	// for ATPG based AppSAT
	void add_atpg_iovecs(uint numvecs = -1);
	void print_appsat_params();

	// key verification routine
	void extract_key(boolvec& key);
	void get_inter_key(void);

	// settled key analysis
	int find_settled_keys(int force_level = -1);
	void find_settled_keys_1();
	void find_settled_keys_2();
	void knowncones_to_knownkeys();
	void add_knownkeys_to_solver();

	void solve();

	// find dip
	virtual int solve_for_dip(iopair_t& dp);

	void key_bit_analysis(circuit& ckt);
	bool eval_ckt(sat_solver& CktSat, id2boolHmap& in_map,
			id2boolHmap& key_map, id2boolHmap& out_map, id2litmap_t& var_mp);
	virtual void solve_key();

	virtual void get_last_key();

	// test combinationality using mitter SAT
	static bool is_combinational(const circuit& ckt, const id2boolmap& consts_map);


	void write_to_file(sat_solver& S, std::vector<slit>& assumps);

	void print_stats(int iter);
	void print_stats(int iter, sat_solver& S);

	static double solve_exact(const circuit& sim_ckt,
					const circuit& enc_ckt);

	inline slit _get_mitt_lit(std::string kname, int primed = 0);
	inline slit _get_mitt_lit(id enc_cir_kid, int primed = 0);

	void solve_unrcyc();
	static uint _get_cyclic_depth(const circuit& enc_ckt);

	// option parser
	static void get_opt_parser(dec_sat& dc, boost::program_options::options_description& op);
	static void append_opt_parser(dec_sat& dc, boost::program_options::options_description& op);
	virtual int parse_args(int argc, char** argv);
	virtual void print_usage_message();

protected:

	virtual void _prepare_sat_attack();

	void solve_slice();
	void solve_full();

	// methods for storing input-output observations
	void record_sum_dip(void);
	void create_ioconstraint(const iopair_t& dp, const circuit& cir, int num_key_vecs = 2);
	void create_ioconstraint(const iopair_t& dp, const circuit& cir, sat_solver& S, int num_key_vecs, slit& io_tip_slit);
	void create_sum_ioconstraint(
			const std::vector<iopair_t>& sdp,
			const circuit& cir, sat_solver& S, int num_key_vecs);

	void create_ioconstraint_ckt(const iopair_t& dp, const circuit& cir);
	void _add_kc_ckt_to_solver(const circuit& kc_cir);
	void _get_kc_ckt_mult(const iopair_t& dp, circuit& kc_cir, const circuit& iocir, int num_copies = 1);
	void _get_kc_ckt(const iopair_t& dp, circuit& kc_cir, const circuit& iocir);
	virtual void _restart_from_ckt(const circuit& cir);

	void add_ioconstraint_to_bdd(const iopair_t& dp);

	void record_wdisagreements();

	// kdip style termination
	bool _build_terminate_condition();

	// tri-buf no-float condition
	void _add_no_float_condition();

	// key-only logic
	void _get_key_only_logic();

	// CycSAT attack methods
	void _build_anticyc_condition();
/*
	void _build_anticyc_condition2();
	void _build_anticyc_condition3();

	void _add_nc_condition_for_gate(gate gt, id w, id wprime);
	void _add_gate_blocking_condition_to_cir(const gate& gt,
				const idvec& blockers_fanins, const idvec& blockee_fanins);
	void _add_gate_blocking_condition(const gate& gt,
			const idvec& blockers_fanins, const idvec& blockee_fanins);
*/

	std::string to_named_str(slit kl);
	std::string to_named_str(const slitset& c);
	std::string to_str(const slitvec& c, int kc = 0);
	std::string to_str(const slitset& c, int kc = 0);

public:
	friend class cycsat_cond_builder;
};

class cycsat_cond_builder {
protected:
	std::shared_ptr<const circuit> enc_cir;
	dec_sat& _tp;
	sat_solver& Fi;
	int verbose = 0;
	// CycSAT attack structures
	id2litmap_t cycmap;
	std::vector<slit> nc_ends;
	std::vector<slit> nc_assumps;
	idvec nc_end_wids;
	slit nc_tip;
	circuit nc_cir;
	id2idmap nc_cir_map;
	uint num_extra_vars = 0;
	idvec wprimes, wprimes1, wprimes2;
	idset oneset;
	// track number of SAT-only variables
	id satonly_wire_id = 0;

public:
	cycsat_cond_builder(dec_sat& top) : _tp(top),
	verbose(top.verbose), enc_cir(top.enc_cir),
	Fi(top.Fi) {}

	void build_anticyc_condition();

protected:
	void _block_feedback_arc(id w);
	void _block_feedback_arc_stable(id w);
	void _propagate_block_condition2(id w);
	void _block_feedback_arc3(id w);
	void _block_feedback_arc4(id w);
	void _add_no_float_condition();
	void _build_cycsat_cone(const idset& key_wires, const idset& one_wires,
			const idset& free_wires, id wprime, id w);
	void _add_gate_blocking_condition_to_cir(const gate& gt,
			const idvec& blocker_fanins, const idvec& blockee_fanins);
	void _add_gate_blocking_condition(const gate& gt,
			const idvec& blocker_fanins, const idvec& blockee_fanins);
	void _add_nc_condition_for_gate(gate gt, id w, id wprime);

};

class dec_sat_appsat : public dec_sat {
protected:
	int low_hd_count = 0;
	bool converged = false;
	bool epsilon = false;

public:
	bool appsat_reinforce = false;
	int num_dip_period = 1; // number of DIP queries
	int num_rand_queries = 20; // number of random queries
	int appssat_error_threshold = 0; // Low-error threshold
	int appsat_Settle = 5; // settlement threshold

public:
	dec_sat_appsat() {}
	dec_sat_appsat(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corr_key) :
		dec(sim_cir, enc_cir, corr_key), dec_sat(sim_cir, enc_cir, corr_key) {}
	virtual ~dec_sat_appsat() {}
	void solve();

	// option parser
	static void get_opt_parser(dec_sat_appsat& dc, boost::program_options::options_description& op);
	static void append_opt_parser(dec_sat_appsat& dc, boost::program_options::options_description& op);
	virtual int parse_args(int argc, char** argv);
	virtual void print_usage_message();

protected:
	void print_appsat_params();
	bool analyze_inter_key();
};

class dec_sat_kdip : public dec_sat {
protected:
public:
	dec_sat_kdip() {}
	dec_sat_kdip(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corr_key) :
		dec(sim_cir, enc_cir, corr_key), dec_sat(sim_cir, enc_cir, corr_key) {}
	virtual ~dec_sat_kdip() {}
	void solve();

protected:
	void build_mitter(const circuit& cir);
};

class dec_sat_ucyc : public dec_sat {
protected:
	circuit un_enc_cir;
	idset init_wids;
public:
	dec_sat_ucyc() {}
	dec_sat_ucyc(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corr_key) :
		dec(sim_cir, enc_cir, corr_key), dec_sat(sim_cir, enc_cir, corr_key) {}
	virtual ~dec_sat_ucyc() {}
	//void solve();

protected:
	void _prepare_sat_attack();
};

class dec_sat_besat : public dec_sat {
protected:
public:
	dec_sat_besat() {}
	dec_sat_besat(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corr_key) :
		dec(sim_cir, enc_cir, corr_key), dec_sat(sim_cir, enc_cir, corr_key) {}
	virtual ~dec_sat_besat() {}
	void solve();
protected:
	bool _besat_check_key();
};


class dec_sat_fraig : public dec_sat {
protected:
	// most recent mitter_out
	id fi_out = -1;
	// Fi condition AIG
	aig_t fi_aig;
public:
	dec_sat_fraig() {}
	dec_sat_fraig(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corr_key) :
		dec(sim_cir, enc_cir, corr_key), dec_sat(sim_cir, enc_cir, corr_key) {}
	virtual ~dec_sat_fraig() {}

	void solve();
	int parse_args(int argc, char** argv);

protected:
	int solve_for_dip(iopair_t& dp);
	void add_ioconstraint(iopair_t& dp);
	void solve_key();
};


inline slit dec_sat::_get_mitt_lit(std::string nm, int primed) {
	if (primed != 0) nm += "_$" + std::to_string(primed);
	return mitt_maps.at(mitt_ckt.find_wcheck(nm));
}
inline slit dec_sat::_get_mitt_lit(id enc_cir_kid, int primed) {
	string nm = enc_cir->wname(enc_cir_kid);
	return _get_mitt_lit(nm, primed);
}
inline std::string _primed_name(const std::string& nm) { return nm + "_$1"; }
inline std::string _unprime_name(const std::string& nm) { return nm.substr(0, nm.length() - 3); }
inline bool _is_primed_name(const std::string& nm) { return _is_in("_$1", nm); }
inline bool _is_iocir_outname(const std::string& nm) { return _is_in("_$io", nm); }
inline std::string _iocir_outname(const std::string& nm) { return nm + "_$io"; }
inline std::string _uniocir_outname(const std::string& nm) { return nm.substr(0, nm.length() - 4); }


} // namespace csat


#endif /* _SAT_DEC_H_ */

