/*
 * bal.h
 *
 *  Created on: May 5, 2020
 *      Author: kaveh
 */

#ifndef SRC_OPT_BAL_H_
#define SRC_OPT_BAL_H_

#include "aig/aig.h"

// TODO: this whole balance operation is under construction. do not use.

namespace opt_ns {

using namespace aig_ns;

class balance_manager_t {
private:
	int verbose = 0;
	aig_t* ntk = nullptr;

	struct supertree_t {
		id root;
		std::unordered_set<alit, alit_Hash> inputs;
		idset ands;
		id2idmap depthmp;
	};


public:
	balance_manager_t(aig_t& ntk) : ntk(&ntk) {}
	void balance_aig();

protected:
	void balance_aig_node(id aid);
	void build_shallowest_tree(supertree_t& supt, alit& newtip);
	void get_and_tree(id aid, supertree_t& supt);
};

} // namespace opt_ns


#endif /* SRC_OPT_BAL_H_ */
