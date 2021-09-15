
#include "circuit.h"

//////////////// The circuit class ////////////////////

namespace ckt {

using namespace utl;

using std::vector;

bool circuit::simulate_gate(id gid, const id2boolmap& id2valMap) const {
	const gate& gt = getcgate(gid);
	return simulate_gate(gt, id2valMap);
}

/*template<typename T>
void circuit::simulate_comb(const id2boolHmap& inputmap,
		const id2boolHmap& keymap, id2boolmap& simMap) const {
	id2boolHmap inMap = inputmap;
	for (auto& p : keymap) {
		inMap[p.first] = p.second;
	}
	simulate_comb(inMap, simMap);
}*/

// combinational gate-level simulation
void circuit::simulate_comb(const id2boolHmap& inputmap, id2boolmap& simMap) const {

	for (auto inid : inputs()) {
		simMap[inid] = inputmap.at(inid);
	}
	for (auto inid : keys()) {
		simMap[inid] = inputmap.at(inid);
	}

	simulate_comb(simMap);
}

void circuit::simulate_inst(const gate& g, id2boolmap& simMap) const {
	typedef std::function<boolvec(const boolvec&)> sim_fun_t;
	const auto& simfn = g.attch.get_cattatchment<sim_fun_t>("simfun");


	const auto& cln = cell_info_of_inst(g.nid);

	uint i = 0;
	boolvec faninsim(g.fanins.size());
	for (auto gi : g.fanins) {
		faninsim[cln.fanin_port_inds[i++]] = simMap.at(gi);
	}
	boolvec outs = simfn(faninsim);
	i = 0;
	for (auto go : g.fanouts) {
		simMap[go] = outs[cln.fanout_port_inds[i++]];
	}
}

void circuit::simulate_comb(id2boolmap& simMap) const {

	simMap.resize(get_max_wid());
	add_const_to_map(simMap);

	const idvec& gate_order = get_topsort_gates();
	// assert(gate_order.size() == nGates());

	for (auto gid : gate_order) {
		const auto& g = getcgate(gid);
		switch (g.gfun()) {
		case fnct::DFF:
			break;
		case fnct::INST: {
			simulate_inst(g, simMap);
			break;
		}
		default:
			simMap[g.gfanout()] = simulate_gate(g, simMap);
		}
	}
}

void circuit::simulate_comb_tern(id2ternmap& simMap) const {

	add_const_to_map(simMap, tern_t(tern_t::ZERO), tern_t(tern_t::ONE));

	const idvec& gate_order = get_topsort_gates();

	for (auto gid : gate_order) {
		if ( !isLatch(gid) )
			simMap[gfanout(gid)] = simulate_gate_tern(gid, simMap);
	}
}

void circuit::fill_inputmap_random(id2boolmap& simmap, bool doins, bool dokeys, bool dolatches) {

	if (doins)
		for (auto inid : inputs())
			simmap[inid] = (rand() % 2 == 0);
	if (dokeys)
		for (auto kid : keys())
			simmap[kid] = (rand() % 2 == 0);

	if (dolatches)
		foreach_latch(*this, dffid, dffout, dffin)
			simmap[dffout] = (rand() % 2 == 0);

	add_const_to_map(simmap);
}


// sequential simulation
void circuit::simulate_seq(const std::vector<id2boolmap>& inputmaps, std::vector<id2boolmap>& simmaps) const {
	simmaps = inputmaps;
	simulate_seq(simmaps);
}

void circuit::simulate_seq(std::vector<id2boolmap>& simmaps) const {

	const idvec& gate_order = get_topsort_gates();

	for (uint clk = 0; clk < simmaps.size(); clk++) {

		add_const_to_map(simmaps[clk]);

		if (clk != 0) {
			foreach_latch(*this, dffid, dffout, dffin)
				simmaps[clk][dffout] = simmaps[clk - 1].at(dffin);
		}
		else {
			foreach_latch(*this, dffid, dffout, dffin)
				simmaps[0][dffout] = 0;
		}

		for (auto gid : gate_order) {
			if (gfunction(gid) != fnct::DFF)
				simmaps[clk][gfanout(gid)] = simulate_gate(gid, simmaps[clk]);
		}
	}
}

tern_t circuit::simulate_gate_tern(id gid, id2ternmap& valmap) const {
	const auto& g = getcgate(gid);
	return simulate_gate_tern(g, valmap);
}

tern_t circuit::simulate_gate_tern(const gate& g, id2ternmap& valmap) const {

	const auto fun = g.gfun();

	std::vector<tern_t> invals;
	for (auto wid : g.fanins) {
		try {
			invals.push_back(valmap.at(wid));
		} catch (std::out_of_range&) {
			neos_abort("value in simmap for " << wname(wid) << " missing");
		}
	}

	return simulate_gate_tern(invals, fun);
}

tern_t circuit::simulate_gate_tern(const vector<tern_t>& invals, fnct fun) {
	tern_t ret;
	switch (fun) {
	case fnct::AND: {
		tern_t val(invals[0]);
		for (uint i = 1; i < invals.size(); i++) {
			val = val & invals[i];
		}
		ret = val;
		break;
	}
	case fnct::NAND: {
		tern_t val(invals[0]);
		for (uint i = 1; i < invals.size(); i++) {
			val = val & invals[i];
		}
		ret = ~val;
		break;
	}
	case fnct::NOR: {
		tern_t val(invals[0]);
		for (uint i = 1; i < invals.size(); i++) {
			val = val | invals[i];
		}
		ret = ~val;
		break;
	}
	case fnct::OR: {
		tern_t val(invals[0]);
		for (uint i = 1; i < invals.size(); i++) {
			val = val | invals[i];
		}
		ret = val;
		break;
	}
	case fnct::XOR:
		ret = invals[0] ^ invals[1];
		break;
	case fnct::XNOR:
		ret = ~(invals[0] ^ invals[1]);
		break;
	case fnct::MUX:
		ret = tern_t::ite(invals[0], invals[1], invals[2]);
		break;
	case fnct::TBUF:
		ret = tern_t::tribuf(invals[0], invals[1]);
		break;
	case fnct::NOT:
		ret = ~invals[0];
		break;
	case fnct::BUF:
		ret = invals[0];
		break;
	default:
		neos_error("unsupported gate");
	}

	return ret;

}

void circuit::random_simulate(const circuit& cir, uint32_t num_patts) {
	assert(cir.nLatches() == 0);
	id2boolmap smap;
	for (uint32_t i = 0; i < num_patts; i++) {
		std::cout << "x=";
		for (auto xid : cir.allins()) {
			//std::cout << "  " << cir.wname(xid) << ":";
			bool b = smap[xid] = rand() % 2;
			//std::cout << _COLOR_RED(b);
			std::cout << b;
		}
		std::cout << "   y=";
		cir.simulate_comb(smap);
		for (auto yid : cir.outputs()) {
			std::cout << "  " << cir.wname(yid) << ":";
			std::cout << smap.at(yid);
		}
		std::cout << "\n";
	}
}

} // namespace ckt
