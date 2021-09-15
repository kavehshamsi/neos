/*
 * dec.h
 *
 *  Created on: May 23, 2020
 *      Author: kaveh
 *      Description: header for decryption classes. Decryptors can derive from
 *      these classes for basic operations
 */

#ifndef SRC_DEC_DEC_H_
#define SRC_DEC_DEC_H_

#include <signal.h>

#include "base/circuit.h"
#include <boost/program_options.hpp>

namespace dec_ns {

using namespace ckt;

class decol {
public:
	uint num_keys = 0;
	uint num_ins = 0;
	uint num_ins_key = 0;
	uint num_outs = 0;
	uint num_dffs = 0;

	std::shared_ptr<const circuit> sim_cir;
	std::shared_ptr<const circuit> enc_cir;

	/* can be used to remember original enc_cir in case enc_cir is
	 simplified or adjusted */
	std::shared_ptr<const circuit> o_enc_cir;

	utl::bimap<id, id> enc2sim;
	id2idHmap ewid2ind, swid2ind;

	boolvec interkey;
	boolvec corrkey, o_corrkey;

	// options
	cpp_time_t start_time;
	int time_deadline = -1;
	int64_t iteration_limit = 5000;

	vector<string> pos_args;

	int verbose = 0;
	int solve_status = 0;

	bool print_help = false;
	bool is_sequential = false;
	bool use_verilog = false;
	bool sim_cyclic = false;
	bool enc_cyclic = false;
	string dec_method;

	// feedback arcs for cyclic circuits
	id2idHmap feedback_arc_set;
	idset feedback_wires;


	// known keys
	id2boolHmap knownkeys;
	idset knowncones;

public:

	decol() {}
	decol(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corr_key);
	virtual ~decol() {}

	void init_circuits(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corr_key, bool oracle_bin);
	void read_sim_enc_key_args(vector<std::string>& pos_args, bool bin_oracle);
	void create_enc2sim_map(void);

	// key verification routine
	int check_key(const boolvec& key, const id2boolHmap& constant_nodes = utl::empty<id2boolHmap>());
	int check_key_comb(const boolvec& key, const id2boolHmap& constant_nodes);
	int check_key_seq(const boolvec& key, const id2boolHmap& constant_nodes);
	double get_key_error(const boolvec& candkey, const boolvec& corrkey, bool per_output);
	void set_sim_cir(const circuit& sim_ckt);
	void set_enc_cir(const circuit& enc_ckt);
	void set_corrkey(const boolvec& corrkey);
	void gen_sim_cir_from_corrkey();
	static void create_port_indexmap(const circuit& cir, id2idHmap& port2indmap);

	static void get_opt_parser(decol& dc, boost::program_options::options_description& op);
	static void append_opt_parser(decol& dc, boost::program_options::options_description& op);

	virtual void solve() { neos_abort("not implemented"); }

	virtual void get_last_key() {}

	// only for small circuits
	void print_truth_table();

	void print_known_keys();

	uint hamming_distance(boolvec& vec1, boolvec& vec2);

	static void create_enc2sim_map(const circuit& simckt, const circuit& encckt, bimap<id, id>& enc2sim);

	static void check_key(const circuit& sim_ckt, const circuit& enc_ckt,
			const boolvec& key, const id2boolHmap& constant_nodes = utl::empty<id2boolHmap>());

	static void check_key(const circuit& enc_ckt, const boolvec& corr_key,
			const boolvec& hype_key, const id2boolHmap& constant_nodes = utl::empty<id2boolHmap>());

protected:

	// helper methods
	void _init_handlers();
	// safely set intermediate key by blocking signals while assignment
	void _safe_set_interkey(const boolvec& key);
	void _block_signals();
	void _unblock_signals();
	void _init_sigint_handler();
	void _init_deadline_handler();
	static void _deadline_handler(int signum);
	static void _sigint_handler(int signum, siginfo_t* u1, void* u2);
	static void check_key_arg(const circuit& enc_cir, const boolvec& crrk);

};

// oracle guided deobfuscation
class dec : public decol {
public:
	struct iopair_t {
		// boolvec sorted according to enc_cir.inputs()/outpus() order
		boolvec x;
		boolvec y;
		void clear() { x.clear(); y.clear(); }
	};

	// sequence of input patterns for sequantial circuits
	struct ioseq_t {
		vector< boolvec > xs;
		vector< boolvec > ys;
		vector< boolvec > nss;


		uint length() const {
			return xs.size();
		}
		void clear() {
			xs.clear();
			ys.clear();
		}

		ioseq_t() {}

		ioseq_t(uint len) :
			xs(vector<boolvec>(len, boolvec())),
			ys(vector<boolvec>(len, boolvec())),
			nss(vector<boolvec>(len, boolvec()))
		{}
	};

	string oracle_binary;
	utl::subproc_t oracle_subproc;

	dec() {}
	dec(const circuit& sim_ckt, const circuit& enc_ckt, const boolvec& corr_key = utl::empty<boolvec>(), bool oracle_bin = false) :
			decol(sim_ckt, enc_ckt, corr_key) {}
	virtual ~dec() {}

	static void get_opt_parser(dec& dc, boost::program_options::options_description& op);
	static void append_opt_parser(dec& dc, boost::program_options::options_description& op);

	double get_key_error(const boolvec& key, bool per_output = false, int num_patts = 500, int max_depth = 30);
	double get_key_error_seq(const boolvec& key, bool per_output = false, int num_patts = 500, int max_depth = 30);
	double get_key_error_comb(const boolvec& key, bool per_output = false, int num_patts = 500);

	virtual boolvec query_oracle(const boolvec& xvec);
	virtual vector<boolvec> query_seq_oracle(vector<boolvec>& xvecs);

};


} // dec_ns

#endif /* SRC_DEC_DEC_H_ */
