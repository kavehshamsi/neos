/*
 * rwr.h
 *
 *  Created on: Sep 25, 2019
 *      Author: kaveh
 */

#ifndef SRC_AIG_RWR_H_
#define SRC_AIG_RWR_H_

#include "aig/aig.h"
#include "aig/cut.h"

namespace opt_ns {

typedef uint16_t truth_t;
using namespace aig_ns;

struct rwr_node_t {
	id nid;
	alit a0;
	alit a1;
	idset fanouts;
	int node_size = 0;
	int level = -1;
	truth_t tt = 0;
	id input_index = -1;
	id travid = -1;
	alit mpnd = -1;
};

struct npn_indices {
	int8_t flip = 0;
	int8_t pInd = 0;
	int8_t nInd = 0;
};

struct canon_data_t {
	truth_t ctt; // truth-table
	vector<truth_t> mindag_list;
	bool is_var : 1 = false;
	bool has_dag : 1 = false;
	bool is_prac : 1 = true;
	int canon_class : 13 = -1;
};

struct tt_data_t {
	alit dag_node = -1;
	bool is_canon : 1 = false;
	bool visited : 1 = false;
	int canon_class : 14 = -1;
	npn_indices indx;
};

class rewrite_manager_t {
private:
	aig_t* ntk = nullptr;

	typedef std::array<std::array<int, 16>, 24> tt_perms_t;
	typedef std::array<std::array<int, 16>, 24> tt_negs_t;


	std::array<std::array<int, 4>, 24> varperms;
	tt_perms_t perms;
	tt_negs_t negs;

	vector<tt_data_t> allTabs;
	vector<canon_data_t> canonTabs;
	vector<truth_t> prac_npns;
	const int64_t struct_per_class_limit = 1;
	uint num_npn_class = 0;
	uint32_t num_tt_discovered = 0;

	id max_id = 0;
	alitvec L;
	idvec invars;
	aig_t bn; // best network
	idmap<rwr_node_t> rwrdag;
	idmap<idvec> dagsorts;
	bool dagsorts_built = false;
	Hmap<id, truth_t> simmap;
	id curtid = 0;
	id curnum_mffc = 0;
	id curcc = -1;
	Hmap<id, id> travidvec;


	// array storing NPN DAG
	const static std::string dag_str;

	bool use_prac_only = true;
	// array stroing practical NPN classes
	const static std::string prac_str;

	// important scratchpad
	idset moved_fanouts;

	const cut_t* curcut = nullptr;
	const cut_t* bestcut = nullptr;
	truth_t bestcut_rtt;
	truth_t bestcut_ctt;
	// for random replacement choose
	// among the best candidates randomly
	vector<const cut_t*> bestcut_vec;
	vector<truth_t> bestcut_rtt_vec;
	vector<truth_t> bestcut_ctt_vec;

	idvec curcut_aorder, bestcut_aorder;
	idvec curcut_invec, bestcut_invec;

	struct stats {
		double setup_time = 0;
		double cut_time = 0;
		double sim_time = 0;
		double npn_time = 0;
		double dag_tsort_time = 0;
		double cut_sort_time = 0;
		double tsort_time = 0;
		double network_mod_time = 0;
		double total_time = 0;
		double count_unused_time = 0;
		double eval_time = 0;
		double move_time = 0;
		id total_saved = 0;
		id original_nodes = 0;
		id final_nodes = 0;
		id cuts_analyzed = 0;
		id replaced = 0;
	} pst;

	typedef std::vector< cut_t > cutvec_t;

public:
	int verbose = 0;
	int replacement_gain_threshold = 0;
	bool random_replacement = true;

	rewrite_manager_t(int permcomp = 0);
	void rewrite_aig(aig_t& ntk);


	void aig_npnclasses(const aig_t& rntk, Hset<truth_t>& npnclasses);
	void find_practical_classes(const std::string& bench_dir);
	void print_stats();

protected:

	int precompute = 0;

	inline static truth_t reorient(truth_t tt, const std::array<int, 16>& arr) {
		truth_t ret = 0;
		std::bitset<16> ttb = tt;
		for (int8_t i = 0; i < 16; i++) {
			ret |= (ttb[i] << arr[i]);
		}
		return ret;
	}

	inline static truth_t restore(truth_t tt, const std::array<int, 16>& arr) {
		truth_t ret = 0;
		auto arri = arr;
		for (int8_t i = 0; i < 16; i++) {
			arri[arr[i]] = i;
		}
		std::bitset<16> ttb = tt;
		for (int8_t i = 0; i < 16; i++) {
			ret |= (ttb[i] << arri[i]);
		}
		return ret;
	}


	inline truth_t negpermute_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = reorient(reorient(tt, negs[x.nInd]), perms[x.pInd]);
		ntt = x.flip ? ~ntt:ntt;
		return ntt;
	}

	inline truth_t permnegate_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = reorient(restore(tt, perms[x.pInd]), negs[x.nInd]);
		ntt = x.flip ? ~ntt:ntt;
		return ntt;
	}

	class npn_enumerator {
	private:
		rewrite_manager_t& mgr;
		truth_t rtt;
	public:
		npn_indices ix, lix;
		npn_enumerator(truth_t rtt, rewrite_manager_t& mgr) : mgr(mgr), rtt(rtt) {}

		inline truth_t negpermute_table(truth_t tt, const npn_indices& x) {
			return mgr.negpermute_table(tt, x);
		}

		bool next(truth_t& ntt);
	};


	void build_npn_perms();
	alit add_var(const std::string& nm, truth_t tt);
	alit add_and(alit a0, alit a1, truth_t tt, uint& nc, int& nlvl);
	alit add_xor(alit a0, alit a1, truth_t tt0, truth_t tt1, uint& nc, int& nlvl);
	int64_t move_to_dag(truth_t rtt, truth_t xtt, const cut_t& cut, alit& nal, bool& invert);
	void move_to_orig(const cut_t& cut, alit nr, bool invert);
	void dag_trans_fanin(id aid, oidset& tfanin);
	idset dag_trans_fanin_winputs(id aid);
	idvec topsort_dag_trans_fanin(id aid);
	void try_node(alit a0, alit a1, bool exor = false);
	id count_nodes(alit root);
	id count_nodes(alit a0, alit a1, bool exor);

	void topsort_cut(const aig_t& rntk, const cut_t& cut, idvec& andorder);
	truth_t simulate_cut(const aig_t& rntk);

	int rewrite_node(id aid);
	int count_unused_cone(const aig_t& net, id root);

	void write_dag_to_ss(std::stringstream& ss);
	void load_dag_from_string(const std::string& str);
	void write_prac_to_ss(std::stringstream& ss);
	void load_prac_from_string(const std::string& str);

	void rebuild_fanouts();
	void build_dagsorts();
	static void print_cut(const aig_t& ntk, const cut_t& cut);
	void build_canon_class(truth_t tt);

	void cleanup_dag();

	void print_indices(const npn_indices& indx);
	void print_dagnode(id dagnd);

	void get_relative_perms(truth_t rtt, truth_t dtt,
			std::bitset<4>& inmasks, std::array<int, 4>& inperms, bool& outmask);

	// isolated per-call cut enumeration
	void cut_enumerate(const aig_t& rntk, id aid, cutvec_t& cuts, uint K);
	int eval_gain(truth_t rtt, truth_t& best_dtt);
	int label_mffc(id root, id travid);
	int count_added_nodes(truth_t rtt, truth_t dtt, alit dgal);
};

} // namespace aig_ns

#endif /* SRC_AIG_RWR_H_ */
