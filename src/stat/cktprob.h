/*
 * ckt_sat.h
 *
 *  Created on: Apr 30, 2016
 *      Author: kaveh
 */

#ifndef CKTPROB_H_
#define CKTPROB_H_

#include "utl/utility.h"
#include "base/circuit.h"
#include "base/blocks.h"

namespace cprb {

using namespace ckt;
using namespace utl;

typedef long double real;
typedef Hmap<id, real> id2realHmap;
typedef idmap<real> id2realmap;

void compute_probs();
void printprobs(int option);

void propagate_prob(const circuit& ckt, id2realmap& probMap);
real propagate_prob(const gate& gt, id2realmap& probMap);
void sim_and_propagate_prob(const circuit& cir, id2realmap& probmap);

void propagate_controllability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);
void propagate_controllability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);

void propagate_observability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);
void propagate_observability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);


}


#endif /* SRC_CKTPRROB_H_ */
