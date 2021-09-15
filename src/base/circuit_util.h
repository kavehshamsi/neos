/*
 * circuit_sim.h
 *
 *  Created on: Jun 10, 2020
 *      Author: kaveh
 *      Description: Some circuit uitlities
 */

#ifndef SRC_BASE_CIRCUIT_UTIL_H_
#define SRC_BASE_CIRCUIT_UTIL_H_

#include "base/circuit.h"
#include <boost/dynamic_bitset.hpp>

namespace ckt_util {

using namespace ckt;

template<typename M>
auto simulate_gate_bitset(const gate& gt, const M& id2valMap) {

	typedef boost::dynamic_bitset<> dbitset;

	// meta programming magic
	static_assert(std::is_same<typename M::mapped_type, dbitset>::value);

	std::vector<const dbitset*> invals;

	int64_t sz = -1;
	for (auto gfan : gt.fanins) {
		try {
			invals.push_back(&(id2valMap.at(gfan)));
		}
		catch(std::out_of_range&) {
			neos_abort("gate simulation: input missing from value-map for gate "
					<< gt.nid << " and input " << gfan << "\n");
		}
	}

	dbitset outVal;

	switch (gt.gfun()) {
	case fnct::AND: {
		outVal = *invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal &= *invals[i];
		break;
	}
	case fnct::NAND: {
		outVal = *invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal &= *invals[i];
		outVal = ~outVal;
		break;
	}
	case fnct::OR: {
		outVal = *invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal |= *invals[i];
		break;
	}
	case fnct::NOR: {
		outVal = *invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal |= *invals[i];
		outVal = ~outVal;
		break;
	}
	case fnct::XOR: {
		outVal = *invals[0] ^ *invals[1];
		break;
	}
	case fnct::XNOR: {
		outVal = *invals[0] ^ *invals[1];
		outVal = ~outVal;
		break;
	}
	case fnct::MUX: {
		outVal = (~(*invals[0]) & *invals[1]) | (*invals[0] & *invals[2]);
		break;
	}
	case fnct::NOT: {
		outVal = ~(*invals[0]);
		break;
	}
	case fnct::BUF: {
		outVal = *invals[0];
		break;
	}
	default:
		neos_error("not supposed to bit-parallel simulate this gate");
	}


	/*std::cout << outVal << "  =  " << funct::enum_to_string(gt.gfun()) << " (";
	for (auto val : invals) {
		std::cout << val << ", ";
	}
	std::cout << ");\n"; */


	return outVal;
}

template<typename M>
void simulate_comb_bitset(const circuit& cir, M& id2valMap) {
	for (auto gid : cir.get_topsort_gates()) {
		const auto& g = cir.getcgate(gid);
		if (!g.isLatch()) {
			id2valMap[g.fo0()] = simulate_gate_bitset(g, id2valMap);
		}
	}
}

} // namespace ckt_util

#endif /* SRC_BASE_CIRCUIT_UTIL_H_ */
