
#include <utl/ext_cmd.h>
#include "mc/unr.h"


namespace mc {

void Unroll::extend_to(uint f) {
	assert(f > nf);

	std::cout << "extending to " << f << "\n";
	for (uint i = nf; i < f; i++) {
		add_frame();
	}
}

void CircuitUnroll::set_tr(const circuit& tr, const oidset& frozens, bool zn) {
	if (tran.nWires() != 0)
		neos_abort("cannot reset transition relation");

	zero_init = zn;
	tran = tr;
	frozen_tr = frozens;
	frozen_wfanouts = utl::to_hset(frozen_tr);
	// update_frozen_fanout();
}

void CircuitUnroll::add_frame() {

	// std::cout << "nf is " << nf << "\n";

	assert(uncir.error_check());

	assert(c2umaps.size() == nf);
	c2umaps.push_back(id2idmap());

	assert(c2umaps[nf].empty());

	// add constants
	if (tran.has_const0())
		c2umaps[nf][tran.get_cconst0()] = uncir.get_const0();
	if (tran.has_const1())
		c2umaps[nf][tran.get_cconst1()] = uncir.get_const1();

	// take care of latch inputs and outputs
	foreach_wire(tran, tw, twid) {

		if (tran.isConst(twid))
			continue;

		if (tran.isLatch_output(twid)) {
			id dffin = tran.gfanin0(tran.wfanin0(twid));
			if (nf == 0) {
				id uid = zero_init ?
					uncir.get_const0():
					uncir.add_wire(_init_name(tran.wname(twid)), wtype::IN);

				c2umaps[nf][twid] = uid;
				init_wires.insert(uid);
			}
			else {
				c2umaps[nf][twid] = c2umaps[nf - 1].at(dffin);
			}
		}
		else if (_is_in(twid, frozen_wfanouts)) {
			if (nf == 0 || _is_not_in(twid, c2umaps[nf - 1]) ) {
				c2umaps[nf][twid] = uncir.add_wire(tran.wname(twid), tran.wrtype(twid));
			}
			else {
				c2umaps[nf][twid] = c2umaps[nf - 1].at(twid);
			}
		}
		else {
			c2umaps[nf][twid] = uncir.add_wire(_frame_name(tran.wname(twid), nf), tran.wrtype(twid));
		}

		if (tran.isOutput(twid)) {
			id uid = c2umaps[nf].at(twid);
			if ( !uncir.isOutput(uid) ) {
				id out = uncir.add_wire(_frame_name(tran.wname(twid), nf), wtype::OUT);
				uncir.add_gate({c2umaps[nf].at(twid)}, out, fnct::BUF);
				c2umaps[nf][twid] = out;
			}
		}

	}

	foreach_gate(tran, tg, tgid) {
		if (!tg.isLatch()) {
			if (!_is_in(tgid, frozen_gfanouts) || nf == 0) {
				uncir.add_gate(_frame_name(tran.gname(tgid), nf), tg, c2umaps[nf]);
			}
		}
	}


	/*
	foreach_latch(tran, dffid, dffout, dffin) {
		std::cout << "latch init : " << uncir.wname(c2umaps[0].at(dffout)) << "\n";
	}*/

	/*
	foreach_latch(tran, dffgid, dffout, dffin) {
		std::cout << "latch out : " << uncir.wname(c2umaps[nf].at(dffin)) << " " << c2umaps[nf].at(dffin) << "\n";
		//assert( c2umaps[nf].at(dffin) == uncir.find_wcheck(_frame_name(tran.wname(dffin), nf) ) );
	}
 	*/

	assert(uncir.error_check());
	nf++;
}

id CircuitUnroll::create_uncir_wire(id trwid) {

	if (_is_in(trwid, c2umaps[nf])) {
		id uid = c2umaps[nf].at(trwid);
		assert(uncir.wire_exists(uid));
		// std::cout << "wire :" << uncir.wname(c2umaps[nf].at(trwid)) << " -> " << tran.wname(trwid) << "\n";
		return uid;
	}

	id uid = -1;
	if (_is_in(trwid, frozen_wfanouts)) {
		if (nf == 0) {
			uid = c2umaps[nf][trwid] = uncir.add_wire(tran.wname(trwid), tran.wrtype(trwid));
		}
		else {
			uid = c2umaps[nf][trwid] = c2umaps[0].at(trwid);
		}
	}
	else {
		uid = c2umaps[nf][trwid] = uncir.add_wire(_frame_name(tran.wname(trwid), nf), tran.wrtype(trwid));
	}

	assert(uncir.wire_exists(uid));
	assert(uid != -1);
	return uid;
}

void AigUnroll::set_tr(const aig_t& tr, const oidset& frozens, bool zinit) {
	if (tran.nAnds() != 0)
		neos_abort("cannot reset transition relation");

	tran = tr;
	frozen_tr = frozens;
	zero_init = zinit;
}

void AigUnroll::add_frame() {

	// std::cout << "nf is " << nf << "\n";

	assert(c2umaps.size() == (uint)nf);
	c2umaps.push_back(id2alitmap());

	if (tran.has_const0())
		c2umaps[nf][tran.get_cconst0id()] = unaig.get_const0();

	// take care of inputs
	for (auto inid : tran.inputs()) {
		if (_is_in(inid, frozen_tr)) {
			if (nf == 0)
				c2umaps[nf][inid] = unaig.add_input(tran.ndname(inid));
			else {
				id fzid = c2umaps[nf - 1].at(inid).lid();
				// restoring frozen input that was swept
				if (!unaig.node_exists(fzid)) {
					id x = unaig.add_input(tran.ndname(inid));
					for (uint i = 0; i < nf; i++) {
						c2umaps[i][inid] = x;
					}
				}
				c2umaps[nf][inid] = c2umaps[0].at(inid);
			}
		}
		else {
			c2umaps[nf][inid] = unaig.add_input(_frame_name(tran.ndname(inid), nf));
		}
		if (tran.isKey(inid))
			unaig.add_to_keys(c2umaps[nf].at(inid).lid());
	}

	// take care of latches
	for (auto laid : tran.latches()) {

		if (nf == 0) {
			alit linit = zero_init ?
				unaig.get_const0():
				unaig.add_input(_init_name(tran.ndname(laid)));

			c2umaps[nf][laid] = linit;
			init_wires.insert(linit.lid());
		}
		else {
			id lprevid = c2umaps[nf - 1].at(tran.nfanin0(laid)).lid();
			if (!unaig.node_exists(lprevid))
				neos_abort( "can't locate previous state " << tran.ndname(laid) << " in frame " << nf);
			c2umaps[nf][laid] = tran.getNode(laid).cm0() ^
					c2umaps[nf - 1].at(tran.getNode(laid).fi0());
		}

	}

	// add AND gates
	new_ands.clear();
	for (auto nid : tran.top_order()) {

		auto& nd = tran.getNode(nid);

		if (nd.isAnd()) {
			bool strashed = false;

			if ( !unaig.node_exists( c2umaps[nf].at(nd.fi0()) ) )
				neos_abort("missing " << tran.ndname(nd.fi0()) << " " << c2umaps[nf].at(nd.fi0()).lid() );
			if ( !unaig.node_exists( c2umaps[nf].at(nd.fi1()) ) )
				neos_abort("missing " << tran.ndname(nd.fi1()) << " " << c2umaps[nf].at(nd.fi1()).lid() );

			c2umaps[nf][nid] = unaig.add_and(nd, c2umaps[nf], _frame_name(tran.ndname(nid), nf), strashed);
			if (!strashed) new_ands.insert(nid);
		}
		else if (nd.isOutput()) {
			alit oinlit = c2umaps[nf].at(nd.fi0());
			c2umaps[nf][nid] = unaig.add_output(_frame_name(tran.ndname(nid), nf),
					oinlit.lid(), oinlit.sgn() ^ nd.cm0());
		}

	}

	nf++;
}


void AigUnroll::sweep_unrolled_aig() {
	SATsweep::sweep_params_t p;
	sweep_unrolled_aig(p);
}

void AigUnroll::sweep_unrolled_aig(SATAigSweep& ssw) {

	idvec outs;
	for (auto lid : tran.latches()) {
		alit lin = tran.getcNode(lid).fanin0;
		alit ulin = c2umaps[nf-1].at(lin.lid());
		assert(unaig.node_exists(ulin));
		outs.push_back(
			unaig.add_output(tran.ndname(lid) + fmt("_$$%d", nf), ulin)
			);
	}

	ssw.sweep();
	assert(unaig.error_check());

	int i = 0;
	for (auto lid : tran.latches()) {
		auto nd = unaig.getcNode(outs[i]);
		const auto& ld = tran.getcNode(lid);
		c2umaps[nf-1][ld.fi0()] = nd.fanin0;
		assert(nd.fanin0 != outs[i]);
		unaig.remove_node(outs[i]);
		assert(unaig.node_exists(nd.fanin0));
		i++;
	}

	assert(unaig.error_check());
}

void AigUnroll::sweep_unrolled_aig(SATAigSweep::sweep_params_t& swp) {

	// set next state of last round as output to prevent its deletion during sweep
	SATAigSweep ssw(unaig, swp);
	AigUnroll::sweep_unrolled_aig(ssw);

}

aig_t AigUnroll::get_zeroinit_unaig() {

	if (zero_init)
		return unaig;
	else {
		aig_t retaig = unaig;
		id2boolHmap constmap;
		for (auto sid : init_wires) {
			constmap[sid] = 0;
		}
		retaig.propagate_constants(constmap);
		return retaig;
	}
}


void CircuitUnroll::sweep_unrolled_ckt() {
	SATsweep::sweep_params_t p;
	sweep_unrolled_ckt(p);
}

void CircuitUnroll::sweep_unrolled_ckt(SATCircuitSweep::sweep_params_t& swp) {

	// set next state of last round as output to prevent its deletion during sweep
	std::vector<wtype> pretypes;
	foreach_latch(tran, dffgid, dffout, dffin) {
		uncir.setwiretype(c2umaps[nf - 1].at(dffin), wtype::OUT);
	}
	uncir.clear_topsort();

	// sweep with given paramters
	opt_ns::circuit_satsweep(uncir, swp);

	// restore next-state varibales
	foreach_latch(tran, dffgid, dffout, dffin) {
		uncir.setwiretype(c2umaps[nf - 1].at(dffin), tran.wrtype(dffin));
	}
}

int32_t CircuitUnroll::sweep_unrolled_ckt_abc() {

	// set next state of last round as output to prevent its deletion during sweep
	vector<wtype> pretypes;
	vector<string> prevnames;
	foreach_latch(tran, dffgid, dffout, dffin) {
		id uid = c2umaps[nf - 1].at(dffin);
		if (uncir.isInter(uid)) {
			uncir.setwiretype(uid, wtype::OUT);
		}
		prevnames.push_back(uncir.wname(uid));
		// std::cout << "changing " << tran.wname(dffin) << " -> " << uncir.wname(c2umaps[nf - 1].at(dffin)) << "\n";
	}
	uncir.clear_topsort();
	//uncir.write_bench();
	// sweep with given paramters
	circuit old_uncir = uncir;

	assert(old_uncir.error_check());
	int32_t nsaved = ext::abc_simplify(uncir);

	frozen_gfanouts.clear();
	frozen_wfanouts = to_hset(frozen_tr);

	assert(uncir.nOutputs() == old_uncir.nOutputs());

	// restore next-state varibales
	id i = 0;
	foreach_latch(tran, dffgid, dffout, dffin) {
		id uid = uncir.find_wcheck(prevnames[i++]);
		if (uncir.isOutput(uid))
			uncir.setwiretype(uid, wtype::INTER);
		c2umaps[nf - 1][dffin] = uid;
		// std::cout << "changing " << tran.wname(dffin) << " -> "
		// << uncir.wname(c2umaps[nf - 1].at(dffin)) << "\n";
	}

	return nsaved;
/*
	foreach_latch(tran, dffgid, dffout, dffin) {
		auto unm = _frame_name(tran.wname(dffin), nf - 1);
		//std::cout << "looking for " << unm << "\n";

		// std::cout << "finding " << tran.wname(dffin) << " -> " << uncir.wname(c2umaps[nf - 1].at(dffin)) << "\n";
		id uid = uncir.find_wcheck(unm);
		assert(c2umaps[nf - 1].at(dffin) == uid);
		c2umaps[nf - 1][dffin] = uid;
		uncir.setwiretype(uid, tran.wrtype(dffin));
	}
*/

}

void CircuitUnroll::update_frozen_fanout() {

	frozen_gfanouts.clear();
	frozen_wfanouts = to_hset(frozen_tr);

	idset visited;
	idque Q;

	for (auto x : frozen_tr)
		Q.push(x);

	while (!Q.empty()) {
		id curwid = Q.back(); Q.pop();

		for (auto gid : tran.wfanout(curwid)) {
			bool g_is_frozen = true;
			const auto& g = tran.getcgate(gid);
			for (auto gin : g.fanins) {
				if ( _is_not_in(gin, frozen_wfanouts) ) {
					g_is_frozen = false;
					break;
				}
			}

			if (g_is_frozen) {
				frozen_wfanouts.insert(g.fo0());
				frozen_gfanouts.insert(g.nid);
				if ( _is_not_in(g.fo0(), visited) ) {
					Q.push(g.fo0());
					visited.insert(g.fo0());
				}
			}

		}
	}

	needs_frozen_update = false;

}


circuit CircuitUnroll::get_zeroinit_uncir() {

	if (zero_init) {
		return uncir;
	}
	else {
		circuit retcir = uncir;
		id2boolHmap constmap;
		for (auto sid : init_wires) {
			constmap[sid] = 0;
		}
		retcir.propagate_constants(constmap);
		//retcir.tie_to_constant(constmap);
		return retcir;
	}
}


void make_frozen_with_latch(circuit& cir, oidset fzs) {

	id isinit = cir.add_wire("_w_is_init", wtype::INTER);
	cir.add_gate("_g_init", {cir.get_const1()}, isinit, fnct::DFF);

	id i = 0;
	for (auto fid : fzs) {
		assert(cir.isInput(fid) || cir.isKey(fid));
		auto pr = cir.open_wire(fid, cir.wname(fid) + "_la");
		id lout = cir.add_wire(cir.wname(fid) + "_fb", wtype::INTER);
		cir.add_gate({pr.second}, lout, fnct::DFF);
		cir.add_gate({isinit, pr.first, lout}, pr.second, fnct::MUX);
	}

}

} // namespace mc
