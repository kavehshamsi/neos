/*
 * cut.h
 *
 *  Created on: May 3, 2020
 *      Author: kaveh
 */

#ifndef SRC_AIG_CUT_H_
#define SRC_AIG_CUT_H_

#include "aig/aig.h"

namespace aig_ns {

struct cut_t {
	id root = -1;
	oidset ands;
	oidset inputs;
};

typedef std::vector< cut_t > cutvec_t;

void print_cut(const aig_t& ntk, const cut_t& cut);
void cut_enumerate_rec(const aig_t& ntk, idmap<cutvec_t>& cutmap, uint K, uint S = 250);
void cut_enumerate_local(const aig_t& ntk, idmap<cutvec_t>& cutmap, uint K, uint S = 250, bool mffc = false);
void cut_enumerate_pernode(const aig_t& ntk, id aid, cutvec_t& cuts, uint K, uint S = 250, bool mffc = false);

void topsort_cut(const aig_t& rntk, const cut_t& cut, idvec& node_order);

} // namespace aig_ns


#endif /* SRC_AIG_CUT_H_ */
