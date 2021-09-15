/*
 * unr.h
 *
 *  Created on: Feb 19, 2019
 *      Author: kaveh
 *      Description: Dynmaic unrolling and simplification, can be used
 *      in BMC and sequential deobfuscation. supports simplification across multiple
 *      frames.
 */

#ifndef SRC_MC_UNR_H_
#define SRC_MC_UNR_H_

#include "base/circuit.h"
#include "aig/aig.h"
#include "opt/opt.h"
#include "sat/sat.h"
#include "sat/c_sat.h"

namespace mc {

using namespace ckt;
using namespace sat;
using namespace csat;
using namespace aig_ns;
using namespace opt_ns;

class Unroll {
protected:
	oidset init_wires;
	uint nf = 0; // num current frames

public:
	// frozen nodes of the translation relation
	oidset frozen_tr;

	sat_solver* S = nullptr;

	bool zero_init = false;

	virtual ~Unroll() {}

	virtual void add_frame() = 0;
	void extend_to(uint f);
	uint cur_len() const { return nf; }
	//virtual void sweep_multi_frame() = 0;
	//virtual void sweep_single_frame();
	//virtual id tran2unr(id tr, int f) = 0;
	//virtual std::pair<id, int> unr2tran(id ur) = 0;
};


class AigUnroll : public Unroll {
public:
	aig_t tran; // transition relation. sequential AIG with DFFs
	aig_t unaig; // unrolled AIG

	std::vector<id2alitmap> c2umaps;

	// will include new nodes on each frame. used in incremental BMC etc.
	idset new_ands;

	~AigUnroll() {}
	AigUnroll() {}
	AigUnroll(aig_t& tran) : tran(tran) {}

	void set_tr(const aig_t& ntk, const oidset& frozens, bool zero_init = false);

	void add_frame();
	void tr_sat_sweep(SATAigSweep::sweep_params_t& swp);
	void sweep_unrolled_aig();
	void sweep_unrolled_aig(SATAigSweep& ssw);
	void sweep_unrolled_aig(SATAigSweep::sweep_params_t& swp);

	aig_t get_zeroinit_unaig();

public:
	friend class BmcAig;
};

class CircuitUnroll : public Unroll {
private:
	idset frozen_wfanouts;
	idset frozen_gfanouts;

public:
	circuit tran; // transition relation. sequential circuit with DFFs
	circuit uncir; // unrolled circuit

	bool needs_frozen_update = true;

	std::vector<id2idmap> c2umaps;
	std::vector<idset> frame_gates;

	~CircuitUnroll() {}
	CircuitUnroll() {}
	CircuitUnroll(const circuit& cir, const oidset& frozens, bool zero_init = false) {
		set_tr(cir, frozens, zero_init);
	}

	void set_tr(const circuit& cir, const oidset& frozens, bool zero_init = false);

	void add_frame();
	void tr_sat_sweep(SATCircuitSweep::sweep_params_t& swp);
	void sweep_unrolled_ckt();
	void sweep_unrolled_ckt(SATCircuitSweep::sweep_params_t& swp);
	int32_t sweep_unrolled_ckt_abc();

	circuit get_zeroinit_uncir();

protected:
	void update_frozen_fanout();
	id create_uncir_wire(id trwid);

public:
	friend class BmcCkt;
};


inline std::string _primed_name(const std::string& nm) { return nm + "_$1"; }
inline std::string _unprime_name(const std::string& nm) { return nm.substr(0, nm.length() - 3); }
inline std::string _frame_name(const std::string& nm, int f) { return fmt("%s_%d", nm % f); }
inline std::pair<std::string, int> _unframe_name(const std::string& nm) {
	int f = 0;
	if (_is_in("_init$", nm))
		return std::make_pair(nm.substr(0, nm.length() - 6), f);

	auto pos = nm.find_last_of('_');
	try {
		assert(pos != string::npos);
		f = stoi(nm.substr(pos+1, nm.length()));
	}
	catch(std::invalid_argument&)
		neos_error("problem unframing the name " << nm);

	return std::make_pair(nm.substr(0, pos), f);
}
inline bool _is_primed_name(const std::string& nm) { return _is_in("_$1", nm); }
inline std::string _init_name(const std::string& nm) { return nm + "_init$"; }

void make_frozen_with_latch(circuit& cir, oidset fzs);

}


#endif /* SRC_MC_UNR_H_ */
