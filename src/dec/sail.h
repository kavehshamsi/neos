/*
 * sail.h
 *
 *  Created on: Oct 4, 2018
 *      Author: kaveh
 */

#ifndef SRC_MISC_SAIL_H_
#define SRC_MISC_SAIL_H_

#include "utl/ext_cmd.h"
#include "base/circuit.h"
#include "enc/enc.h"

namespace misc {

using namespace ckt;

void sail(const circuit& sim_ckt, int num_keys);

} // namespace misc

#endif /* SRC_MISC_SAIL_H_ */
