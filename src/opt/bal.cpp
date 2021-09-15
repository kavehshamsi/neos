/*
 * bal.cpp
 *
 *  Created on: May 5, 2020
 *      Author: kaveh
 */

#include "opt/bal.h"
#include "utl/ext_cmd.h"

namespace opt_ns {

void balance_manager_t::balance_aig() {

	aig_t orig_ntk = *ntk;

	ntk->print_stats();

	idvec aorder = ntk->top_andorder();
	idvec revorder = aorder;
	std::reverse(revorder.begin(), revorder.end());

	for (auto aid : revorder) {
		if (ntk->node_exists(aid))
			balance_aig_node(aid);
	}

	//ntk->remove_dead_logic();

	ntk->print_stats();

	ext::check_equivalence_abc(orig_ntk, *ntk);

}

void balance_manager_t::build_shallowest_tree(supertree_t& supt, alit& newtip) {
	std::multimap<int, alit> level2nid;

	id num_ands = ntk->nAnds();

	for (auto xl : supt.inputs) {
		level2nid.insert(std::make_pair(supt.depthmp.at(xl.lid()), xl));
	}

	V1(
	std::cout << "levels: ";
	for (auto& p : level2nid) {
		std::cout <<  p.first << ":" << ntk->ndname(p.second) << "  ";
	}
	std::cout << "\n";
	);

	std::cout << "num ands: " << supt.ands.size() << "\n";

	while (level2nid.size() > 1) {
		auto it0 = level2nid.begin();
		alit x0 = it0->second;
		level2nid.erase(it0);

		auto it1 = level2nid.begin();
		alit x1 = it1->second;
		level2nid.erase(it1);

		alit yl = ntk->add_and(x0, x1);
		int ylvl = MAX(ntk->nlevel(x0.lid()), ntk->nlevel(x1.lid())) + 1;
		level2nid.insert(std::make_pair(ylvl, yl));
	}

	newtip = level2nid.begin()->second;

	id added_ands = (ntk->nAnds() - num_ands);
	std::cout << "added: " << added_ands << " " << supt.ands.size() << "\n";
	assert(added_ands <= supt.ands.size());
}

void balance_manager_t::balance_aig_node(id aid) {

	assert(ntk->node_exists(aid));
	const auto& nd = ntk->getcNode(aid);
	assert(ntk->isAnd(aid));

	supertree_t supt;
	get_and_tree(aid, supt);

	if (supt.ands.size() > 1) {
		alit newtip;
		build_shallowest_tree(supt, newtip);
		if (newtip.lid() != aid) {
			ntk->move_all_fanouts(aid, newtip.lid(), newtip.sgn());
			ntk->strash_fanout(newtip.lid());
			ntk->remove_node_recursive(aid);
		}
	}

}

void balance_manager_t::get_and_tree(id aid, supertree_t& supt) {

	idque Q;
	Q.push(aid);

	supt.root = aid;
	idset visited;

	V1(std::cout << "\nsupt root : " << ntk->ndname(supt.root) << "\n");

	while (!Q.empty()) {

		id cnid = Q.front();
		Q.pop();
		visited.insert(cnid);
		const auto& cnd = ntk->getcNode(cnid);

		supt.ands.insert(cnid);
		V1(std::cout << "   " << ntk->to_str(cnid) << "\n");
		for (auto fil : {cnd.fanin0, cnd.fanin1}) {
			const auto& innode = ntk->getcNode(fil.lid());
			if ( _is_in(fil.lid(), visited) )
				continue;
			if (fil.sgn() || innode.isInput() || innode.fanouts.size() > 1) {
				supt.inputs.insert(fil);
				supt.depthmp[fil.lid()] = ntk->nlevel(fil.lid());
			}
			else {
				Q.push(fil.lid());
			}
		}

	}

	V1(
	std::cout << "  inputs:";
	for (auto xl : supt.inputs) {
		std::cout << ntk->ndname(xl.lid()) << ":" << supt.depthmp.at(xl.lid()) << "  ";
	}
	std::cout << "\n";
	);
}

} // namespace opt_ns


