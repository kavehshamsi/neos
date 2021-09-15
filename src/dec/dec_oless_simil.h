/*
 * oless.h
 *
 *  Created on: Nov 24, 2019
 *      Author: kaveh
 */

#ifndef SRC_OLESS_OLESS_H_
#define SRC_OLESS_OLESS_H_


#include "dec/dec.h"

#include "base/circuit.h"
#include "dec/dec_sat.h"
#include "aig/aig.h"
#include "opt/rwr.h"

namespace dec_ns {

using namespace ckt;
using namespace aig_ns;
using namespace opt_ns;

class dec_oless_simil : public decol {

protected:

	id2idHmap kid2index_map;
	std::shared_ptr<rewrite_manager_t> rwr_mgr;

public:

	dec_oless_simil(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corrkey) :
		decol(sim_cir, enc_cir, corrkey) {}

	int parse_args(int argc, char** argv) {}

	void run_0();
	void run_1(uint k, uint num_cut, uint cut_key_size);

	double get_metric(const aig_t& enc_aig);
};

} // namespace dec

#endif /* SRC_OLESS_OLESS_H_ */
