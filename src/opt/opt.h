/*
 * opt.h
 *
 *  Created on: Aug 29, 2018
 *      Author: kaveh
 *      Description: Sweeping algorithms
 */

#ifndef SRC_AIG_OPT_H_
#define SRC_AIG_OPT_H_

#include "aig/aig.h"
#include "bdd/bdd.h"
#include "sat/sat.h"
#include "sat/c_sat.h"
#include "aig/aig_sat.h"

#include <iomanip>
#include <boost/dynamic_bitset.hpp>



namespace opt_ns {

using namespace aig_ns;
using namespace dd;
using namespace sat;
using namespace csat;

const int verbose = 1;

// top object for sweeping schemes
class Sweep {

public:
	int verbose = 0;

	idset dont_merge;

	double commit_time = 0;

	double analyze_time = 0;
	double time_budget = -1;

public:

	virtual void analyze() = 0;
	virtual void commit() = 0;

	virtual ~Sweep() {}

	void sweep() {
		this->analyze();
		this->commit();
	}

};

class AigSweep : public virtual Sweep {
public:
	aig_t* aig = nullptr;
	alit zero_alit = alit(-1, 0);
	id one_node = -1;

	std::unordered_map<id, id> id2class;
	// classes are stored in a vector and hence never removed
	// class removal :‌ ‌the idset of the class is empty
	std::vector<alitset> classes;

public:
	~AigSweep() {}
	AigSweep(aig_t& aig) : aig(&aig) {}

	void commit();
	void print_equiv_classes();
	std::string aigsweep_nName(alit al);

protected:
	void _add_to_class(alit a1, alit a2);
	void _create_new_singleton_class(alit a1);
	void _refine_by_unequal_pair(alit a1, alit a2);
	void _init_classes_allone();
	void _refine_by_simulation(id2boolvecmap& sim_str_map);
};

class CircuitSweep : public virtual Sweep {
public:
	circuit* cir = nullptr;

	std::unordered_map<id, id> id2class;
	// classes are stored in a vector and hence never removed
	// class removal :‌ ‌the idset of the class is empty
	std::vector<idset> classes;
	std::vector<idpair> merges; // FIXME: Do we realy need this?

public:
	~CircuitSweep() {}
	CircuitSweep(circuit& cir) : cir(&cir) {}

	void commit();
	void print_equiv_classes();

protected:
	void _add_to_class(id wid1, id wid2);
	void _create_new_singleton_class(id wid1);
	void _refine_by_unequal_pair(id wid1, id wid2);
	void _init_classes_allone();
	void _refine_by_simulation(id2boolvecmap& sim_str_map);
};

class BDDSweep : public virtual Sweep {
public:

	uint size_limit = 0;

	Cudd _mgr;
	id2bddmap wid2bddmap;
	std::unordered_map<DdNode*, id> bdd2widmap;

public:
	~BDDSweep() {}
	BDDSweep(int size_limit) : size_limit(size_limit) {}
};

class BDDCircuitSweep : public CircuitSweep, public BDDSweep {
public:
	~BDDCircuitSweep() {}
	BDDCircuitSweep(circuit& cir, int s) : CircuitSweep(cir), BDDSweep(s) {}
	void analyze();
};


class BDDAigSweep : public BDDSweep, public AigSweep {
public:
	~BDDAigSweep() {}
	BDDAigSweep(aig_t& aig, int s) : BDDSweep(s), AigSweep(aig) {}
	void analyze();

};


// the cut-sweeping class

class CutSweep : public AigSweep {
private:
	struct cut_bdd_t {
		oidset inputs;
		BDD cutfun;

		bool operator==( const cut_bdd_t& cut2 ) const {
			return 	   inputs == cut2.inputs
					&& cutfun == cut2.cutfun;
		}
	};

	// specialized hash function for cut_bdd_t
	struct cut_bdd_t_hash {

		std::size_t operator()(const cut_bdd_t& cut) const {
			using boost::hash_combine;
			std::size_t seed = 0;

			for (auto nid : cut.inputs)
				hash_combine(seed, nid);
			hash_combine(seed, cut.cutfun.getNode()); // must be within the same manager
			return seed;
		}
	};

	Cudd mgr; // BDD manager

	std::unordered_map<id, std::vector< cut_bdd_t > > id2cuts;
	std::unordered_map<cut_bdd_t, id, cut_bdd_t_hash> cut2id;

	uint k; // k-feasible cuts
	uint N; // maximum cut-set size
	uint s; // s-size limit of BDD

public:
	CutSweep(aig_t& ntk, uint k = 7, uint N = 30, uint s = 250) :
		AigSweep(ntk), k(k), N(N), s(s) {}
	void analyze();

protected:
	void _enumerate_cuts();
	void _cutEnum(id nid);
	id _addCut(id aid, BDD ndfun, const oidset& inputs);

};

// SAT-sweeping class
class SATsweep : public virtual Sweep {
public:

	struct sweep_params_t {

		sat_solver* S = nullptr;
		bool external_solver = false;
		id2litmap_t vmap; // variable map
		slitvec assumps; // assumptions for solver

		/* if clauses are already present
		 * set this value to false */
		bool clauses_present = false;

		/* if two ndoes are equal will add a clause
		 to reinforce this to this solver */
		bool reinforce_equality = false;

		/* if inputs are under an external condition */
		bool external_condition = false;

		// number of distant-1 vectors to simulate besides cex
		uint num_dist_one = 0;

		// test according to top order
		bool ordered_test = true;

		/* use hashmap of strings for analyzing classes
		 * under simulation data */
		bool string_hash = true;

		uint num_patterns = 4028;

		int64_t prop_limit = -1;
		int64_t mitt_prop_limit = -1;

		inline void set_external_solver(sat_solver* Sext) {
			S = Sext;
			external_solver = true;
			external_condition = true;
		}

	} mp;

	typedef boost::dynamic_bitset<> dbitset;
	Hmap<dbitset, idset> simclasses;
	Hmap<id, dbitset> sim_data;
	Hmap<id, dbitset> cex_data;

	id2idmap ordermap;

	// position in the order that is swept
	id order_pos = 0;
	idset proven_set;
	idset strashed;

	uint32_t timed_out = 0, sat_calls = 0, cex_count = 0, equals = 0;

	double sim_time = 0;
	double ref_time = 0;
	double sat_time = 0;
	double mine_time = 0;

public:
	SATsweep();
	SATsweep(sweep_params_t& params);

	~SATsweep();

	static int _test_equivalence_sat(id wid1, id wid2, sat_solver& S,
			id2litmap_t& vmap, bool inverted = false,
			const slitvec& assumps = slitvec(),
			int64_t propagate_limit = -1, bool reinforce = false);

	static int _test_equivalence_sat(slit wslit1, slit wslit2, sat_solver& S, bool inverted,
			const slitvec& assumps, int64_t propagate_limit, bool reinforce = false);

	static uint _get_num_patterns(uint num_nodes);

protected:
	static void _append_dbitset(boost::dynamic_bitset<> &bits1,
			const boost::dynamic_bitset<> &bits2) {
	    for (uint i = 0; i < bits2.size(); i++)
	    	bits1.push_back(bits2[i]);
	}

};

class SATCircuitSweep : public SATsweep, public CircuitSweep {
public:

	typedef std::multiset<id> idmulset;

	bool use_fraig = true;

	idvec wire_order;
	idmap<id> nd2fgmap;
	id zero_wid = -1, one_wid = -1;
	bool track_complement = false;

	struct cirFanin {
		idmulset fanins;
		fnct function;

		cirFanin(const gate& g) {
			for (auto fanin : g.fanins) {
				fanins.insert(fanin);
			}
			function = g.gfun();
		}

		bool operator==(const cirFanin& lhs) const {
			return fanins == lhs.fanins && function == lhs.function;
		}
	};

	struct cirFanin_Hash {

		std::size_t operator()(const cirFanin& af) const {
			using boost::hash_combine;
			std::size_t seed = 0;

			// mix fanin ids and fanin complement flags
			hash_combine(seed, af.fanins);
			hash_combine(seed, af.function);

			return seed;
		}

	};

	typedef std::unordered_map<cirFanin, id, cirFanin_Hash> cirFaninTable;
	cirFaninTable cirfaninTab;

	void analyze();
	SATCircuitSweep(circuit& cir) : CircuitSweep(cir) {}
	SATCircuitSweep(circuit& cir, sweep_params_t& p) : SATsweep(p), CircuitSweep(cir) {}

protected :
	void _classify_unresolved_nodes();
	void _get_wire_order();
	void _sweep_class_cex_driven(uint cl);
	bool _pick_next_class(uint& cl);
	void _simulate_cex();
	void _refine_by_simulation(id2boolvecmap& sim_str_map);
	void _print_stats();
	void _analyze_fraig();
	void _analyze_cex_driven();
	void _record_cex_fraig();
	void _resimulate_with_cex(id curnid);
	void _mine_sim_patterns_allsat();

};


class SATAigSweep : public SATsweep, public AigSweep {
public:
	struct fraig_nd_t {
		alit mpnd;
		dbitset simvec;
		bool strashed = false;
		bool satfail = false;
	};

	struct fraig_stats_t {
		double sat_time = 0;
		double sim_time = 0;
		uint32_t num_sat_calls = 0;
		uint32_t num_sat_timeout = 0;
		uint32_t num_strashed = 0;
		uint32_t equals = 0;
	} _fgst;

	/* use Fraig algorithm instead of cex_driven
	much faster than cex_driven */
	bool use_fraig = true;

	aigFaninTable strash_table;
	idmap<fraig_nd_t> fgmp;

	idvec node_order;

	bool track_complement = false;

	oidset mit_outs;
	id mit_activating_cex_index = -1;
	id2boolmap mit_active_cex;
	int mit_status = sl_Undef;

public:
	void analyze();
	SATAigSweep(aig_t& aig) : AigSweep(aig) {}
	SATAigSweep(aig_t& aig, sweep_params_t& p) : SATsweep(p), AigSweep(aig) {}

protected :
	void _analyze_cex_driven();
	void _analyze_fraig();
	bool _record_cex_fraig(id curnid);
	void _check_miter_out();
	int _test_equivalence_sat_vardec(id wid1, id wid2, bool inv);
	void _init_fraig();
	void _collect_init_patterns();
	void _fraig_node(id nid);
	void _simulate_cex();
	void _sweep_class_cex_driven(uint cl);
	bool _pick_next_class(uint& cl);
	void _refine_by_simulation(id2boolvecmap& sim_str_map);
	void _create_one_classes();
	void _print_stats();

};

// external object-free interface
void aig_cutsweep(aig_t& ntk);
void aig_satsweep(aig_t& ntk);
void aig_bddsweep(aig_t& cir, int s);

void circuit_satsweep(circuit& ntk, double time_budget = -1);
void circuit_satsweep(circuit& cir, SATsweep::sweep_params_t& params, double time_budget = -1);
void circuit_bddsweep(circuit& cir, int s, double time_budget = -1);

void _merge_equiv_nodes(circuit& cir, const std::vector<idset>& classes,
		std::vector<idpair>& merges);
void _merge_equiv_nodes(aig_t& ntk, const std::vector<alitset>& classes,
		const idset& dont_merge, alit zero_lit);


// fine-tuned proving algorithms using fraig/etc
int prove_fraig0(const aig_t& miter, const oidset& mit_outs, id2boolHmap& cex);

}


#endif /* SRC_AIG_OPT_H_ */
