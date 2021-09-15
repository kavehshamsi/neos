/*
 * circuit_words.cpp
 *
 *  Created on: Jan 13, 2021
 *      Author: kaveh
 */

#include "base/circuit_words.h"

namespace ckt {

cir_word reduce_op(const cir_word& x, fnct fn) {
	id out = x.cir->add_wire(wtype::INTER);
	assert(fn == fnct::XOR || fn == fnct::XNOR || fn == fnct::AND
			|| fn == fnct::NAND || fn == fnct::OR || fn == fnct::NOR);

	x.cir->add_gate(x.wordids, out, fn);

	return cir_word(*x.cir, idvec({out}));
}

cir_word apply_op(const vector<cir_word>& ins, fnct fn) {
	circuit* cir = ins[0].cir;
	size_t sz = ins[0].size();

	for (auto& x : ins) {
		assert(cir == x.cir);
		assert(sz == x.size());
	}

	cir_word retword(*cir);

	for (size_t i = 0; i < sz; i++) {
		idvec gateins;
		for (uint j = 0; j < ins.size(); j++) {
			gateins.push_back(ins[j][i]);
		}
		id gateout = cir->add_wire(wtype::INTER);
		cir->add_gate(gateins, gateout, fn);
		retword.wordids.push_back(gateout);
	}

	return retword;
}

cir_word operator!(const cir_word& a) {
	return apply_op({a}, fnct::NOT);
}

cir_word operator~(const cir_word& a) {
	return apply_op({a}, fnct::NOT);
}

cir_word operator&(const cir_word& a, const cir_word& b) {
	return apply_op({a, b}, fnct::AND);
}
cir_word operator^(const cir_word& a, const cir_word& b) {
	return apply_op({a, b}, fnct::XOR);
}

cir_word operator|(const cir_word& a, const cir_word& b) {
	return apply_op({a, b}, fnct::XOR);
}

cir_word create_word_wires(circuit& cir, size_t N, wtype wt = wtype::INTER) {
	cir_word ret(cir);
	for (size_t i = 0; i < N; i++) {
		ret.wordids.push_back(cir.add_wire(wt));
	}
	return ret;
}

cir_word operator+(const cir_word& a, const cir_word& b) {
	assert(a.cir == b.cir);
	assert(a.size() == b.size());

	idvec avec, bvec, svec;
	circuit vadd = ckt_block::vector_adder(a.size(), avec, bvec, svec);

	cir_word s = create_word_wires(*a.cir, a.size());

	id2idmap added2new;
	for (size_t i = 0; i < a.size(); i++) {
		added2new[avec[i]] = a[i];
		added2new[bvec[i]] = b[i];
		added2new[svec[i]] = s[i];
	}

	a.cir->add_circuit(vadd, added2new);

	return s;
}

cir_word operator==(const cir_word& a, const cir_word& b) {
	assert(a.cir == b. cir);
	assert(a.size() == b.size());
	cir_word xorouts = apply_op({a, b}, fnct::XNOR);
	return reduce_op(xorouts, fnct::AND);
}

cir_word operator!=(const cir_word& a, const cir_word& b) {

	assert(a.cir == b. cir);
	assert(a.size() == b.size());
	cir_word xorouts = apply_op({a, b}, fnct::XOR);
	return reduce_op(xorouts, fnct::OR);

}

cir_word operator>(const cir_word& a, const cir_word& b) {

	assert(a.cir == b.cir);
	assert(a.size() == b.size());

	int N = a.size();

	idvec avec, bvec;
	id agb;
	circuit vcomp = ckt_block::vector_half_comparator(N, avec, bvec, agb);

	id2idmap added2new;
	for (size_t i = 0; i < N; i++) {
		added2new[avec[i]] = a[N - i - 1];
		added2new[bvec[i]] = b[N - i - 1];
	}

	id out = a.cir->add_wire(wtype::INTER);
	added2new[agb] = out;

	a.cir->add_circuit(vcomp, added2new);

	return cir_word(*a.cir, idvec({out}));

}

cir_word operator<(const cir_word& a, const cir_word& b) {
	return operator>(b, a);
}

cir_word operator<=(const cir_word& a, const cir_word& b) {
	return operator!(operator>(a, b));
}

cir_word operator>=(const cir_word& a, const cir_word& b) {
	return operator!(operator<(a, b));
}

cir_word concat(const cir_word& a, const cir_word& b) {
	cir_word ret = a;
	utl::push_all(ret.wordids, b.wordids);
	return ret;
}

cir_word reduce_sum(const cir_word& a) {

	circuit badd = ckt_block::bit_counter(a.size());

	idvec ovec = utl::to_vec(badd.outputs());
	idvec invec = utl::to_vec(badd.inputs());

	id2idmap added2new;
	for (size_t i = 0; i < a.size(); i++) {
		added2new[invec[i]] = a[i];
	}
	cir_word outw(*a.cir, ovec.size(), wtype::INTER);

	for (size_t i = 0; i < ovec.size(); i++) {
		added2new[ovec[i]] = outw[i];
	}

	a.cir->add_circuit(badd, added2new);

	return outw;
}

cir_word operator+(cir_word& a) {
	return reduce_sum(a);
}

} // namespace words

