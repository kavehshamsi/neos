
#include <utl/utility.h>
#include "base/blocks.h"


//////////////// The circuit class ////////////////////
namespace ckt_block {

tree get_tree(uint width, fnct gatetype) {

	tree tree;

	string wname, gname;
	id in, O = -1;
	idque inque;

	for (uint i = 0; i < width; i++){
		wname = "treein" + std::to_string(i);
		in = tree.add_wire(wname, wtype::IN);
		inque.push(in);
		tree.in_vec.push_back(in);
	}

	while (inque.size() != 1){

		gate andgate;

		// join inputs
		andgate.fanins.push_back(utldeque(inque));
		andgate.fanins.push_back(utldeque(inque));

		// create output
		wname = "inter" + std::to_string(inque.size());
		O = tree.add_wire(wname, wtype::INTER);
		andgate.setfanout(O);

		// add gate
		gname = "andgate" + std::to_string(inque.size());
		tree.add_gate(gname, andgate.fi(), andgate.fo0(), gatetype);

		// push output into queue
		inque.push(O);
	}

	tree.setwiretype(O, wtype::OUT);
	tree.out = O;

	return tree;
}

tree and_tree(uint width){
	return get_tree(width, fnct::AND);
}

tree eq_comparator(int width, idvec& ains, idvec& bins, id& out) {

	tree retcir;
	idvec xouts;
	for (int i = 0; i < width; i++) {
		id a = retcir.add_wire("a" + std::to_string(i), wtype::IN);
		id b = retcir.add_wire("b" + std::to_string(i), wtype::IN);
		ains.push_back(a);
		bins.push_back(b);
		id xo = retcir.add_wire("xo" + std::to_string(i), wtype::INTER);
		retcir.add_gate({a, b}, xo, fnct::XNOR);
		xouts.push_back(xo);
	}
	out = retcir.add_wire("out", wtype::OUT);
	retcir.add_gate(xouts, out, fnct::AND);

	return retcir;

}

tree key_comparator(uint width, bool apply_random_mapping) {

	tree tree = and_tree(width);
	tree.key_vec.clear();
	gate tmpgate;
	idvec newin_vec;

	for (uint i = 0; i < width; i++) {
		gate inXor;
		string wname, gname;
		id A, B, O;
		wname = "in" + std::to_string(i);
		A = tree.add_wire(wname, wtype::IN);
		newin_vec.push_back(A);
		wname = "keyinput" + std::to_string(i);
		B = tree.add_wire(wname, wtype::KEY);
		tree.key_vec.push_back(B);
		O = tree.in_vec[i];
		wname = "in" + std::to_string(i) + "xor";
		tree.setwirename(O, wname);
		tree.setwiretype(O, wtype::INTER);

		inXor.fanins.push_back(A);
		inXor.fanins.push_back(B);
		inXor.fanouts = {O};

		gname = "xor_" + std::to_string(i);
		fnct function = fnct::XNOR;
		if (apply_random_mapping)
			function = (rand() % 2 == 1) ? fnct::XOR : fnct::XNOR ;
		tree.add_gate(gname, inXor.fanins, inXor.fanouts[0], function );
	}

	tree.in_vec = newin_vec;
	tree.out = *(tree.outputs().begin());
	tree.setwirename(tree.out, "compout");

	//tree.write_bench();

	return tree;

}


tree fixed_comparator(id width, const boolvec& key){

	if ((uint)width != key.size()) {
		std::cout << "size mismatch between key and width in fixed comparator\n";
		std::cout << width << " vs " << key.size() << "\n";
	}

	tree comptree;
	for (uint i = 0; i < width; i++) {
		comptree.in_vec.push_back(comptree.add_wire("in" + std::to_string(i), wtype::IN));
	}

	id2idmap added2new;
	tree andtree = and_tree(width);
	for (uint i = 0; i < width; i++) {
		id treein;
		if (!key[i]) {
			treein = comptree.add_wire("in" + std::to_string(i) + "bar", wtype::INTER);
			comptree.add_gate({comptree.in_vec[i]}, treein, fnct::NOT);
		}
		else {
			treein = comptree.in_vec[i];
		}
		added2new[andtree.in_vec[i]] = treein;
	}

	comptree.out = added2new[andtree.out] = comptree.add_wire("compout", wtype::OUT);
	comptree.add_circuit(andtree, added2new);

	return comptree;

}

tree anti_sat(id width) {

	tree comp1 = key_comparator(width);
	tree comp2 = key_comparator(width);

	id2idmap added2new;

	for (auto& x : comp1.in_vec){
		added2new[x] = x;
	}

	comp1.add_circuit(comp2, added2new, "b");

	gate endand;
	gate inv;
	id g, gp, gpbar, out;

	// the two outputs of the andtrees one of which has to be complemented
	g = comp1.out;
	gp = added2new.at(comp2.out);

	comp1.setwirename(g, "antisat_g");

	// add inverter to the end of one of the andtrees
	inv.fanins.push_back(gp);
	gpbar = comp1.add_wire("antisat_gbar", wtype::OUT);
	inv.fanouts = {gpbar};
	comp1.add_gate("anti_invg", inv.fanins, inv.fanouts[0], fnct::NOT);

	// create out and AND gate
	string wname;
	endand.fanins.push_back(g);
	endand.fanins.push_back(gpbar);
	out = comp1.add_wire("out", wtype::OUT);
	endand.fanouts = {out};
	comp1.add_gate("antiend", endand.fanins, endand.fanouts[0], fnct::AND);

	// set correct wiretypes
	comp1.setwiretype(g, wtype::INTER);
	comp1.setwiretype(gp, wtype::INTER);
	comp1.setwiretype(gpbar, wtype::INTER);

	// reparse and reconstruct child attributes
	comp1.reparse();
	comp1.in_vec.clear();
	comp1.key_vec.clear();

	for (auto x : comp1.inputs()) {
		comp1.in_vec.push_back(x);
	}
	for (auto x : comp1.keys()) {
		comp1.key_vec.push_back(x);
	}
	comp1.out = *comp1.outputs().begin();

	// comp1.write_bench();

	return comp1;
}

swbox n_switchbox(uint N){

	swbox nswb;

	id2idmap added2new;

	mux_t nmux = ckt_block::nmux(N);

	for (uint i = 0; i < N ; i++){
		string wname = string("swbin") + std::to_string(i);
		id nsin = nswb.add_wire(wname, wtype::IN);
		nswb.in_vec.push_back(nsin);
		added2new[nmux.in_vec[i]] = nsin;
	}

	for (uint i = 0; i < N; i++) {
		string wname = string("swbout") + std::to_string(i);
		id nswbout = nswb.add_wire(wname, wtype::OUT);
		nswb.out_vec.push_back(nswbout);
		added2new[nmux.out] = nswbout;

		//add to sel_vec nswb
		idvec tmp_sel_vec = nmux.sel_vec;
		for (auto& x : tmp_sel_vec){
			x += nswb.nWires();
		}
		nswb.sel_vec.insert(nswb.sel_vec.end(),
				tmp_sel_vec.begin(), tmp_sel_vec.end());

		//add mux
		nswb.add_circuit(nmux, added2new, std::to_string(i));
	}
	//nswb.recount_wires();
	//build_wire_InOutSets(nswb);

	return nswb;
}

circuit half_adder() {
	id a, b, s, co;
	return half_adder(a, b, s, co);
}

circuit half_adder(id& a, id& b, id& s, id& co) {
	circuit retcir;
	a = retcir.add_wire("a", wtype::IN);
	b = retcir.add_wire("b", wtype::IN);

	s = retcir.add_wire("s", wtype::OUT);
	co = retcir.add_wire("co", wtype::OUT);

	retcir.add_gate({a, b}, co, fnct::AND);
	retcir.add_gate({a, b}, s, fnct::XOR);

	// retcir.write_bench();

	return retcir;
}

circuit full_adder() {
	id a, b, ci, s, co;
	return full_adder(a, b, ci, s, co);
}

circuit full_adder(id& a, id& b, id& ci, id& s, id& co) {
	circuit retcir;
	a = retcir.add_wire("a", wtype::IN);
	b = retcir.add_wire("b", wtype::IN);
	ci = retcir.add_wire("ci", wtype::IN);

	s = retcir.add_wire("s", wtype::OUT);
	co = retcir.add_wire("co", wtype::OUT);

	id x0 = retcir.add_wire("x0", wtype::INTER);
	id x1 = retcir.add_wire("x1", wtype::INTER);
	id x2 = retcir.add_wire("x3", wtype::INTER);

	retcir.add_gate({a, b}, x0, fnct::XOR);
	retcir.add_gate({x0, ci}, s, fnct::XOR);
	retcir.add_gate({a, b}, x1, fnct::AND);
	retcir.add_gate({x0, ci}, x2, fnct::AND);
	retcir.add_gate({x1, x2}, co, fnct::OR);

	// retcir.write_bench();

	return retcir;
}


circuit vector_adder(uint N) {
	idvec avec, bvec, svec;
	return vector_adder(N, avec, bvec, svec);
}

circuit vector_adder(uint N, idvec& avec, idvec& bvec, idvec& svec) {

	circuit retcir;

	circuit ha_cir = half_adder();
	id ha = ha_cir.find_wcheck("a");
	id hb = ha_cir.find_wcheck("b");
	id hs = ha_cir.find_wcheck("s");
	id hco = ha_cir.find_wcheck("co");

	circuit fa_cir = full_adder();
	id a = fa_cir.find_wcheck("a");
	id b = fa_cir.find_wcheck("b");
	id ci = fa_cir.find_wcheck("ci");
	id s = fa_cir.find_wcheck("s");
	id co = fa_cir.find_wcheck("co");

	idvec covec, civec;
	for (uint i = 0; i < N; i++) {
		avec.push_back( retcir.add_wire("a" + std::to_string(i), wtype::IN) );
	}
	for (uint i = 0; i < N; i++) {
		bvec.push_back( retcir.add_wire("b" + std::to_string(i), wtype::IN) );
	}
	for (uint i = 0; i < N; i++) {
		svec.push_back( retcir.add_wire("s" + std::to_string(i), wtype::OUT) );
	}
	for (uint i = 0; i < N; i++) {
		covec.push_back( retcir.add_wire("co" + std::to_string(i), wtype::INTER) );
	}

	for (uint i = 0; i < N; i++) {
		id2idmap added2new;
		if (i == 0) {
			added2new[ha] = avec[i];
			added2new[hb] = bvec[i];
			added2new[hs] = svec[i];
			added2new[hco] = covec[i];
			retcir.add_circuit(ha_cir, added2new);
		}
		else {
			added2new[a] = avec[i];
			added2new[b] = bvec[i];
			added2new[s] = svec[i];
			added2new[co] = covec[i];
			added2new[ci] = covec[i-1];
			retcir.add_circuit(fa_cir, added2new);
		}
	}

	retcir.remove_dead_logic();

	retcir.write_bench();

	circuit::random_simulate(retcir, 5);

	return retcir;
}

circuit bit_counter(uint N) {
	idvec ins, outs;
	return bit_counter(N, ins, outs);
}

circuit bit_counter(uint N, idvec& ins, idvec& outs) {

	circuit retcir;

	if (N == 1) {
		id outid = retcir.add_wire("out0", wtype::OUT);
		id inid = retcir.add_wire("in0", wtype::IN);
		ins = {inid};
		outs = {outid};
		retcir.add_gate({inid}, outid, fnct::BUF);
		return retcir;
	}

	vector<vector<uint>> mat(1, vector<uint>(1, N));

	circuit fa_cir = full_adder();
	circuit ha_cir = half_adder();
	idvec fa_ins = utl::to_vec(fa_cir.inputs());
	idvec fa_outs = utl::to_vec(fa_cir.outputs());
	idvec ha_ins = utl::to_vec(ha_cir.inputs());
	idvec ha_outs = utl::to_vec(ha_cir.outputs());

/*
	for (auto x : fa_ins)
		std::cout << "in " << fa_cir.wname(x) << "\n";
	for (auto x : fa_outs)
		std::cout << "in " << fa_cir.wname(x) << "\n";
*/

	{
		uint lr = 0;
		idvec outs;
		while (lr < N) {

			if (mat[lr].back() > 2)
				mat.push_back(vector<uint>(mat[lr].size() + 1, 0));
			else
				mat.push_back(vector<uint>(mat[lr].size(), 0));

			uint col_max = mat[lr].size();

			for (uint col = 0; col < col_max; col++) {
				mat[lr+1][col] += mat[lr][col]/3 + (mat[lr][col] % 3);
				mat[lr+1][col+1] += mat[lr][col]/3;
			}

			uint num_one = 0, num_two = 0;
			for (auto x : mat[lr+1]) {
				if (x == 1) num_one++;
				if (x == 2) num_two++;
			}
			if ((num_one + num_two == mat[lr+1].size())
					&& (num_one >= num_two) ) {
				break;
			}

			lr++;
		}
	}


/*
	for (uint lr = 0; lr < mat.size(); lr++) {
		std::cout << "lr" << lr << ": ";
		for (uint col = 0; col < mat[lr].size(); col++) {
			std::cout << mat[lr][col] << ", ";
		}
		std::cout << "\n";
	}
*/

	//std::cout << "num outputs is: " << num_outs << "\n";

	idque sQ, cQ;

	for (int i = 0; i < N; i++) {
		id xid = retcir.add_wire(fmt("in%d", i), wtype::IN);
		sQ.push(xid);
	}

	uint out_ind = 0;
	id net_ind = 0;
	id num_fa = 0;

	while (true) {

		assert(sQ.size() >= 2);

		bool use_ha = false;

		id2idmap added2new;
		added2new[ fa_ins[0] ] = sQ.front(); sQ.pop();
		added2new[ fa_ins[1] ] = sQ.front(); sQ.pop();
		if (sQ.empty()) {
			use_ha = true;
		}
		else {
			added2new[ fa_ins[2] ] = sQ.front();
			sQ.pop();
		}

		id swid = added2new[use_ha ? ha_outs[0]:fa_outs[0]] =
				retcir.add_wire(fmt("ns%d_%d", net_ind++ % out_ind), wtype::INTER);
		id cwid = added2new[use_ha ? ha_outs[1]:fa_outs[1]] =
				retcir.add_wire(fmt("nc%d_%d", net_ind++ % out_ind), wtype::INTER);

		retcir.add_circuit(use_ha ? ha_cir:fa_cir, added2new);
		num_fa++;

		sQ.push(swid);
		cQ.push(cwid);

		if (sQ.size() == 1) {
			retcir.setwiretype(swid, wtype::OUT);
			retcir.setwirename(swid, fmt("out%d", out_ind++));
			outs.push_back(swid);
			if (cQ.size() == 1) {
				retcir.setwiretype(cwid, wtype::OUT);
				retcir.setwirename(cwid, fmt("out%d", out_ind++));
				outs.push_back(cwid);
				break;
			}
			sQ = cQ;
			cQ = idque();
			net_ind = 0;
		}
	}

	std::cout << (N & (N-1)) << "\n";
	std::cout << std::log2(N) << "\n";
	uint num_outs = ((N & (N-1)) == 0) ? std::log2(N) : (uint)std::log2(N) + 1;
	std::cout << "num outs " << num_outs << "\n";
	outs.resize(num_outs);
	retcir.trim_circuit_to(utl::to_hset(outs));

	//retcir.write_bench();
	assert(retcir.error_check());

/*
	id2boolmap simmap;
	for (uint i = 0; i < 5; i++) {
		std::cout << "x=";
		foreach_inid(retcir, xid) {
			bool b = simmap[xid] = rand() % 2;
			std::cout << b;
		}
		retcir.simulate_comb(simmap);
		std::cout << " -> y=";
		foreach_outid(retcir, yid) {
			std::cout << simmap.at(yid);
		}
		std::cout << "\n";
	}
*/

	// retcir.print_stats();
	// std::cout << "num fa: " << num_fa << "\n";
	//retcir.write_bench();

	return retcir;
}


void add_binary_sorter(circuit& cir, idvec& xs, uint i, uint j) {

	id yi, yj;

	//std::cout << "bit sorter " << i << " " << j << "\n";

	yi = cir.add_wire(wtype::INTER);
	yj = cir.add_wire(wtype::INTER);

	cir.add_gate({xs.at(i), xs.at(j)}, yi, fnct::OR);
	cir.add_gate({xs.at(i), xs.at(j)}, yj, fnct::AND);
	xs.at(i) = yi;
	xs.at(j) = yj;

}

void odd_even_merge(circuit& cir, idvec& xs, int lo, int n, int r) {

	//std::cout << "\nodd_even merge " << lo << "  " << n << "  " << r << "\n";

	int m = r*2;

	if (m < n) {
		odd_even_merge(cir, xs, lo, n, m);
		odd_even_merge(cir, xs, lo+r, n, m);
		for (int i = lo + r;  i + r < lo + n && i + r < xs.size(); i += m) {
			add_binary_sorter(cir, xs, i, i + r);
		}
	}
	else {
		if (lo + r < xs.size())
			add_binary_sorter(cir, xs, lo, lo + r);
	}

	//std::cout << "done with merge " << lo << "  " << n << "  " << r << "\n\n";

}

void odd_even_mergesort(circuit& cir, idvec& xs, int lo, int n) {

	//std::cout << "\nmerge sort " << lo << " " << n << "\n";

	if (n > 1) {
		int m = 2;
		while (m < n) m <<= 1;
		m >>= 1;
		//std::cout << "m is " << m << "\n";

		odd_even_mergesort(cir, xs, lo, m);
		odd_even_mergesort(cir, xs, lo + m, n - m);

		odd_even_merge(cir, xs, lo, n, 1);
	}

	//std::cout << "done with merge sort " << lo << " " << n << "\n\n";

}


circuit bit_odd_even_sorter(uint N) {
	circuit retcir;
	idvec xs;
	idvec ys;
	for (uint i = 0; i < N; i++) {
		xs.push_back(retcir.add_wire(fmt("x%d", i), wtype::IN));
	}

	idvec lvls(N, 0);

	odd_even_mergesort(retcir, xs, 0, N);

	for (uint i = 0; i < N; i++) {
		retcir.setwiretype(xs[i], wtype::OUT);
		retcir.setwirename(xs[i], fmt("y%d", i));
		retcir.setnodeid(xs[i], retcir.get_max_gid() + 1);
	}

	//retcir.write_bench();
	//circuit::random_simulate(retcir, 20);

	//retcir.print_stats();

	return retcir;
}

circuit half_bit_comparator(id& a, id& b, id& agb) {

	circuit retcir;
	a = retcir.add_wire("a", wtype::IN);
	b = retcir.add_wire("b", wtype::IN);

	agb = retcir.add_wire("agb", wtype::OUT);

	id bbar = retcir.add_wire("bbar", wtype::INTER);

	retcir.add_gate({b}, bbar, fnct::NOT);
	retcir.add_gate({bbar, a}, agb, fnct::AND);

	// retcir.write_bench();

	return retcir;
}

circuit half_bit_comparator() {
	id a, b, agb;
	return half_bit_comparator(a, b, agb);
}

circuit full_bit_comparator() {
	id a, b, agb, bga, aeb;
	return full_bit_comparator(a, b, agb, bga, aeb);
}

circuit full_bit_comparator(id& a, id& b, id& agb, id& bga, id& aeb) {

	circuit retcir;
	a = retcir.add_wire("a", wtype::IN);
	b = retcir.add_wire("b", wtype::IN);

	agb = retcir.add_wire("agb", wtype::OUT);
	bga = retcir.add_wire("agb", wtype::OUT);
	aeb = retcir.add_wire("aeb", wtype::OUT);

	id bbar = retcir.add_wire("bbar", wtype::INTER);
	id abar = retcir.add_wire("abar", wtype::INTER);

	retcir.add_gate({b}, bbar, fnct::NOT);
	retcir.add_gate({a}, abar, fnct::NOT);

	retcir.add_gate({bbar, a}, agb, fnct::AND);
	retcir.add_gate({b, abar}, bga, fnct::AND);
	retcir.add_gate({b, a}, aeb, fnct::XNOR);

	//retcir.write_bench();

	return retcir;
}

circuit vector_half_comparator(uint N) {
	idvec avec, bvec;
	id avgbv;
	return vector_half_comparator(N, avec, bvec, avgbv);
}

circuit vector_half_comparator(uint N, idvec& avec, idvec& bvec, id& avgbv) {

	circuit retcir;
	circuit hac = half_bit_comparator();
	idvec hacins = utl::to_vec(hac.inputs());
	id hacout = *(hac.outputs().begin());

	for (uint i = 0; i < N; i++) {
		id ai = retcir.add_wire("a" + std::to_string(i), wtype::IN);
		avec.push_back(ai);
	}
	for (uint i = 0; i < N; i++) {
		id bi = retcir.add_wire("b" + std::to_string(i), wtype::IN);
		bvec.push_back(bi);
	}

	avgbv = retcir.add_wire("avgbv_out", wtype::OUT);

	id eiacc, ei, gi;
	idvec orins, eis, gis;

	for (uint i = 0; i < N; i++) {
		id2idmap added2new;
		// add bit comparator
		added2new[hacins[0]] = avec[i];
		added2new[hacins[1]] = bvec[i];
		retcir.add_circuit(hac, added2new);

		// collect gi
		id gi = added2new.at(hacout);
		retcir.setwirename(gi, fmt("g%1%", i));
		retcir.setwiretype(gi, wtype::INTER);
		gis.push_back(gi);

		// build gibar
		ei = retcir.add_wire(fmt("e%1%", i), wtype::INTER);
		retcir.add_gate({avec[i], bvec[i]}, ei, fnct::XNOR);
		eis.push_back(ei);

		// collect orin
		id orin = retcir.add_wire(fmt("ori%1%", i), wtype::INTER);
		orins.push_back(orin);

		// 3 different cases for gibar
		if (i == 0) {
			retcir.add_gate({gi}, orin, fnct::BUF);
		}
		else if (i == 1) {
			eiacc = eis[0];
			retcir.add_gate({gi, eiacc}, orin, fnct::AND);
		}
		else {
			id neiacc = retcir.add_wire(fmt("xacc%1%", i), wtype::INTER);
			retcir.add_gate({eiacc, eis[i-1]}, neiacc, fnct::AND);
			eiacc = neiacc;
			retcir.add_gate({gi, eiacc}, orin, fnct::AND);
		}
	}

	retcir.add_gate(orins, avgbv, fnct::OR);

	// retcir.write_bench();
	//retcir.error_check();
	//circuit::random_simulate(retcir, 10);

	return retcir;
}

circuit sorted_vector_half_comparator(uint N) {
	idvec avec, bvec;
	id avgbv;
	return sorted_vector_half_comparator(N, avec, bvec, avgbv);
}

circuit sorted_vector_half_comparator(uint N, idvec& avec, idvec& bvec, id& avgbv) {
	circuit retcir;
	circuit hac = half_bit_comparator();
	idvec hacins = utl::to_vec(hac.inputs());
	id hacout = *(hac.outputs().begin());

	for (uint i = 0; i < N; i++) {
		id ai = retcir.add_wire("a" + std::to_string(i), wtype::IN);
		avec.push_back(ai);
	}
	for (uint i = 0; i < N; i++) {
		id bi = retcir.add_wire("b" + std::to_string(i), wtype::IN);
		bvec.push_back(bi);
	}

	avgbv = retcir.add_wire("avgbv_out", wtype::OUT);

	id eiacc, ei, gi;
	idvec orins, eis, gis;

	for (uint i = 0; i < N; i++) {
		id2idmap added2new;
		// add bit comparator
		added2new[hacins[0]] = avec[i];
		added2new[hacins[1]] = bvec[i];
		retcir.add_circuit(hac, added2new);

		// collect gi
		id gi = added2new.at(hacout);
		retcir.setwirename(gi, fmt("g%1%", i));
		retcir.setwiretype(gi, wtype::INTER);
		gis.push_back(gi);
	}

	retcir.add_gate(gis, avgbv, fnct::OR);

	return retcir;
}


/*
 * get mux element with tri-state buffers.
 * is a subset selector circuit and is used in crossbars.
 */
mux_t key_tmux(int num_ins, bool iskey_controlled) {

	assert (num_ins >= 0);

	mux_t tmux;

	for (int i = 0; i < num_ins; i++) {

		// add input wires
		string wname = "in" + std::to_string(i);
		tmux.in_vec.push_back(tmux.add_wire(wname, wtype::IN));

		// add select wires
		wname = (iskey_controlled ? "keyinput":"sel") + std::to_string(i);
		tmux.sel_vec.push_back(tmux.add_wire(wname,
				(iskey_controlled ? wtype::KEY : wtype::IN)));
	}

	// create output
	string wname = "nmuxout";
	tmux.out = tmux.add_wire(wname, wtype::OUT);

	// add tri-state buffers
	gate tbuf;
	tbuf.setgfun(fnct::TBUF);
	tbuf.fanouts = {tmux.out};

	for (int i = 0; i < num_ins; i++) {
		tbuf.fanins.clear();
		tbuf.fanins.push_back(tmux.sel_vec[i]);
		tbuf.fanins.push_back(tmux.in_vec[i]);

		string gname = "trigate" + std::to_string(i);
		tmux.add_gate(gname, tbuf);
	}

	return tmux;
}

mux_t key_nmux_w2mux(int num_ins, bool iskey_controlled) {

		mux_t nmux;

		gate largeOr;

		int num_select = ceil(log2(num_ins));

		for (int i = 0; i < num_select; i++){
			string wname = (iskey_controlled ? "keyinput":"sel") + std::to_string(i);
			nmux.sel_vec.push_back(nmux.add_wire(wname,
					(iskey_controlled ? wtype::KEY : wtype::IN)));
		}

		idvec cur_layer, next_layer;
		for (int i = 0; i < num_ins; i++) {
			string wname = "in" + std::to_string(i);
			id in = nmux.add_wire(wname, wtype::IN);
			cur_layer.push_back(in);
			nmux.in_vec.push_back(in);
		}

		id layer_count = 0, inter_count = 0, g_count = 0;
		while (cur_layer.size() > 1) {
			gate mux2;
			mux2.fanins.push_back(nmux.sel_vec[layer_count]);

			mux2.fanins.push_back(cur_layer.back());
			cur_layer.pop_back();
			mux2.fanins.push_back(cur_layer.back());
			cur_layer.pop_back();

			string wname = "inter" + std::to_string(inter_count++);

			mux2.fanouts = {nmux.add_wire(wname, wtype::INTER)};

			next_layer.push_back(mux2.fanouts[0]);

			string gname = "mux2gate" + std::to_string(g_count++);
			nmux.add_gate(gname, mux2.fanins, mux2.fanouts[0], fnct::MUX);

			if ((cur_layer.size() == 0) && (next_layer.size() == 1)) {
				string wname = "nmuxout";
				nmux.setwiretype(next_layer[0], wtype::OUT);
				nmux.setwirename(next_layer[0], wname);
				nmux.out = next_layer[0];
				break;
			}

			if (cur_layer.size() <= 1) {
				for (auto& x : next_layer) {
					cur_layer.push_back(x);
				}
				layer_count++;
				next_layer.clear();
			}
		}

		build_wire_InOutSets(nmux);

		return nmux;

}

mux_t nmux(int num_ins, int num_select, bool iskey_controlled){

	mux_t nmux;

	gate largeOr;

	if (num_select == -1)
		num_select = ceil(log2(num_ins));

	for (int i = 0; i < num_select; i++) {
		gate inputNot;
		string wname, gname;
		id in, inb;
		if (iskey_controlled) {
			in = nmux.add_key();
			inb = nmux.add_wire("sel" + std::to_string(i) + "_not", wtype::INTER);
		}
		else {
			in = nmux.add_wire("sel" + std::to_string(i), wtype::IN);
			inb = nmux.add_wire("sel" + std::to_string(i) + "_not", wtype::INTER);
		}

		nmux.sel_vec.push_back(in);
		gname = "not_" + std::to_string(i);
		nmux.add_gate(gname, {in}, inb, fnct::NOT);
	}

	uint mask = 0;
	mask = pow(2,num_select+1)-1;

	for (int i = 0; i < num_ins; i++) {

		gate selAnd;
		string wname, gname;
		id kin, T;

		wname = "in" + std::to_string(i);
		kin = nmux.add_wire(wname, wtype::IN);
		nmux.in_vec.push_back(kin);
		selAnd.fanins.push_back(kin);

		for (int j = 0; j < num_select; j++){
			if ((mask >> j) & 0x01)
				selAnd.fanins.push_back(j*2);
			else
				selAnd.fanins.push_back(j*2+1);
		}

		wname = "T" + std::to_string(i);
		T = nmux.add_wire(wname, wtype::INTER);
		gname = "Tgate_" + std::to_string(i);
		largeOr.fanins.push_back(T);
		nmux.add_gate(gname, selAnd.fanins, T, fnct::AND);
		mask--;
	}

	largeOr.fanouts = {nmux.add_wire("out", wtype::OUT)};
	nmux.out = largeOr.fanouts[0];
	nmux.add_gate("endOR", largeOr.fanins, largeOr.fanouts[0], fnct::OR);

	return nmux;

}


tree lut(uint numinputs){

	tree lut;

	gate largeOr;

	uint numkeys = pow(2, numinputs);

	idvec inbar_vec;
	for (uint i = 0; i < numinputs; i++) {
		gate inputNot;
		string wname, gname;
		id x, xb;
		wname = "in" + std::to_string(i);
		x = lut.add_wire(wname, wtype::IN);
		lut.in_vec.push_back(x);
		wname = "in" + std::to_string(i) + "not";
		xb = lut.add_wire(wname, wtype::INTER);
		inbar_vec.push_back(xb);
		inputNot.fanins.push_back(x);
		inputNot.fanouts = {xb};
		gname = "gnot_" + std::to_string(i);
		lut.add_gate(gname, inputNot.fanins, inputNot.fanouts[0], fnct::NOT );
	}

	uint mask = 0;

	for (uint i = 0; i < numkeys; i++){

		gate keyAnd;
		string wname, gname;
		id kin, T;

		wname = "keyinput" + std::to_string(i);
		kin = lut.add_wire(wname, wtype::KEY);
		lut.key_vec.push_back(kin);
		keyAnd.fanins.push_back(kin);

		for (uint j = 0; j < numinputs; j++) {
			keyAnd.fanins.push_back( ((i >> j) & 1) ? lut.in_vec[j]:inbar_vec[j] );
		}

		wname = "T" + std::to_string(i);
		T = lut.add_wire(wname, wtype::INTER);
		gname = "Tgate_" + std::to_string(i);
		largeOr.fanins.push_back(T);
		lut.add_gate(gname, keyAnd.fanins, T, fnct::AND);
		mask--;
	}
	largeOr.fanouts = {lut.add_wire("out", wtype::OUT)};
	lut.add_gate("endOR", largeOr.fanins, largeOr.fanouts[0], fnct::OR);
	lut.out = largeOr.fanouts[0];

	return lut;
}

tree ra_lut(int num_rows, int num_columns, int num_on_rows) {

	tree retcir;

	for (int i = 0; i < num_columns; i++) {
		retcir.in_vec.push_back(retcir.add_wire("in" + std::to_string(i), wtype::IN));
	}

	vector<idvec> keys(num_rows);
	vector<idvec> rows(num_rows);
	idvec andouts;
	for (int i = 0; i < num_rows; i++) {
		idvec andins;
		for (int j = 0; j < num_columns; j++) {
			rows[i].push_back( retcir.add_wire( fmt("r%1%_%2%", j % j), wtype::INTER)  );
			id kid = retcir.add_key();
			retcir.key_vec.push_back(kid);
			retcir.add_gate( {retcir.in_vec[j], kid}, rows[i][j], fnct::XOR );
			andins.push_back( rows[i][j] );
		}
		id ao = retcir.add_wire("ao" + std::to_string(i), wtype::INTER);
		andouts.push_back(ao);
		retcir.add_gate( andins, ao, fnct::AND );
	}

	idvec orouts;
	for (int i = 0; i < num_rows; i++) {
		id kid = retcir.add_key();
		retcir.key_vec.push_back(kid);
		orouts.push_back( retcir.add_wire("oo" + std::to_string(i), wtype::INTER) );
		retcir.add_gate( {andouts[i], kid}, orouts[i], fnct::AND );
	}

	retcir.out = retcir.add_wire("out", wtype::OUT);
	retcir.add_gate(orouts, retcir.out, fnct::OR);

	return retcir;

}

tree rca_lut(id& out, vector<idvec>& row_keys, vector<idvec>& cola_keys,
		int num_rows, int num_columns, int num_on_rows, int num_on_columns) {

	tree retcir;

	for (int i = 0; i < num_columns; i++) {
		retcir.in_vec.push_back(retcir.add_wire("in" + std::to_string(i), wtype::IN));
	}

	row_keys = vector<idvec>(num_rows);
	cola_keys = vector<idvec>(num_rows);
	vector<idvec> rows(num_rows);
	idvec andouts;
	for (int i = 0; i < num_rows; i++) {
		idvec andins;
		for (int j = 0; j < num_columns; j++) {
			id rid = retcir.add_wire( fmt("r%1%_%2%", i % j), wtype::INTER);
			rows[i].push_back( rid );
			id kid = retcir.add_key();
			retcir.key_vec.push_back(kid);
			row_keys[i].push_back(kid);
			retcir.add_gate( {retcir.in_vec[j], kid}, rows[i][j], fnct::XNOR );
			kid = retcir.add_key();
			retcir.key_vec.push_back(kid);
			cola_keys[i].push_back(kid);
			id rout = retcir.add_wire( fmt("ra%1%_%2%", i % j), wtype::INTER ) ;
			retcir.add_gate( {rows[i][j], kid}, rout, fnct::AND );
			andins.push_back( rout );
		}
		id ao = retcir.add_wire("ao" + std::to_string(i), wtype::INTER);
		andouts.push_back(ao);
		retcir.add_gate( andins, ao, fnct::AND );
	}

    if (num_rows != 1) {
    	out = retcir.out = retcir.add_wire("out", wtype::OUT);
    	retcir.add_gate(andouts, retcir.out, fnct::OR);
    }
    else {
        out = retcir.out = andouts.at(0);
        retcir.setwirename(out, "out");
        retcir.setwiretype(out, wtype::OUT);
    }

	retcir.write_bench();

	return retcir;

}

tree rca_lut(int num_rows, int num_columns, int num_on_rows, int num_on_columns) {
	vector<idvec> row_keys;
	vector<idvec> cola_keys;
	id out;
	return rca_lut(out, row_keys, cola_keys, num_rows, num_columns, num_on_rows, num_on_columns);
}

circuit encXOR(void){
	string wname;
	circuit cirXor;

	gate encXOR;

	encXOR.fanins.push_back(cirXor.add_wire("A", wtype::IN));
	encXOR.fanins.push_back(cirXor.add_wire("B", wtype::IN));
	encXOR.fanouts = {cirXor.add_wire("out", wtype::OUT)};

	cirXor.add_gate("encXOR", encXOR.fanins, encXOR.fanouts[0], fnct::XOR);

	return cirXor;
}

circuit encXNOR(void){
	string wname;
	circuit cirXnor;

	gate encXNOR;

	encXNOR.fanins.push_back(cirXnor.add_wire("A", wtype::IN));
	encXNOR.fanins.push_back(cirXnor.add_wire("B", wtype::IN));
	encXNOR.fanouts = {cirXnor.add_wire("out", wtype::OUT)};

	cirXnor.add_gate("encXNOR", encXNOR.fanins, encXNOR.fanouts[0], fnct::XNOR);

	return cirXnor;
}

circuit encAND(void){
	string wname;
	circuit cirAnd;

	gate encAND;

	encAND.fanins.push_back(cirAnd.add_wire("A", wtype::IN));
	encAND.fanins.push_back(cirAnd.add_wire("B", wtype::IN));
	encAND.fanouts = {cirAnd.add_wire("out",wtype::OUT)};

	cirAnd.add_gate("encAND", encAND.fanins, encAND.fanouts[0], fnct::AND);

	return cirAnd;
}

circuit encANDN(void){
	string wname;
	circuit cirAnd;

	gate ANDNand;
	gate ANDNinv;

	ANDNand.fanins.push_back(cirAnd.add_wire("A", wtype::IN));
	id Bnot = cirAnd.add_wire("Bnot", wtype::INTER);
	ANDNand.fanins.push_back(Bnot);
	ANDNinv.fanins.push_back(cirAnd.add_wire("B", wtype::IN));
	ANDNinv.fanouts = {Bnot};
	ANDNand.fanouts = {cirAnd.add_wire("out", wtype::OUT)};

	cirAnd.add_gate("encXOR", ANDNand.fanins, ANDNand.fanouts[0], fnct::AND);
	cirAnd.add_gate("encXOR", ANDNinv.fanins, ANDNinv.fanouts[0], fnct::NOT);

	return cirAnd;
}


circuit encOR(void){
	string wname;
	circuit cirOr;

	gate encOR;

	encOR.fanins.push_back(cirOr.add_wire("A", wtype::IN));
	encOR.fanins.push_back(cirOr.add_wire("B", wtype::IN));
	encOR.fanouts = {cirOr.add_wire("out",wtype::OUT)};

	cirOr.add_gate("encOR", encOR.fanins, encOR.fanouts[0], fnct::OR);
	build_wire_InOutSets(cirOr);

	return cirOr;
}


circuit Inverter(void){
	string wname;
	circuit cirInv;

	gate inverter;

	inverter.fanins.push_back(cirInv.add_wire("A", wtype::IN));
	inverter.fanouts = {cirInv.add_wire("out", wtype::OUT)};

	cirInv.add_gate("Inv", inverter.fanins, inverter.fanouts[0], fnct::NOT);
	build_wire_InOutSets(cirInv);

	return cirInv;
}

void build_wire_InOutSets(circuit& ckt){
	foreach_gate(ckt, g, gid) {
		for (auto& y : g.fanins){
			if ( !_is_in(gid, ckt.wfanout(y)) ) {
				std::cout << "fanout fixed for" << ckt.wname(y) << "\n";
				ckt.getwire(y).addfanout(gid);
			}
		}
		if (_is_not_in( gid, ckt.wfanins(g.fo0())) ){
			std::cout << "fanin fixed\n";
			ckt.getwire(g.fo0()).fanins.push_back(gid);
		}
	}
	return;
}

void test_all(void){

	circuit ckt = Inverter();
	std::cout << "\n\n inverter \n";
	ckt.write_bench();

	ckt = encOR();
	std::cout << "\n\n encOR \n";
	ckt.write_bench();

	ckt = encANDN();
	std::cout << "\n\n ANDN \n";
	ckt.write_bench();

	ckt = encXOR();
	std::cout << "\n\n encXOR \n";
	ckt.write_bench();

	ckt = encXNOR();
	std::cout << "\n\n encXNOR \n";
	ckt.write_bench();

	ckt = lut(4);
	std::cout << "\n\n LUT 4in \n";
	ckt.write_bench();

	ckt = nmux(3, 5);
	std::cout << "\n\n mux 3sel \n";
	ckt.write_bench();

	ckt = n_switchbox(4);
	std::cout << "\n\n n_switchbox 4in-out \n";
	ckt.write_bench();

	ckt = key_comparator(5);
	std::cout << "\n\n key comparator 5 \n";
	ckt.write_bench();

	boolvec key(4,0);
	ckt = fixed_comparator(4, key);
	std::cout << "\n\n fixed comparator 4in\n";
	ckt.write_bench();

	return;
}

} // end of namespace

