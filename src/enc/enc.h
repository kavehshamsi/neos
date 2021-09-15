/*
 * enc.h
 *
 *  Created on: May 18, 2016
 *      Author: kaveh
 */

#ifndef ENC_H_
#define ENC_H_

#include "utl/utility.h"
#include "utl/cktgraph.h"
#include "sat/c_sat.h"
#include "base/circuit.h"
#include "base/blocks.h"
#include "main/main_data.h"

namespace enc {

// #define CAMOU

using namespace ckt;
using namespace utl;




// gate insertion pair
struct gcon_pair {
	id gate_B;
	id gate_O;
};

enum method {
	DL,	// Dummy Logic (DL) obfuscation
	CO,	// Corrected Output (CO)
	ANT,	// Anti-SAT

	XRND,	// Random XOR/XNOR insertion
	LT,	// LUT insertion
	MXRND,	// MUX insertion

	U	// undefined
};

enum integ_method {
	RAN,	// random integration
	HI		// high controllability signals
};


class enc_wire {
private:

	circuit& ckt;

	std::map<id, id> fanin_size_map;

	std::vector<idset> candidate_wire_sets; // vector of cadidate wire sets
	idvec all_wires_vec; // this is useful in traversals


public:

	struct enc_wire_options {
		std::string method = "mmux";
		std::string selection_method = "rnd";
		std::string mux_cell_type;
		bool keep_acyclic = false;
		uint num_muxs;
		uint mux_size;
		float intensity = 0.0;
		uint num_cbs = 1;

		int parse_command_line(const std::vector<std::string>& pos_args);
		void print_options();
	} opt;

	// enc needs to access this
	std::map<id, idvec> muxtrees;

	enc_wire(circuit& ckt, enc_wire_options& opt) :
		 ckt(ckt), opt(opt) {}

protected:

	// internal methods

	// get candidate wire set
	void _get_candidate_wires_kcut();
	void _get_candidate_wires_diag();
	void _get_candidate_wires_wirecone();

	// pick from candidate set
	void _muxtrees_from_candidates_crosslock();
	void _muxtrees_from_candidates_mmux();

	// mmux specific pick strategies
	void _topsort_mmux_pick();
	void _cg_mmux_pick(float cgness);

	// check whether enough wires are in set for mmux/crosslock
	bool _check_wobf_feasibility(const idset& wrset);

	void _build_transfanin_sizemap(const circuit& cir);

	void _get_all_wires_vec();
	void _decode_wobf_pick_method(const std::string& method);

public:

	void _pick_crosslock_wires();
	void _pick_manymux_wires();

	static void _find_depthmap (id root, const circuit& ckt,
			std::map<id, int>& depth_map, idvec& sorted_gates);

	static void _open_mmux_wires(circuit& ckt,
			idvec& muxout_wires, std::map<id, idvec>& muxin_wires);

	// for inserting the locking circuit structures given a muxtree map
	static void _insert_muxtrees(circuit& ckt, std::map<id, idvec>& muxin_maps,
			const std::string& mux_cell_type);

};

class encryptor {
public:
	std::string description;
	std::string usage_message;
	std::vector<int> num_params;

	typedef std::function<void(const std::vector<std::string>&, circuit&, boolvec&)> enc_fun_t;
	enc_fun_t enc_fun;

	encryptor() {}
	encryptor(const std::string& description,
			const std::string& usage_message,
			std::vector<int> num_params,
			enc_fun_t enc_fun) :
				description(description),
				usage_message(usage_message),
				num_params(num_params),
				enc_fun(enc_fun) {}
};

// holds map for dispatching to encryptor functions
class enc_manager_t {
public:
	std::map<std::string, encryptor> encryptor_map;

	enc_manager_t();
};

// encrypt class with static members
class encrypt {
private:
	circuit *ckt;
	circuit *enc_ckt;

	const bool static randomsource = false;
	const bool static tri_buf = true;

public:
	// struct for keeping optimization score
	struct wire_enc_score {

		// weights in cost function
		static constexpr double scc_size_weight = -1;
		static constexpr double feedback_arc_weight = 0.1;
		static constexpr double max_scc_density_weight = 3;

		double feedback_arc_size;
		double scc_size;
		double max_scc_density;

		double total;

		void compute_total() {
			total = scc_size_weight * scc_size
					+ feedback_arc_weight * feedback_arc_size
					+ max_scc_density_weight * max_scc_density;
		}

		void print_score() {
			std::cout << "\nscc_size : " << scc_size;
			std::cout << "\nmax_scc_density : " << max_scc_density;
			std::cout << "\nfeedback_arc_weight : " << feedback_arc_weight << "\n";
		}
	};

	// fully programmable logic obfuscation
	struct univ_opt {
		int num_ins = -1;
		int num_outs = -1;
		int width = -1;
		enum profile_t {CONE, SQUARE} profile;
		int gate_type = 0; // 0:LUT 1:AND
		int depth = -1;
		int backstep = 0;
		float indeg = 1;
	};

public:
	encrypt() :
		ckt(NULL),
		enc_ckt(NULL)
		{}

	// verbosity control
	const static bool verbose = false;

	static boolvec enc_antisat(circuit& cir, int ant_width, int num_trees = 1,
			bool pick_internal_ins = false, bool pick_internal_outs = false, int xor_obf_tree = 0, const idvec& roots = {});

	static boolvec enc_sfll_point(circuit& cir, int lut_width, int num_inserts = 1,
			bool pick_internal_ins = false, bool pick_internal_outs = false, int xor_obf_tree = 0, const idvec& roots = {});

	static boolvec enc_antisat_dtl(circuit& cir, int tree_width, int num_first_layer, fnct flip_fun, int num_inserts = 1,
			bool pick_internal_ins = false, bool pick_internal_outs = false, int xor_obf_tree = 0, const idvec& roots = {});

	static boolvec enc_sfll_dtl(circuit& cir, int tree_width, int num_first_layer, fnct flip_fun, int num_inserts = 1,
			bool pick_internal_ins = false, bool pick_internal_outs = false, int xor_obf_tree = 0, const idvec& roots = {});

	static gcon_pair insert_tree(circuit& cir, ckt_block::tree& tree,
			const idvec& inputs, id output, fnct insert_gatefun);
	static gcon_pair insert_tree(circuit& cir, ckt_block::tree& tree,
			const idvec& inputs, id output, fnct insert_gatefun, id2idmap& added2new);

	static void diversify_tree(ckt_block::tree& tir, Omap<idpair, fnct> flipmap, bool is_double);

	// high error gate level obfuscation
	static boolvec enc_xor_rand(circuit& ckt, const boolvec& key, fnct gtype = fnct::XOR,
			const idset& avoid_set = empty<idset>());
	static boolvec enc_xor_prob(circuit& ckt, const boolvec& key,
			const idset& avoid_set = empty<idset>());
	static boolvec enc_xor_fliprate(circuit& ckt, const boolvec& key, uint32_t num_patts,
				const idset& avoid_set = empty<idset>());
	static boolvec enc_parity(circuit& ckt, uint tree_size,
			uint keys_per_tree, uint num_trees);
	static boolvec enc_lut(uint rounds, uint width, circuit& ckt);
	static boolvec enc_error(uint rounds, uint width, circuit& ckt);

	// wire obfuscation
	static boolvec enc_manymux (circuit& ckt, enc_wire::enc_wire_options& opt);
	static boolvec enc_shfk (circuit& ckt);
	static boolvec enc_loopy_wMUX (id width, id minlooplen, circuit& ckt);

	static bool enc_tbuflock(circuit& cktm, id num_bufs, int method);

	static bool enc_interconnect(circuit& cir, enc_wire::enc_wire_options& opt);

	static bool enc_universal(circuit& cir, univ_opt& opt);
	static void get_universal_circuit(circuit& ucir, univ_opt& opt);
	static void get_universal_simckt(circuit& ucir, univ_opt& opt);

	static void get_universal_circuit(circuit& ucir, int gate_type, int num_ins,
			int num_outs, std::vector<int>& widths, int depth, float indeg, int backstep);
	static void get_univ_muxfanins(idvec& muxfanins, const std::vector<idvec>& layer_ins,
			float indeg, int backstep, int l, int w);

	static void layer_stats(circuit& ckt);

	static boolvec enc_swbox(circuit& ckt, const oidset& shuffle);

	static bool completemux(circuit& ckt, float intensity);

	// mux removal attack for cyclic obfuscated circuits
	static void mux_removal_attack(const circuit& ckt);

	// simple cyclification
	static void cyclify(circuit& ckt, int num_edges);

	// gate insertion methods
	static id insert_xor(circuit& ckt, id wr, circuit& encxor);
	static gcon_pair insert_gate(circuit& ckt, id wr, fnct fnct, string str);
	static gcon_pair insert_key_gate(circuit& cir, id wid, bool key, fnct gtype);
	static void insert_INV(circuit& ckt, id wr);
	static void insert_2mux(id wr, id otherend, circuit& ckt, id namestamp);
	static void insert_2muxgate(id wr, id otherend, circuit& ckt, id namestamp);


	// sequential obfuscation
	static void enc_seq(circuit& ckt);

	static void enc_metric_adhoc(circuit& ckt, uint steps, uint target_mux, uint target_xor);
	static double propagate_metric_adhoc(const circuit& ckt, bool silent = false);
	static double structural_tests(const circuit& ckt, bool silent = false);
	static void enc_xmux(circuit& ckt, uint num_muxes, uint num_xors);


	static void enc_absorb(const string& verilog_filename, const string& outfile, int min_width = 32,
			int num_absorbs = 1, int min_capacity = 1, int num_consts_lock = 0);

protected:

	static void _compute_score(const circuit& tmp_ckt, wire_enc_score& scr);
	static void _wire_enc_optimization(const circuit& tmp_ckt,
			id numbuf, std::set<idpair>& edge_set);
	static double _max_scc_density(const circuit& tmp_ckt,
			std::vector<idset>& sccs);

	static bool _pick_tree_input_outputs(id& output, idset& treeins, circuit& cir, int width,
			bool pick_internal_ins = true, bool pick_internal_outs = true, bool kcut = false);
	static bool _pick_tree_inputs(idset& treeins, circuit& cir,
			id output, int ant_width, bool pick_internal);

	static bool _pick_from_fanin(idset& retset, const circuit& ckt, id r,
			id size, id wrange = -1);

	static id _inc_input_width(id gu, circuit& ckt);
	static bool _feed_back(id gu, id gv, circuit& ckt, id& edge_count);
	static bool _feed_back_edgeonly(id gu, id gv, circuit& ckt);
	static void _make_path_removable(idvec& pathwires,
			circuit& ckt, id& edge_count);
	static void turn_to_keywire(circuit& ckt, id wr, int offset = -1);
	static bool _pick_wires_random(idset& retset, const circuit& ckt,
			id count, int opt, id wrange = -1);
	static bool _pick_wires_random(idvec& retvec, const circuit& ckt,
			id count, int opt, id wrange = -1);

	// removal attack on cyclic obfuscation
	static bool _is_key_mux(id gid, const circuit& ckt);
	static void _get_mux_cone_inputs(id gid, const circuit& ckt,
			idvec& muxins_vec);
	static bool _is_root_key_mux(id gid, const circuit& ckt);
	void _wire_output_dependency(const circuit& ckt,
			std::vector<idset>& id2reachable);

public :
	static id _find_path(id u, circuit& ckt, uint len, idvec& pathwires);
};


}

#endif /* ENC_H_ */
