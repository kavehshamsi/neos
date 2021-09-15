/*
 * bmc.h
 *
 *  Created on: Sep 2, 2018
 *      Author: kaveh
 */

#ifndef SRC_MC_BMC_H_
#define SRC_MC_BMC_H_

#include "sat/sat.h"
#include "sat/c_sat.h"
#include "base/circuit.h"
#include "mc/unr.h"
#include "opt/opt.h"

namespace mc {

using namespace ckt;
using namespace utl;
using namespace csat;

using std::cout;

using boost::format;
using std::cout;

void _unroll_ckt_old(const circuit& cir, circuit& uncir,
		int numFrames, const oidset& frozens, bool zero_init = true);

void _unroll_aig(const aig_t& aig, aig_t& uaig,
		int numFrames, const oidset& frozens, bool zero_init = true);

/*
 * a class for an internal BMC solver
 * increamental unrolling combined with SAT solving is used
 */
class Bmc {
public:

	oidset frozenPis;

	sat_solver* BmcS = nullptr; // can be provided and modified from outside
	sat_solver TrS;

	id2litmap_t umap;
	std::vector<id2litmap_t> lmaps;
	std::vector<id2litmap_t> trlmaps; // litmaps for transition relation
	int tran_simp_stagnant = 0;

	std::vector<slit> bmc_assumps; // assumption vector for incremental solving
	std::vector<slit> bmc_inits; // list of clock-0 state literals
	slit bmc_init_lit; // set this lit to 1 to activate init state

	std::vector<id2litmap_t> in2lit_maps; // necessary for frame data

	int cexLen = -1;
	int checkedBound = 0;
	int currentlyUnrolled = 0;
	int timeLimit = -1;
	int bmccount = 0;

	std::vector<boolvec> cex; // includes inputs per frame

	struct Bmc_config_t {

		/*
		 * use aig's for representation
		 */
		bool aig_mode = false;

		/* tr simplification
		 * 0 : no simplification
		 * 1 : last-frame simplify
		 * 2 : multi-frame incremental
		 */
		int tran_simp = 0;

		// sat simplification
		int sat_simp = 0;

		/* simplify relative to existing solver
		 * 0 : independent
		 * 1 : relative to BmcS
		 */
		bool relative_simp = 0;

		int propagation_limit = -1;

	} config;

	Bmc() : BmcS(NULL) {}
	Bmc(Bmc_config_t c) : BmcS(NULL), config(c) {}

	virtual ~Bmc() { if (BmcS) delete BmcS; }

	virtual int bmc_solve(int newBound, id out) = 0;
	virtual bool getValue(id wid, int f) = 0;
	inline slit getlit(int f, id wid) { return lmaps.at(f).at(wid); }
	virtual slit getlit(int f, const std::string& nm) = 0;
	bool hasCex() { return cexLen != -1; }

protected:
	inline void _restart_solver();
};

/*
 * a class for an internal BMC solver
 * increamental unrolling combined with SAT solving is used
 */
class BmcCkt : public Bmc {
public:

	circuit bmcckt;
	circuit tranckt; // transition relation circuit
	CircuitUnroll bmc_unr;

	oidset frozenWires; // wires with an exclusivelye frozen fanin
	oidset frozenGates; // gates with an exclusivelye frozen fanin

	BmcCkt() {}
	~BmcCkt() {}
	BmcCkt(const circuit& ckt, const oidset& frozens,
			const Bmc_config_t& config);

	int bmc_solve(int newBound, id out = -1);
	bool getValue(id wid, int f);

	slit getlit(int f, const std::string& nm) { return lmaps[f].at(tranckt.find_wcheck(nm)); }

	void update_frozen_cone();
	void restart_to_unrolled_ckt(const CircuitUnroll& cunr);

protected:
	int _solve_int(int newBound, id out);
	void _simplify_tr(int f);
	void _simplify_tr_single(int f);
	static void _add_frame_wire_vars(sat_solver& S, const circuit& ckt,
			std::vector<id2litmap_t>& vmaps, int f, const oidset& frozen_inputs);
	static void _add_frame_gate_clauses(sat_solver& S, const circuit& ckt,
			std::vector<id2litmap_t>& vmaps, int nFrame, const oidset& frozen_gates);

};


/*
 * a class for an internal BMC solver
 * increamental unrolling combined with SAT solving is used
 */
class BmcAig : public Bmc {
public:


	aig_t tranaig; // transition relation circuit
	AigUnroll unraig;

	BmcAig() {}
	~BmcAig() {}
	BmcAig(const aig_t& ntk, const oidset& frozens,
			const Bmc_config_t& config);

	int bmc_solve(int newBound, id out);
	bool getValue(id wid, int f);

	// restart to a simplified unrolling of already verified frames where outname is assumed asserted
	void restart_to_unrolled(const AigUnroll& new_unraig, const std::string& outname);

	slit getlit(int f, const std::string& nm) {
		try {
			return lmaps[f].at(tranaig.find_wcheck(nm));
		}
		catch(std::out_of_range&) {
			neos_abort("could not find " << nm << " at frame " << f << " in var map");
		}
	}

protected:
	void _simplify_tr(int f);
};


inline void Bmc::_restart_solver() {
	if (BmcS) delete BmcS;
	BmcS = new sat_solver();
	//BmcS->reseed_solver();
}


}


#endif /* SRC_MC_BMC_H_ */
