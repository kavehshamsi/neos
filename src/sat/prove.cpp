/*
 * prove.cpp
 *
 *  Created on: Jun 25, 2020
 *      Author: kaveh
 */


#include "sat/prove.h"

namespace prove {

using namespace opt_ns;


int prove_fraig0(const aig_t& miter_orig, const oidset& mit_outs, id2boolHmap& cex, int64_t max_prop_limit) {
	aig_t miter;
	return prove_fraig0(miter_orig, miter, mit_outs, cex, max_prop_limit);
}

int prove_fraig0(const aig_t& miter_orig, aig_t& miter, const oidset& mit_outs, id2boolHmap& cex, int64_t max_prop_limit, sat_solver* Sptr, const slitvec& assumps) {

	miter = miter_orig;

	opt_ns::rewrite_manager_t rwr_mgr;
	rwr_mgr.verbose = 0;
	int64_t prop_limit = 10000;
	int64_t mitt_prop_limit = 5000;
	int32_t zdd_limit = 200;
	double prop_limit_growth = 2;
	double zdd_limit_growth = 1.2;

	for (int r = 0; r < 100; r++) {

		sat_solver S;
		SATAigSweep swp(miter);

		if (Sptr) {
			S = *Sptr;
			swp.mp.set_external_solver(&S);
			if (!assumps.empty())
				swp.mp.assumps = assumps;
		}

		swp.verbose = 0;

		if (max_prop_limit != -1 && prop_limit > max_prop_limit) {
			std::cout << "reached prop limit";
			return sl_Undef;
		}

		if (prop_limit >= 300000 || r >= 6) {
			std::cout << "moving to unlimited SAT\n";
			prop_limit = -1;
		}
		else
			prop_limit = prop_limit * prop_limit_growth;

		swp.mp.prop_limit = 10000;
		swp.mp.mitt_prop_limit = prop_limit;
		std::cout << "Iter " << r << ": fraiging -> prop_limit: " << swp.mp.prop_limit << "\n";
		swp.mit_outs = mit_outs;
		swp.sweep();

		if (swp.mit_status != sl_Undef) {
			if (swp.mit_status == sl_True) {
				std::cout << "miter activated in fraig round " << r << "\n";

				for (const auto& p : swp.mit_active_cex) {
					//std::cout << miter.ndname(p.first) << "->" << p.second << " ";
					cex[p.first] = p.second;
				}

				return sl_True;
			}
			else if (swp.mit_status == sl_False) {
				std::cout << "miter proven UNSAT in fraig round " << r << "\n";
				return sl_False;
			}
		}
		else {
			std::cout << "fraig round " << r << " was inconclusive on miter status\n";
		}
		miter.print_stats();

		std::cout << "  rewriting ";
		// three rounds of rewriting
		rwr_mgr.rewrite_aig(miter);
		rwr_mgr.rewrite_aig(miter);
		rwr_mgr.rewrite_aig(miter);

		miter.print_stats();


	}

	return -1;

}

} // namespace prove
