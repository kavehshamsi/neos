/*
 * opt.cpp
 *
 *  Created on: Aug 29, 2018
 *      Author: kaveh
 */
#include <utl/ext_cmd.h>
#include "opt/opt.h"

namespace opt_ns {

void aig_bddsweep(aig_t& aig, int s) {
	BDDAigSweep bddswp(aig, s);
	bddswp.sweep();
}

void BDDAigSweep::analyze() {

	std::cout << "called analyze for Aig BDDsweep\n";
	std::cout << "size limit is " << size_limit << "\n";
	auto tm = utl::_start_wall_timer();

	//std::cout << "size limit is " << size_limit << "\n";

	aig->add_const_to_map(wid2bddmap, _mgr.bddZero());

	for (auto inid : aig->inputs()) {
		wid2bddmap[inid] = _mgr.bddVar();
	}

	const idvec& gate_order = aig->top_andorder();

	bool premature = false;
	for (auto nid : gate_order) {
		analyze_time = utl::_stop_wall_timer(tm);
		if (time_budget != -1 && analyze_time > time_budget) {
			premature = true;
			break;
		}

		auto& nd = aig->getcNode(nid);

		auto& f0 = wid2bddmap.at(nd.fi0());
		auto& f1 = wid2bddmap.at(nd.fi1());

		bddvec fanins = {nd.cm0() ? ~f0:f0   ,   nd.cm1() ? ~f1:f1};
		BDD res = apply_op(_mgr, fanins, fnct::AND, size_limit);

		if (res.getNode() == NULL) {
			res = wid2bddmap[nid] = _mgr.bddVar();
		}
		else {
			//std::cout << "node " << cir->wname(gout) << " has size " << _mgr.nodeCount({res}) << "\n";
			wid2bddmap[nid] = res;
		}
		//std::cout << "num nodes at " << cir->wname(gout) << " "  << _mgr.nodeCount({res}) << "\n";

		if (_is_not_in(res.getNode(), bdd2widmap)) {
			_create_new_singleton_class(nid);
			bdd2widmap[res.getNode()] = nid;
		}
		else {
			id eq = bdd2widmap.at(res.getNode());
			_add_to_class(eq, nid);
			//std::cout << "equal nodes " << cir->wname(gout) << "  " << cir->wname(bdd2widmap.at(res.getNode())) << "\n";
		}
	}

	assert(!premature);
	if (premature) {
		for (auto nid : gate_order) {
			if (_is_not_in(nid, wid2bddmap)) {
				_create_new_singleton_class(nid);
			}
		}
	}

	analyze_time = utl::_stop_wall_timer(tm);
	std::cout << " time: " << analyze_time << "\n";

	print_equiv_classes();

}



} // namespace opt_ns
