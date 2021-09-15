/*
 * cut.cpp
 *
 *  Created on: May 3, 2020
 *      Author: kaveh
 */

#include "aig/cut.h"

namespace aig_ns {


cut_t trivial_cut(id aid) {
	cut_t ret;
	ret.root = aid;
	ret.inputs = {aid};
	return ret;
}

void print_cut(const aig_t& ntk, const cut_t& cut) {
	for (auto inid : cut.inputs) {
		std::cout << "INPUT(" << ntk.ndname(inid) << ")\n";
	}
	std::cout << "OUTPUT(" << ntk.ndname(cut.root) << ")\n";
	for (auto aid : cut.ands) {
		std::cout << ntk.to_str(aid) << "\n";
	}
}

void cut_enumerate_rec(const aig_t& ntk, idmap<cutvec_t>& cuts, uint K, uint S) {

	for (auto aid : ntk.inputs()) {
		cuts[aid] = {trivial_cut(aid)};
	}

	if (ntk.has_const0()) {
		id x = ntk.get_cconst0id();
		cuts[ntk.get_cconst0id()] = {trivial_cut(x)};
	}

	for (auto aid : ntk.top_andorder()) {

		const auto& nd = ntk.getcNode(aid);

		auto& ndcutvec = cuts[aid] = {trivial_cut(aid)};

		Oset<oidset> ndcutset;

		const auto& c0 = cuts.at(nd.fi0());
		const auto& c1 = cuts.at(nd.fi1());

		for (uint i = 0; i < c0.size(); i++) {
			for (uint j = i; j < c1.size(); j++) {
				cut_t newcut = c0[i];
				newcut.root = aid;
				newcut.ands.insert(aid);
				utl::add_all(newcut.inputs, c1[j].inputs);
				if (newcut.inputs.size() <= K) {
					size_t sz = ndcutset.size();
					ndcutset.insert(newcut.inputs);
					if (ndcutset.size() > sz) {
						utl::add_all(newcut.ands, c1[j].ands);
						ndcutvec.push_back(newcut);
					}
				}
			}
		}

	}

}

void cut_enumerate_local(const aig_t& ntk, idmap<cutvec_t>& cuts, uint K, uint S, bool mffc) {

	foreach_and(ntk, ad, aid) {
		auto& x = cuts[aid];
		cut_enumerate_pernode(ntk, aid, x, K, S, mffc);
	}

}

void cut_enumerate_pernode(const aig_t& ntk, id aid, cutvec_t& cuts, uint K, uint S, bool mffc) {
	//std::cout << "now on node: ";
	//ntk.print_node(aid);

	//assert(rntk.error_check());

	cuts = { {aid, oidset(), oidset({aid})} };
	idvec Q = {0};

	Oset<oidset> all_cuts;

	while(!Q.empty()) {

		id curind = Q.back(); Q.pop_back();
		auto cp = cuts[curind].inputs;

		for (auto nid : cp) {

			const auto& nd = ntk.getcNode(nid);
			if (nd.isAnd() && (!mffc | nid == aid | nd.fanouts.size() == 1)) {
				auto newcut = cuts[curind];

				newcut.inputs.erase(nid);
				newcut.ands.insert(nid);
				assert(nd.fi0() != nid);
				assert(nd.fi1() != nid);
				if (_is_not_in(nd.fi0(), newcut.ands))
					newcut.inputs.insert(nd.fi0());
				if (_is_not_in(nd.fi1(), newcut.ands))
					newcut.inputs.insert(nd.fi1());

				if ( !newcut.inputs.empty() && newcut.inputs.size() <= K
						&& newcut.inputs != cuts[curind].inputs
						) {
					size_t sz = all_cuts.size();
					all_cuts.insert(newcut.inputs);
					if (all_cuts.size() > sz) {
						cuts.push_back(newcut);
						Q.push_back(cuts.size() - 1);
					}
				}

			}
		}
	}

	std::cout << "cut num: " << cuts.size() << "\n";

}

void topsort_cut(const aig_t& rntk, const cut_t& cut, idvec& node_order) {

	assert(rntk.isAnd(cut.root));
	assert(_is_in(cut.root, cut.ands));

	idque deg_zero;
	for (auto aid : cut.inputs) {
		deg_zero.push(aid);
	}

	Omap<id, int> degmap;

	for (auto aid : cut.ands) {
		degmap[aid] = 2;
	}

	while(!deg_zero.empty()) {
		id curid = deg_zero.front(); deg_zero.pop();

		const auto& cnd = rntk.getcNode(curid);
		for (auto fout : cnd.fanouts) {
			if (_is_in(fout, cut.ands)) {
				if (--degmap.at(fout) == 0) {
					node_order.push_back(fout);
					deg_zero.push(fout);
					//std::cout << "adding " << rntk.ndname(fout) << "\n";
					if (fout == cut.root)
						break;
				}
			}
		}
	}

	assert(node_order.back() == cut.root);
	assert(node_order.size() == cut.ands.size());

}


} // namespace

