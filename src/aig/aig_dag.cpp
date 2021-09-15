/*
 * aig.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: kaveh
 */

#include "aig/aig.h"
#include <limits>

namespace aig_ns {

void aig_t::clear_orders() const {
	const_cast<aig_t*>(this)->order_needs_update = true;
}

idvec aig_t::top_andorder() const {
	idvec and_order;
	for (auto aid : top_order()) {
		if (isAnd(aid))
			and_order.push_back(aid);
	}
	return and_order;
}

const idvec& aig_t::top_order() const {

	if (!order_needs_update)
		return top_order_vec;


	// tricking C++ into thinking this function is const
	aig_t& ta = const_cast<aig_t&>(*this);
	ta.top_order_vec.clear();

	idmap<int> ckt_indegmap;

	foreach_node((*this), nd, nid) {
		if (nd.isOutput() || nd.isLatch())
			ckt_indegmap[nid] = 1;
		else
			ckt_indegmap[nid] = 2;
	}

	// add all zero fanin nodes
	push_all(ta.top_order_vec, combins());

	if (has_const0())  {
		ta.top_order_vec.push_back(get_cconst0id());
	}

	idque deg_zero;
	for (auto aid : ta.top_order_vec) {
		ta.getNode(aid).depth = 0;
		deg_zero.push(aid);
	}

	while (!deg_zero.empty()) {

		// dequeue and push to ordering
		id aid = deg_zero.front(); deg_zero.pop();

		//std::cout << "\nat " << ncName(aid) << " " << (int)ntype(aid) << ": ";
		// reduce indegree for adjacent vertices

		for (auto aid2 : nfanouts(aid)) {
			//std::cout << " " << ncName(aid2) << " ";
			// enque if the deg reaches 0
			if ((--ckt_indegmap.at(aid2)) == 0) {
				deg_zero.push(aid2);
				ta.top_order_vec.push_back(aid2);
			}
		}
	}

	//std::cout << top_order_vec.size() << " " << nNodes() << "\n";

	assert(top_order_vec.size() == nNodes());

	for (auto aid : top_order_vec) {
		auto& nd = ta.getNode(aid);
		if (nd.isAnd()) {
			nd.depth = MAX(ta.nlevel(nd.fi0()), ta.nlevel(nd.fi1())) + 1;
		}
		else if (nd.isOutput()) {
			nd.depth = ta.nlevel(nd.fi0()) + 1;
		}
	}

	ta.order_needs_update = false;

	return top_order_vec;
}

idset aig_t::trans_fanout(id aid) const {

	idset tfanout;
	idque Q;
	Q.push(aid);
	tfanout.insert(aid);

	while ( !Q.empty() ) {

		id curid = Q.front();
		Q.pop();
		for (auto fanout : nfanouts(curid)) {
			if (_is_not_in(fanout, tfanout)) {
				Q.push(fanout);
				tfanout.insert(fanout);
			}
		}
	}

	return tfanout;
}

// topsorted transfanin
idvec aig_t::topsort_trans_fanin(id aid) {

	idvec tsfanin;
	idset tfanin = trans_fanin(aid);
	Hmap<id, int> degmap;
	idque deg_zero;

	for (auto tf : tfanin) {
		for (auto fo : nfanouts(tf)) {
			if (_is_in(fo, tfanin)) {
				degmap[tf]++;
			}
		}
	}

	deg_zero.push(aid);
	tsfanin.push_back(aid);

	while (!deg_zero.empty()) {
		id curid = deg_zero.front(); deg_zero.pop();
		const auto& nd = getcNode(curid);
		for (auto fanin : {nd.fi0(), nd.fi1()}) {
			if (fanin != -1 && --degmap[fanin] == 0) {
				deg_zero.push(fanin);
				tsfanin.push_back(fanin);
			}
		}
	}

	std::reverse(tsfanin.begin(), tsfanin.end());

	return tsfanin;
}


idset aig_t::trans_fanin(id aid) const {
	idset pis;
	return trans_fanin(aid, pis);
}

idset aig_t::trans_fanin(id aid, idset& pis) const {

	idset tfanin;
	idque Q;
	Q.push(aid);
	tfanin.insert(aid);

	while ( !Q.empty() ) {

		id curid = Q.front();
		Q.pop();

		const auto& nd = getcNode(curid);

		if (nd.isInput())
			pis.insert(curid);

		for (auto fanin : {nd.fi0(), nd.fi1()} ) {
			if (_is_not_in(fanin, tfanin) && fanin != -1) {
				Q.push(fanin);
				tfanin.insert(fanin);
			}
		}
	}

	return tfanin;
}


} // namespace aig


