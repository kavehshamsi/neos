/*
 * qbf.h
 *
 *  Created on: Jun 3, 2021
 *      Author: kaveh
 */

#ifndef SRC_SAT_QBF_H_
#define SRC_SAT_QBF_H_

#include "sat/c_sat.h"

namespace csat {

void cir_to_qdimacs(const circuit& cir, const vector<idvec>& quantlists, const vector<bool>& isuniv, string& qdstr);

}

#endif /* SRC_SAT_QBF_H_ */
