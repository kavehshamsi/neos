/*
 * rwr.cpp
 *
 *  Created on: Sep 25, 2019
 *      Author: kaveh
 *      Description: AIG rewriting engine
 */

#include "opt/rwr.h"
#include <bitset>
#include <array>
#include <dirent.h>
#include <utl/ext_cmd.h>
#include <utl/utility.h>

namespace opt_ns {

#define foreach_npn_tt(ptt, rtt) \
		for (auto _macro_npit = npn_enumerator(rtt, (*this)); _macro_npit.next(ptt);)


rewrite_manager_t::rewrite_manager_t(int percomp) {

	// std::cout << "starting rewrite engine\n";
	auto tm = _start_wall_timer();

	build_npn_perms();

	precompute = percomp;

	if ( precompute ) {
		// uint32_t max_iter = 20000;
		uint64_t iter = 0;

		if (use_prac_only)
			load_prac_from_string(prac_str);

		alit a  = add_var("A", 0xAAAA);
		alit b  = add_var("B", 0xCCCC);
		alit c  = add_var("C", 0xF0F0);
		alit d  = add_var("D", 0xFF00);
		alit c0 = add_var("C0", 0x0000);

		Oset<idpair> tried_pair;

		L = {a, b, c, d};

		bool done = false;
		while (true) {

			uint Ls = L.size();
			for (uint i0 = 0; i0 < Ls; i0++) {
				for (uint i1 = i0 + 1; i1 < Ls; i1++) {
					alit n0 = L[i0], n1 = L[i1];
					idpair tp = {n0.lid(), n1.lid()};
					if ( _is_in(tp, tried_pair) )
						continue;
					tried_pair.insert(tp);

					const auto& rnd0 = rwrdag.at(n0.lid());
					const auto& rnd1 = rwrdag.at(n1.lid());

					if (rnd0.level + rnd1.level > 5) {
						done = true;
						break;
					}

					try_node(n0, n1);
					try_node(n0, ~n1);
					try_node(~n0, n1);
					try_node(~n0, ~n1);
					try_node(n0, n1, 1);
				}

				if ( done || num_npn_class >= (use_prac_only ? prac_npns.size() : 222) ) {
					done = true;
					break;
				}
			}

			V1(std::cout << "\nbnsize: " << rwrdag.size() << "\n");

			if ( done ) {
				break;
			}
		}

		// cleanup dag of non-optimal nodes
		cleanup_dag();

		V1(std::cout << "nodemap size: " << rwrdag.size() << "\n");

		std::stringstream ss;
		write_dag_to_ss(ss);
		string ss_str = ss.str();
		string table_txtfile = "const std::string rewrite_manager_t::dag_str"
				" = R\"(" + ss_str + "\n)\";";
		std::ofstream("src/opt/rwr_table.txt") << table_txtfile;

		load_dag_from_string(ss.str());

		/*for (auto& x : canonTabs) {
			if (!x.is_prac)
				continue;
			std::cout << "\n\nat class " << x.canon_class << ":\n";
			for (auto& y : x.mindag_list) {
				std::cout << "\n";
				print_dagnode(allTabs[y].dag_node.lid());
			}
			std::cout << "\n";
		}*/
	}
	else {
		load_prac_from_string(prac_str);
		load_dag_from_string(dag_str);
	}


	pst.setup_time += _stop_wall_timer(tm);
	// std::cout << "setup time: " << pst.setup_time << "\n";

	/*for (auto& p : tt2nd) {
		std::bitset<16> bc = p.first;
		std::cout << p.second << " -> {" << bc << "\n";
	}*/
}

void rewrite_manager_t::cleanup_dag() {
	idset live_nodes;
	for (auto& ct : canonTabs) {
		//std::cout << "mindag list size: " << canonMap[utt].mindag_list.size() << "\n";
		for (auto x : ct.mindag_list) {
			oidset tfanin;
			dag_trans_fanin(allTabs[x].dag_node.lid(), tfanin);
			utl::add_all(live_nodes, tfanin);
		}
	}

	for (auto inv : invars)
		live_nodes.insert(inv);

	auto ndmp = rwrdag;
	for (auto p : ndmp) {
		if (_is_not_in(p.second.nid, live_nodes)) {
			rwrdag.erase(p.first);
		}
	}

}

bool rewrite_manager_t::npn_enumerator::next(truth_t& ntt) {
	ntt = negpermute_table(rtt, ix);
	lix = ix;
	// std::cout << flip << "  " << curn << "  " << curp << "\n";
	if (++ix.flip == 2) {
		ix.flip = 0;
		if (++ix.nInd == 16) {
			ix.nInd = 0;
			++ix.pInd;
		}
	}

	if (ix.pInd == 24 && ix.nInd == 0 && ix.flip == 0)
		return false;

	return true;
}

template<size_t N>
inline std::bitset<N> reverse_bitset(const std::bitset<N>& bs) {
	std::bitset<N> ret;
	for (uint i = 0; i < N; i++) {
		ret[N - 1 - i] = bs[i];
	}
	return ret;
}

template<typename T, size_t N>
inline std::array<T, N> reverse_array(const std::array<T, N>& arr) {
	std::array<T, N> ret;
	for (uint i = 0; i < N; i++) {
		ret[N - 1 - i] = arr[i];
	}
	return ret;
}

void rewrite_manager_t::build_canon_class(truth_t ctt) {

	//std::cout << "utt : " << utt << "\n";
	if (allTabs[ctt].canon_class == -1) {
		//std::cout << "class : " << std::bitset<16>(ctt) << "\n";
		canonTabs.push_back(canon_data_t());
		int canon_class = canonTabs.size() - 1;
		canonTabs.back().canon_class = canon_class;
		canonTabs.back().ctt = ctt;

		allTabs[ctt].is_canon = true;
		allTabs[ctt].canon_class = canon_class;
		allTabs[ctt].indx = {0, 0, 0};
		truth_t ptt = ctt;
		//std::cout << "for class : " << ctt << "\n";
		for (auto npit = npn_enumerator(ctt, *this); npit.next(ptt);) {
			if (allTabs[ptt].canon_class == -1) {
				//std::cout << "   at ptt : " << ptt << "\n";
				allTabs[ptt].canon_class = canon_class;
				assert(!allTabs[ptt].is_canon);
				allTabs[ptt].indx = npit.lix;
			}
		}
	}

}

void rewrite_manager_t::build_npn_perms() {

	std::array<int, 4> arr = {0, 1, 2, 3};
	uint per = 0;
	do {
		varperms[per] = arr;
		//std::cout << per << " : " << to_str(arr) << "\n";
		for (uint i = 0; i < 16; i++) {
			std::bitset<4> bs = i;
			uint num = 0;
			for (uint x = 0; x < 4; x++) {
				num += (bs[x] << arr[x]);
			}
			perms[per][i] = num;
			//std::cout << "  " << num << "\n";
		}
		per++;
	}
	while (std::next_permutation(arr.begin(), arr.end()));

	for (uint i = 0; i < 16; i++) {
		/*std::bitset<4> bs = i;
		std::cout << "for " << bs << "\n";*/
		for (uint j = 0; j < 16; j++) {
			uint npos = j ^ i;
			negs[i][j] = npos;
			//std::cout << "  " << npos << "\n";
		}
	}

	int32_t nfuncs = (1 << 16);

	// std::cout << "nfuncs: " << nfuncs << "\n";

	allTabs = vector<tt_data_t>(nfuncs);

	build_canon_class(0x0000);
	build_canon_class(0xAAAA);
/*	build_canon_class(0xCCCC);
	build_canon_class(0xF0F0);
	build_canon_class(0xFF00);*/



	for (truth_t ctt = 0; ctt != nfuncs - 1; ctt++) {
		build_canon_class(ctt);
	}

	for (auto& x : allTabs) {
		assert(x.canon_class != -1);
	}

	// std::cout << "nClasses: " << canonTabs.size() << "\n";
}


void rewrite_manager_t::write_dag_to_ss(std::stringstream& ss) {

	id newid = 0;
	id2idHmap old2new;
	old2new[-1] = -1;

	for (uint i = 0; i < invars.size(); i++) {
		old2new[invars[i]] = newid++;
	}

	for (auto p : rwrdag) {
		if (p.second.input_index == -1) {
			old2new[p.second.nid] = newid++;
		}
	}

	for (auto p : rwrdag) {
		if (!_is_in(p.second.a0.lid(), old2new))
			neos_abort("failed at " << p.second.a0);
		if (!_is_in(p.second.a1.lid(), old2new))
			neos_abort("failed at " << p.second.a1);
	}

	ss << "DAG\n";
	for (auto p : rwrdag) {
		alit n0(old2new.at(p.second.a0.lid()), p.second.a0.sgn());
		alit n1(old2new.at(p.second.a1.lid()), p.second.a1.sgn());
		assert(p.second.input_index != -1 || p.second.node_size != 0);
		ss << old2new.at(p.second.nid) << " " << n0 << " " << n1
				<< " " << p.second.node_size << " \n";
	}

	ss << "NPN\n";
	for (auto& ctd : canonTabs) {
		assert(allTabs[ctd.ctt].is_canon);
		// std::cout << "class " << std::bitset<16>(ctd.ctt) << " mindag: " << ctd.mindag_list.size() << "\n";
		for (auto xtt : ctd.mindag_list) {
			const auto& x = allTabs[xtt];
			ss << std::bitset<16>(xtt) << " -> "
					<< alit(old2new.at(x.dag_node.lid()), x.dag_node.sgn()) << "\n";
		}
	}
	std::cout << "wrote text to string\n";
}

void rewrite_manager_t::load_dag_from_string(const std::string& str) {
	std::stringstream ss(str);
	std::string line;
	int state = 0;

	invars = idvec(5, 0);
	rwrdag.clear();
	for (auto& x : canonTabs) {
		x.mindag_list.clear();
	}

	while(getline(ss, line)) {
		//std::cout << line << "\n";
		if (line == "")
			continue;

		if (line == "DAG") {
			state = 0;
			continue;
		}

		if (line == "NPN") {
			state = 1;
			continue;
		}

		if (state == 0) {
			id nid, f0, f1;
			int b0, b1;
			int ns = 0;
			//std::cout << line;
			sscanf(line.c_str(), "%lld %lld:%d %lld:%d %ud\n",
					&nid, &f0, &b0, &f1, &b1, &ns);
			//std::cout << "read node " << nid << " " << f0
				//	<< " " << f1 << " " << b0 << " " << b1 << " " << ns << "\n";
			id input_index = (f1 == -1 || f0 == -1) ? nid:-1;
			auto& rwrnd = rwrdag[nid] = {nid, alit(f0, b0), alit(f1, b1), {}, ns, 0, 0, input_index};
			if (input_index != -1) {
				invars[input_index] = nid;
				rwrnd.level = 0;
				assert(input_index == nid);
				// std::cout << "is input " << input_index << "\n";
			}
			else {
				rwrnd.level = MAX(rwrdag.at(f0).level, rwrdag.at(f1).level) + 1;
			}
		}
		else if (state == 1) {
			char ttcstr[25];
			id nid;
			int cm;
			sscanf(line.c_str(), "%s -> %lld:%d\n", ttcstr, &nid, &cm);
			//std::cout << line << "\n";
			truth_t tt = strtol(ttcstr, nullptr, 2);
			//std::cout << std::bitset<16>(tt) << " " << nid << ":" << cm << "\n";
			allTabs[tt].dag_node = alit(nid, cm);
			allTabs[(truth_t)~tt].dag_node = ~(alit(nid, cm));
			rwrdag.at(nid).tt = tt;

			allTabs[tt].visited = true;

			int cc = allTabs[tt].canon_class;

			//std::cout << "cc: " << cc << "\n";
			canonTabs[cc].mindag_list.push_back(tt);
		}
	}

	for (uint i = 0; i < canonTabs.size(); i++) {
		for (auto xtt : canonTabs[i].mindag_list) {
			assert(allTabs[xtt].canon_class == i);
		}
	}

	for (auto& x : allTabs) {
		assert(x.canon_class != -1);
	}

	rebuild_fanouts();
	build_dagsorts();

	pst.dag_tsort_time = 0;

	//std::cout << "DAG size : " << rwrdag.size() << "\n";

	/*
	for (auto& p : npntts) {

		//std::cout << "AT CLASS : " << std::bitset<16>(p.first) << " -> " << p.second << "\n";
		Hmap<id, truth_t> simmap;
		simmap[invars[0]] = 0xAAAA;
		simmap[invars[1]] = 0xCCCC;
		simmap[invars[2]] = 0xF0F0;
		simmap[invars[3]] = 0xFF00;
		simmap[invars[4]] = 0x0000;

		idvec tsorted_rwrnids = topsort_trans_fanin(p.second.lid());

		for (auto nid : tsorted_rwrnids) {
			const auto& dagnd = nodemap.at(nid);
			if (dagnd.input_index != -1) {
			}
			else {


				truth_t tt0 = simmap.at(dagnd.a0.lid()) ^ (dagnd.a0.sgn() ? 0xffff:0);
				truth_t tt1 = simmap.at(dagnd.a1.lid()) ^ (dagnd.a1.sgn() ? 0xffff:0);

				//std::cout << dagnd.a0 << "  : " << std::bitset<16>(tt0) << "  " << std::bitset<16>(nodemap.at(dagnd.a0.lid()).tt) << "\n";
				//std::cout << dagnd.a1 << "  : " << std::bitset<16>(tt1) << "  " << std::bitset<16>(nodemap.at(dagnd.a1.lid()).tt) << "\n";

				simmap[dagnd.nid] = tt0 & tt1;
			}

			//std::cout << std::left << std::setw(4) << dagnd.nid << " : " << std::bitset<16>(simmap.at(dagnd.nid)) << "\n";
		}

		truth_t rtt = simmap.at(p.second.lid()) ^ (p.second.sgn() ? 0xffff:0);
		if (rtt != p.first) {
			neos_abort(" != " << std::bitset<16>(p.first));
		}
		else {
			//std::cout << rtt << " == " << p.first << "\n";
		}
	}
 	 */

	// std::cout << "PASSED READIN\n";

}


void rewrite_manager_t::rebuild_fanouts() {

	for (auto p : rwrdag) {
		p.second.fanouts.clear();
	}

	for (auto p : rwrdag) {
		for (auto fanin : {p.second.a0.lid(), p.second.a1.lid()}) {
			if (fanin != -1) {
				rwrdag.at(fanin).fanouts.insert(p.first);
			}
		}
	}
}

void rewrite_manager_t::build_dagsorts() {
	for (const auto& p : rwrdag) {
		dagsorts[p.first] = topsort_dag_trans_fanin(p.first);
	}
	dagsorts_built = true;
}

alit rewrite_manager_t::add_var(const std::string& nm, truth_t tt) {
	alit xl = alit(max_id);
	rwrdag[max_id] = {max_id, -1, -1, {}, 0, 0, tt, max_id, -1};
	invars.push_back(xl.lid());
	allTabs[tt].dag_node = xl;
	allTabs[(truth_t)~tt].dag_node = ~xl;
	allTabs[tt].visited = true;
	allTabs[(truth_t)~tt].visited = true;
	int cc = allTabs[tt].canon_class;
	canonTabs[cc].is_var = true;
	canonTabs[cc].mindag_list.push_back(tt);
	max_id++;
	return xl;
}

alit rewrite_manager_t::add_and(alit a0, alit a1, truth_t tt, uint& nc, int& lvl) {
	auto& nr = rwrdag[max_id] = {max_id, a0, a1, {}, 0, -1, tt, -1};
	auto& rf0 = rwrdag.at(a0.lid());
	auto& rf1 = rwrdag.at(a1.lid());
	rf0.fanouts.insert(max_id);
	rf1.fanouts.insert(max_id);
	nc = rwrdag.at(max_id).node_size = count_nodes(a0, a1, 0);
	lvl = MAX(rf0.level, rf1.level) + 1;
	nr.tt = tt;
	nr.level = lvl;
	return alit(max_id++, 0);
}

alit rewrite_manager_t::add_xor(alit a0, alit a1, truth_t tt0, truth_t tt1, uint& nc, int& lvl) {
	tt0 ^= (a0.sgn() ? 0xffff:0);
	tt1 ^= (a1.sgn() ? 0xffff:0);

	alit o0 = add_and(a0, ~a1, tt0 & ~tt1, nc, lvl);
	alit o1 = add_and(~a0, a1, ~tt0 & tt1, nc, lvl);
	alit o = ~add_and(~o0, ~o1, ~(tt0 ^ tt1), nc, lvl);
	return o;
}

void rewrite_manager_t::print_dagnode(id dagnd) {

	const auto& tsorted = topsort_dag_trans_fanin(dagnd);

	for (auto x : tsorted) {
		if (rwrdag.at(x).input_index != -1) {
			std::cout << "INPUT(" << x << ")\n";
		}
		else {
			std::cout << x << " = and(" << rwrdag.at(x).a0 << ", " << rwrdag.at(x).a1 << ")\n";
		}
	}
	std::cout << "OUTPUT(" << dagnd << ")\n";

}

void rewrite_manager_t::try_node(alit a0, alit a1, bool exor) {
	const auto& rnd0 = rwrdag.at(a0.lid());
	const auto& rnd1 = rwrdag.at(a1.lid());
	truth_t tt0 = rnd0.tt;
	truth_t tt1 = rnd1.tt;
	truth_t tt;
	if (exor)
		tt = (a0.sgn() ? ~tt0:tt0) ^ (a1.sgn() ? ~tt1:tt1);
	else
		tt = (a0.sgn() ? ~tt0:tt0) & (a1.sgn() ? ~tt1:tt1);

	uint nc;
	int nwlvl;

	int cc = allTabs[tt].canon_class; // canon class index

	if (!canonTabs[cc].is_prac)
		return;

	alit nn = -1;

	if (!canonTabs[cc].has_dag) {
		std::cout << "new npn class: " << ++num_npn_class << "\r";
		canonTabs[cc].has_dag = true;
	}

	if (!allTabs[tt].visited) {
		// create new node in dag
		nn = exor ? add_xor(a0, a1, tt0, tt1, nc, nwlvl) : add_and(a0, a1, tt, nc, nwlvl);

		allTabs[tt].visited = true;
		allTabs[(truth_t)~tt].visited = true;
		num_tt_discovered++;

		// add to list for exploration
		L.push_back(nn.lid());
	}
	else {
		// no point in adding new node
		return;
	}

	for (auto xtt : canonTabs[cc].mindag_list) {
		const auto& xnd = rwrdag.at(allTabs[xtt].dag_node.lid());
		if (nc > xnd.node_size && nwlvl > xnd.level )
			return;
	}

	allTabs[tt].dag_node = nn;
	allTabs[(truth_t)~tt].dag_node = ~nn;

	if (canonTabs[cc].mindag_list.size() < struct_per_class_limit) {
		canonTabs[cc].mindag_list.push_back(tt);
	}

	/*
	// if smaller than best dag
	if ( nc <= mindag_size ) {

		allTabs[tt].dag_node = nn;
		allTabs[(truth_t)~tt].dag_node = ~nn;

		if (nc == mindag_size) {
			// add to allTabs
			if (canonTabs[cc].mindag_list.size() < struct_per_class_limit) {
				canonTabs[cc].mindag_list.push_back(tt);
			}
		}
		else if (nc < mindag_size) {
			canonTabs[cc].mindag_list.clear();
			canonTabs[cc].mindag_list.push_back(tt);
		}

		canonTabs[cc].mindag_size = nc;

	}
*/
}

void rewrite_manager_t::dag_trans_fanin(id aid, oidset& tfanin) {

	idque Q;

	if (rwrdag.at(aid).input_index == -1) {
		Q.push(aid);
		tfanin.insert(aid);
	}

	while ( !Q.empty() ) {

		id curid = Q.front();
		Q.pop();

		const auto& nd = rwrdag.at(curid);

		for (auto fanin : {nd.a0.lid(), nd.a1.lid()} ) {
			if (_is_not_in(fanin, tfanin) && fanin != -1
					&& rwrdag.at(fanin).input_index == -1) {
				Q.push(fanin);
				tfanin.insert(fanin);
			}
		}
	}

}

idset rewrite_manager_t::dag_trans_fanin_winputs(id aid) {

	idset tfanin;
	idque Q;

	if (rwrdag.at(aid).input_index == -1) {
		Q.push(aid);
		tfanin.insert(aid);
	}

	while ( !Q.empty() ) {

		id curid = Q.front();
		Q.pop();

		const auto& nd = rwrdag.at(curid);

		for (auto fanin : {nd.a0.lid(), nd.a1.lid()} ) {
			if (_is_not_in(fanin, tfanin) && fanin != -1) {
				Q.push(fanin);
				tfanin.insert(fanin);
			}
		}
	}

	return tfanin;
}


idvec rewrite_manager_t::topsort_dag_trans_fanin(id aid) {

	auto tm = _start_wall_timer();

/*
	auto it = dagnd2tsort.find(aid);
	if (it != dagnd2tsort.end())
		return (*it).second;

	std::cout << "building topsort\n";
	//idset tfanin = dag_trans_fanin(aid);

	idset tfanin2 = dag_trans_fanin_winputs(aid);
	tfanin2.insert(aid);
	idvec tsfanin2;
	std::map<id, id> tsfanin2map;
	for (auto x : tfanin2) {
		tsfanin2map[dagnd2lev.at(x)] = x;
	}
	for (auto& p : tsfanin2map) {
		tsfanin2.push_back(p.second);
	}

	dagnd2tsort[aid] = tsfanin2;

	pst.dag_tsort_time += _stop_wall_timer(tm);

	return dagnd2tsort.at(aid);
*/

/*
	idvec tsfanin2;

	for (auto x : dag_topsort) {
		if (_is_in(x, tfanin2)) {
			tsfanin2.push_back(x);
		}
	}
*/


	//tsfanin2.push_back(aid);


	idvec tsfanin;
	oidset tfanin;
	dag_trans_fanin(aid, tfanin);
	for (auto x : invars) {
		tfanin.insert(x);
	}

	Omap<int, int> degmap;
	idque deg_zero;

	for (auto tf : tfanin) {
		if (_is_not_in(tf, degmap)) {
			degmap[tf] = 0;
		}
		for (auto fo : rwrdag.at(tf).fanouts) {
			if (_is_in(fo, tfanin)) {
				degmap[tf]++;
			}
		}
	}

	deg_zero.push(aid);
	tsfanin.push_back(aid);

	while (!deg_zero.empty()) {
		id curid = deg_zero.front(); deg_zero.pop();
		const auto& nd = rwrdag.at(curid);
		//std::cout << " exploring " << rwrdag.at(curid).nid << " "
			//	<< rwrdag.at(curid).a0 << " " << rwrdag.at(curid).a1 << "\n";
		for (auto fanin : {nd.a0.lid(), nd.a1.lid()}) {
			if (fanin != -1 && --degmap[fanin] == 0) {
				deg_zero.push(fanin);
				tsfanin.push_back(fanin);
				// std::cout << "added fanin " << fanin << "\n";
			}
		}
	}

	std::reverse(tsfanin.begin(), tsfanin.end());

	//std::cout << to_str(tsfanin2, " ") << "} should be {" << to_str(tsfanin, " ") << "}\n";

	//assert(tsfanin2 == tsfanin);

	pst.dag_tsort_time += _stop_wall_timer(tm);

	return tsfanin;

}

id rewrite_manager_t::count_nodes(alit root) {
	oidset tfanin;
	dag_trans_fanin(root.lid(), tfanin);
	return tfanin.size();
}

id rewrite_manager_t::count_nodes(alit a0, alit a1, bool exor) {
	oidset a0fanin, a1fanin;
	dag_trans_fanin(a0.lid(), a0fanin);
	dag_trans_fanin(a1.lid(), a1fanin);
	return utl::set_union(a0fanin, a1fanin).size() + (exor ? 3:1);
}


void rewrite_manager_t::cut_enumerate(const aig_t& rntk, id aid, cutvec_t& cuts, uint K) {
	//std::cout << "now on node: ";
	//ntk.print_node(aid);

	//assert(rntk.error_check());

	auto tm = _start_wall_timer();

	cuts = { {aid, oidset(), oidset({aid})} };
	idvec S = {0};

	Oset<oidset> all_cuts;

	while(!S.empty()) {

		id curind = S.back(); S.pop_back();
		auto cp = cuts[curind].inputs;

		for (auto nid : cp) {

			const auto& nd = rntk.getcNode(nid);
			if (nd.isAnd()) {
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
						S.push_back(cuts.size() - 1);
					}
				}
			}
		}
	}

	// std::cout << "cut num: " << cuts.size() << "\n";

	pst.cut_time += _stop_wall_timer(tm);

}

// list npnclasses of a given aig
void rewrite_manager_t::aig_npnclasses(const aig_t& rntk, Hset<truth_t>& npnclasses) {

	pst = rewrite_manager_t::stats();

	idvec nodes;
	foreach_and_ordered(rntk, aid) {
		nodes.push_back(aid);
	}

	//simmap = vector<truth_t>(rntk.get_max_id(), 0);

	for (auto aid : nodes) {
		if (rntk.node_exists(aid) && rntk.isAnd(aid)) {
			cutvec_t cuts;
			cut_enumerate(rntk, aid, cuts, 4);
			//std::cout << rntk.nName(aid) << " : \n";
			for (auto& cut : cuts) {
				if (cut.inputs.size() == 4) {

					curcut = &cut;
					curcut_invec = to_vec(cut.inputs);

					simmap[curcut_invec[0]] = 0xAAAA;
					simmap[curcut_invec[1]] = 0xCCCC;
					simmap[curcut_invec[2]] = 0xF0F0;
					simmap[curcut_invec[3]] = 0xFF00;

					if (rntk.has_const0()) {
						simmap[rntk.get_cconst0id()] = 0x0000;
					}

					curcut_aorder.clear();
					topsort_cut(rntk, cut, curcut_aorder);

					truth_t rtt = simulate_cut(rntk);
					// std::cout << "rtt is " << std::bitset<16>(rtt) << "\n";

					npnclasses.insert(allTabs[rtt].canon_class);
				}
			}
			//std::cout << "\n\n";
		}
	}

}

void rewrite_manager_t::rewrite_aig(aig_t& rntk) {

	ntk = &rntk;
	aig_t orig_ntk = rntk;

	pst = rewrite_manager_t::stats(); // make sure things are cleared

	auto tm = _start_wall_timer();
	pst.original_nodes = ntk->nAnds();

	//travidvec = idvec(ntk->get_max_id(), -1);
	//simmap = vector<truth_t>(ntk->get_max_id(), 0);

	idvec ordered_nodes = rntk.top_andorder();

	pst.tsort_time += _stop_wall_timer(tm);

	pst.replaced = 0;
	uint count = 0;
	for (auto aid : ordered_nodes) {
		if (ntk->node_exists(aid) && ntk->isAnd(aid)) {
			// std::cout << count++ << "/" << ntk->nNodes() << "\n";
			if (rewrite_node(aid)) {
				pst.replaced++;
				continue;
			}
			/*if (replaced == 10)
				break;*/
			//std::cout << "\n\n";
		}
	}

	//assert(ntk->error_check());
	// ntk->remove_dead_logic();

	pst.final_nodes = ntk->nAnds();
	pst.total_time = _stop_wall_timer(tm);

	V1(print_stats());

	// assert(ext::check_equivalence_abc(orig_ntk.to_circuit(), ntk->to_circuit()));
}

void rewrite_manager_t::print_cut(const aig_t& rntk, const cut_t& cut) {
	for (auto xid : cut.inputs) {
		std::cout << "INPUT(" << rntk.ndname(xid) << ")\n";
	}
	std::cout << "OUTPUT(" << rntk.ndname(cut.root) << ")\n";
	for (auto aid : cut.ands) {
		rntk.print_node(aid);
	}
}

void rewrite_manager_t::topsort_cut(const aig_t& rntk, const cut_t& cut, idvec& node_order) {
	auto tm = _start_wall_timer();

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

	pst.cut_sort_time += _stop_wall_timer(tm);

}

truth_t rewrite_manager_t::simulate_cut(const aig_t& rntk) {

	//std::cout << "\n\ncut sim : \n";

	/*
	uint cni = 0;
	for (auto x : cut.inputs) {
		std::cout << "cutin" << cni++ << ": " << ntk->nName(x) << " " << std::bitset<16>(simmap.at(x)) << "\n";
	}
 	 */

	auto tm = _start_wall_timer();

	for (auto aid : curcut_aorder) {
		const auto& ad = rntk.getcNode(aid);

		if (!ad.isAnd()) {
			neos_abort("node to simulate in cut is not an AND: " << rntk.ndname(aid));
		}

		truth_t tt0, tt1;
		/*if (_is_not_in(ad.fi0(), simmap) || _is_not_in(ad.fi1(), simmap))
			neos_abort("cant find node");*/

		tt0 = (simmap.at(ad.fi0()) ^ (ad.cm0() ? 0xffff:0));
		tt1 = (simmap.at(ad.fi1()) ^ (ad.cm1() ? 0xffff:0));

		// std::cout << "cutnd : " << ntk->nName(aid) << " " << ntk->nName(ad.fanin0) << " " << ntk->nName(ad.fanin1) << " " << std::bitset<16>(tt0 & tt1) << "\n";

		simmap[aid] = (tt0 & tt1);

		//assert(_is_not_in(aid, curcut->inputs));
	}
	//std::cout << "\n";

	/*if (_is_not_in(cut.root, simmap))
		neos_abort("cant find node " << ntk->nName(cut.root));*/

	pst.sim_time += _stop_wall_timer(tm);

	return simmap.at(curcut->root);
}

void rewrite_manager_t::find_practical_classes(const std::string& bench_dir) {

	auto dirp = opendir(bench_dir.c_str());
	dirent* dp;
	Hset<truth_t> prac_npn_set;
	while ((dp = readdir(dirp)) != nullptr) {
		std::string filename = dp->d_name;
		if (_is_in(".bench", filename)) {
			std::cout << "file : " << filename << " ";
			aig_t rntk(bench_dir + "/" + filename);
			aig_npnclasses(rntk, prac_npn_set);
			std::cout << " npnclasses -> " << prac_npn_set.size() << "\n";
		}
	}

	std::cout << "total present npnclasses: " << prac_npn_set.size() << "\n";

	prac_npns = utl::to_vec(prac_npn_set);

	std::stringstream ss;
	write_prac_to_ss(ss);
	string ss_str = ss.str();
	string table_txtfile = "const std::string rewrite_manager_t::prac_str"
			" = R\"(" + ss_str + "\n)\";";
	std::ofstream("src/opt/rwr_prac_npns.txt") << table_txtfile;

	(void)closedir(dirp);
}

void rewrite_manager_t::write_prac_to_ss(std::stringstream& ss) {
	for (const auto& x : prac_npns) {
		ss << std::bitset<16>(x) << "\n";
	}
}

void rewrite_manager_t::load_prac_from_string(const std::string& str) {

	std::stringstream ss(str);
	string line;
	int state = 0;

	prac_npns.clear();

	while(getline(ss, line)) {
		//std::cout << line << "\n";
		if (line == "")
			continue;

		char ttcstr[30];
		sscanf(line.c_str(), "%s\n", ttcstr);
		truth_t tt = strtol(ttcstr, nullptr, 2);

		// std::cout << "read npn class" << ttcstr << "\n";
		prac_npns.push_back(tt);
	}

	for (auto& x : canonTabs) {
		x.is_prac = false;
	}

	for (auto& x : prac_npns) {
		canonTabs[x].is_prac = true;
	}

}

void rewrite_manager_t::print_indices(const npn_indices& indx) {

	std::cout << "f{" << indx.flip <<  "}   ";
	std::cout << "m{" << std::bitset<4>(indx.nInd) << "}  ";
	std::cout << "p{" << to_str(varperms[indx.pInd]) << "}\n";

}

void rewrite_manager_t::get_relative_perms(truth_t rtt, truth_t dtt,
		std::bitset<4>& inmasks, std::array<int, 4>& inperms, bool& outmask) {

	assert(allTabs[dtt].canon_class == allTabs[rtt].canon_class);

	//std::cout << "for class : " << ctt << "\n";
	alit dgal = allTabs[dtt].dag_node;

	int cc = allTabs[rtt].canon_class;
	const auto& dindx = allTabs[dtt].indx;
	const auto& rindx = allTabs[rtt].indx;

	outmask = dindx.flip ^ rindx.flip ^ dgal.sgn();

	auto rmask = std::bitset<4>(rindx.nInd);
	auto dmask = std::bitset<4>(dindx.nInd);

	const auto& rperm = varperms[rindx.pInd]; // root permuation
	const auto& dperm = varperms[dindx.pInd]; // dag permutation

	// Took me a week to figure out this piece!
	for (uint i = 0; i < 4; i++) {
		inperms[dperm[i]] = rperm[i];
		inmasks[dperm[i]] = rmask[i] ^ dmask[i];
	}

}

int64_t rewrite_manager_t::move_to_dag(truth_t rtt, truth_t dtt, const cut_t& cut,
		alit& nal, bool& invert) {

	auto tm2 = _start_wall_timer();
	static uint added = 0;
	/*std::cout << "CASE: " << added++ << "\n";
	print_cut(*ntk, cut);
	std::cout << std::bitset<16>(rtt) << "\n\n";*/

	int64_t pre_nc = ntk->nAnds();
	// std::cout << "best ptt : " << std::bitset<16>(best_ptt) << "\n";
	// Hmap<id, truth_t> dagsim;
	bool outmask;
	std::bitset<4> inmasks;
	std::array<int, 4> inperms;
	get_relative_perms(rtt, dtt, inmasks, inperms, outmask);

	alit dgal = allTabs[dtt].dag_node;
/*

	assert(allTabs[dtt].canon_class == allTabs[rtt].canon_class);

	//std::cout << "for class : " << ctt << "\n";
	alit dgal = allTabs[dtt].dag_node;

	int cc = allTabs[rtt].canon_class;
	auto dindx = allTabs[dtt].indx;
	auto rindx = allTabs[rtt].indx;

	for (auto x : canonTabs[cc].mindag_list) {
		assert(allTabs[x].canon_class == cc);
	}

	std::cout << "dindx: ";
	print_indices(dindx);
	std::cout << "rindx: ";
	print_indices(rindx);
	std::cout << "\n";


	truth_t ptt;
	for (auto npit = npn_enumerator(dtt, *this); npit.next(ptt);) {
		assert(allTabs[ptt].canon_class == allTabs[rtt].canon_class);
		assert(allTabs[ptt].canon_class == allTabs[dtt].canon_class);
		if (rtt == ptt) {
			std::cout << "match: ";
			print_indices(npit.lix);
		}
	}


	bool outmask = dindx.flip ^ rindx.flip ^ dgal.sgn();

	auto rmask = std::bitset<4>(rindx.nInd);
	auto dmask = std::bitset<4>(dindx.nInd);
	auto inmask = std::bitset<4>();

	auto rperm = varperms[rindx.pInd]; // root permuation
	auto dperm = varperms[dindx.pInd]; // dag permutation
	auto inperm = rperm; // overal permutation

	// Took me a week to figure out this piece!
	for (uint i = 0; i < 4; i++) {
		inperm[dperm[i]] = rperm[i];
		inmask[dperm[i]] = rmask[i] ^ dmask[i];
	}
*/

	/*std::cout << "\noindx: ";
	std::cout << "f{" << outmask <<  "}   ";
	std::cout << "m{" << omask << "}  ";
	std::cout << "p{" << to_str(operm) << "}\n";*/

	assert(dgal != -1);
	//std::cout << "\n Adding from DAG dag root is " << dgal << " at size: " << rwrdag.at(dgal.lid()).node_size <<  " with cut size: " << cut.ands.size() << "\n";
	// assert(npnal.sgn() == 0);

	const idvec& tsorted_rwrnids = dagsorts.at(dgal.lid());

	/*for (auto aid : tsorted_rwrnids) {
		const auto& dagnd = rwrdag.at(aid);
		if (dagnd.input_index != -1)
			std::cout << "INPUT(" << dagnd.input_index << ");\n";
		else
			std::cout << dagnd.nid << " = and(" << rwrdag.at(dagnd.a0.lid()).nid << ", " << rwrdag.at(dagnd.a1.lid()).nid << ");\n";
	}*/

	//Hmap<id, truth_t> simmap;
	auto invec = to_vec(cut.inputs);

	/*simmap[invec[0]] = 0xAAAA;
	simmap[invec[1]] = 0xCCCC;
	simmap[invec[2]] = 0xF0F0;
	simmap[invec[3]] = 0xFF00;

	if (ntk->has_const0()) {
		simmap[ntk->get_cconst0id()] = 0x0000;
	}

	std::cout << "R: " << cut.root << " INPUT: ";
	for (auto x : invec) {
		std::cout << x << ", ";
		if (!ntk->node_exists(x)) {
			neos_abort("cannot find node " << x << " ");
		}
	}
	std::cout << "\n";
	 */
	// Hmap<id, alit> dag2aig;

	/*for (uint i = 0; i < 4; i++) {
		permposes[varperms[best_ix.pInd][i]] = i;
	}*/


	auto tm1 = _start_wall_timer();
	//std::cout << "ordered: ";
	/*for (id nid : tsorted_rwrnids) {
		std::cout << nid << ", ";
	}
	std::cout << "\n";*/
	for (id nid : tsorted_rwrnids) {
		auto& dagnd = rwrdag.at(nid);

		//std::cout << "input index " << dagnd.input_index << " for " << nid << "\n";
		if (dagnd.input_index != -1) {
			//std::cout << "node " << nid << " is input\n";
			int i = dagnd.input_index;
			if (dagnd.input_index == 4) {
				dagnd.mpnd = ntk->get_const0() ; //^ masks[i];
				//dagsim[dagnd.nid] = 0;
			}
			else {
				int p = inperms[i];
				bool cm = inmasks[i];
				dagnd.mpnd = alit(invec[p], cm);

				//truth_t tt = simmap.at(invec[p]);
				//dagsim[dagnd.nid] = cm ? ~tt:tt;
				//std::cout << "dagnd: " << i << " -> " << p << " " << ntk->nName(invec[p]) << " " << cm << " " << std::bitset<16>(dagsim.at(dagnd.nid)) << "\n";
			}
		}
		else {
			alit in0 = rwrdag.at(dagnd.a0.lid()).mpnd ^ dagnd.a0.sgn();
			alit in1 = rwrdag.at(dagnd.a1.lid()).mpnd ^ dagnd.a1.sgn();
			//std::cout << "in0: " << in0 << " in1: " << in1 << "\n";
			if (in0 == -1 || in1 == -1) {
				neos_abort("problem with node " << dagnd.nid);
			}

			/*truth_t tt0 = dagsim.at(dagnd.a0.lid()) ^ (dagnd.a0.sgn() ? 0xffff:0);
			truth_t tt1 = dagsim.at(dagnd.a1.lid()) ^ (dagnd.a1.sgn() ? 0xffff:0);
			dagsim[dagnd.nid] = tt0 & tt1;
			std::cout << "dagnd: " << dagnd.nid << " and " << dagnd.a0 << "  " << dagnd.a1 << "  " << std::bitset<16>(tt0 & tt1) << "\n";*/

			auto tm0 = _start_wall_timer();
			bool strashed;
			dagnd.mpnd = ntk->add_and(in0, in1, "", strashed);
			//std::cout << "moving and " << in0 << ", " << in1 << " -> " << dagnd.mpnd << " " << strashed << "\n";
			if (dagnd.mpnd == cut.root) {
				neos_abort("move to dag got strashed to root\n");
				/*std::cout << "strashed to root " << cut.root << "\n";
				return -100;*/
			}
			pst.network_mod_time += _stop_wall_timer(tm0);
			//std::cout << "adding AND node ";
			/*if (strashed)
				std::cout << " was strashed to " << dagnd.mpnd.lid() << "\n";
			else
				std::cout << " new node at " << dagnd.mpnd.lid() << "\n";*/
		}
	}

	nal = rwrdag.at(dgal.lid()).mpnd;

	/*truth_t xtt = (nal.sgn() ^ outmask) ? ~dagsim.at(dgal.lid()):dagsim.at(dgal.lid());
	//std::cout << std::bitset<16>(xtt) << " ?= " << std::bitset<16>(rtt) << "\n";
	if (xtt != rtt) {
		std::cout << "DONT MATCH " << std::bitset<16>(rtt) << " != " << std::bitset<16>(xtt) << " \n";
		assert(false);
		//neos_error("don't match " << std::bitset<16>(rtt) << " != " << std::bitset<16>(xtt));
	}*/


	/*std::cout << "WORKING INDEX: ";
	std::cout << "f{" << outmask <<  "}   ";
	std::cout << "m{" << omask << "}  ";
	std::cout << "p{" << to_str(operm) << "}\n";*/

	//std::cout << "inverting " << (new_root.sgn() ^ outmask) << "\n";
	invert = nal.sgn() ^ outmask;
	//moved_fanouts = ntk->nfanouts(cut.root);

	auto tm0 = _start_wall_timer();

	//ntk->strash_fanout(new_root.lid());

	int64_t added_nc = ntk->nAnds() - pre_nc;

	ntk->move_all_fanouts(cut.root, nal.lid(), invert);
	pst.network_mod_time += _stop_wall_timer(tm0);

	//int64_t unused = count_unused_cone(*ntk, cut.root);

	if (ntk->node_exists(cut.root))
		ntk->remove_node_recursive(cut.root);

	assert(ntk->node_exists(nal.lid()));
	ntk->strash_fanout(nal.lid());


	//int64_t rgain = unused - added_nc;
	//std::cout << "again is " << rgain << " unused: " << unused <<  "  added: " << added_nc << "\n";

	pst.move_time += _stop_wall_timer(tm2);
	return 0;

}

void rewrite_manager_t::move_to_orig(const cut_t& cut, alit nr, bool invert) {

	// revert to original
	// std::cout << "rolling back \n";
	auto tm0 = _start_wall_timer();
	for (auto fout : moved_fanouts) {
		ntk->update_node_fanin(fout, nr.lid(), cut.root, invert);
	}

	//ntk->strash_fanout(cut.root);
	if (ntk->nfanouts(nr.lid()).empty())
		ntk->remove_node_recursive(nr.lid());

	pst.network_mod_time += _stop_wall_timer(tm0);
	//ntk->strash_fanout(new_root.lid());
	// assert(ntk->nfanouts(new_root.lid()).empty());

}

int rewrite_manager_t::label_mffc(id root, id travid) {

	//std::cout << "root mffc : " << root << "\n";

	int unused = 0;
	idque Q;
	Q.push(root);
	Omap<id, int> refcount;
	refcount[root] = 1;

	while ( !Q.empty() ) {

		id curid = Q.front(); Q.pop();
		const auto& curnd = ntk->getcNode(curid);

		//std::cout << " curnd " << curid << " " << travidvec[curid] << " fanouts: " << curnd.fanouts.size() << "\n";

		if (_is_not_in(curid, refcount)) {
			refcount[curid] = curnd.fanouts.size();
		}

		if (--refcount.at(curid) <= 0 && _is_not_in(curid, curcut->inputs)) {
			//std::cout << "unused " << curid << "\n";
			travidvec[curid] = travid;
			unused++;
			for ( auto inid : {curnd.fi0(), curnd.fi1()} ) {
				if (inid != -1) {
					//std::cout << "in : " << net.ndname(inid) << "\n";
					Q.push(inid);
				}
			}
		}
	}

	return unused;

/*	int num_mffc = 0;

	idque Q;
	idset visited;
	Q.push(root);
	while (!Q.empty()) {
		id curid = Q.front(); Q.pop();
		if (_is_in(curid, visited))
			continue;
		visited.insert(curid);
		num_mffc++;
		travidvec[curid] = travid;
		std::cout << "is mffc: " << curid << "\n";
		const auto& nd = ntk->getcNode(curid);
		if (nd.isAnd()) {
			for (auto fanin : {nd.fi0(), nd.fi1()}) {
				if (_is_in(fanin, curcut->inputs))
					continue;

				const auto& in_nd = ntk->getcNode(fanin);

				if (in_nd.fanouts.size() <= 1) {
					Q.push(fanin);
					//std::cout << "is mffc: " << fanin << "\n";
				}
				else {
					bool is_mffc = true;
					for (auto fout : in_nd.fanouts) {
						if (_is_not_in(fout, curcut->ands)) {
							is_mffc = false;
							break;
						}
					}
					if (is_mffc) {
						//std::cout << "is mffc: " << fanin << "\n";
						Q.push(fanin);
					}
				}
			}
		}
	}*/
	//std::cout << "mffc size : " << num_mffc << "\n";

/*

	if (nd.isAnd()) {
		for (auto fanin : {nd.fi0(), nd.fi1()}) {
			if (_is_in(fanin, curcut->inputs))
				continue;

			const auto& in_nd = ntk->getcNode(fanin);

			if (in_nd.fanouts.size() == 1) {
				num_mffc += label_mffc(fanin, travid);
			}
			else {
				for (auto fout : in_nd.fanouts) {
					if (_is_not_in(fout, curcut->ands))
						break;
				}
				num_mffc += label_mffc(fanin, travid);
			}
		}
	}
	return num_mffc;

*/

}

int rewrite_manager_t::eval_gain(truth_t rtt, truth_t& best_tt) {

/*	// for trivial vals can early exit
	if (canonTabs[cc].is_var) {
		std::cout << "is primitive var\n";
		best_tt = canonTabs[cc].ctt;
		return curcut->ands.size();
	}*/

	auto tm = _start_wall_timer();

	curtid++;
	curnum_mffc = label_mffc(curcut->root, curtid);
	//std::cout << "curmffc " << curnum_mffc << "\n";

	int best_gain = std::numeric_limits<int>::min();
	assert(curcc != -1);

	bool first_update = false;
	// std::cout << "for all mindags: " << canonTabs[curcc].mindag_list.size() << "\n";
	for (auto dtt : canonTabs[curcc].mindag_list) {

		// get dag node
		alit dgal = allTabs[dtt].dag_node;
		assert(dgal != -1);

		if (rwrdag.at(dgal.lid()).node_size > curcut->ands.size())
			continue;

		int added_nodes = count_added_nodes(rtt, dtt, dgal);

		if (added_nodes == -1)
			continue;

		int gain = curnum_mffc - added_nodes;

		if (gain > best_gain) {
			/*if (first_update)
				std::cout << "second update\n";
			first_update = true;*/
			// std::cout << "egain dtt : " << gain << "\n";
			best_tt = dtt;
			best_gain = gain;
		}
	}

	pst.eval_time += _stop_wall_timer(tm);

	return best_gain;
}

int rewrite_manager_t::count_added_nodes(truth_t rtt, truth_t dtt, alit dgal) {

	if (canonTabs[curcc].is_var) {
		//std::cout << "is primitive var\n";
		return 0;
	}

	int added_nodes = 0;

	// get permutation
	bool outmask;
	std::bitset<4> inmasks;
	std::array<int, 4> inperms;
	get_relative_perms(rtt, dtt, inmasks, inperms, outmask);

	alit nal; bool invert = false;

	//std::cout << "counting added for " << dgal << "\n";
	const idvec& tsorted_dagnids = dagsorts.at(dgal.lid());

	//std::cout << "adding for dag: " << dgal << " " << rwrdag.at(dgal.lid()).node_size << "\n";

	for (id nid : tsorted_dagnids) {
		auto& dagnd = rwrdag.at(nid);

		if (dagnd.input_index != -1) {
			int i = dagnd.input_index;
			if (dagnd.input_index == 4) {
				if (!ntk->has_const0())
					added_nodes++;
				dagnd.mpnd = ntk->get_const0(); //^ masks[i];
			}
			else {
				int p = inperms[i];
				bool cm = inmasks[i];
				dagnd.mpnd = alit(curcut_invec[p], cm);
			}
		}
		else {
			alit in0 = rwrdag.at(dagnd.a0.lid()).mpnd ^ dagnd.a0.sgn();
			alit in1 = rwrdag.at(dagnd.a1.lid()).mpnd ^ dagnd.a1.sgn();

			bool strashed;
			dagnd.mpnd = ntk->lookup_and(in0, in1, strashed);
			if (dagnd.mpnd == curcut->root) {
				//std::cout << "strashed to root while counting " << curcut->root << "\n";
				return -1;
			}

			if (dagnd.mpnd == -1 || travidvec[dagnd.mpnd.lid()] == curtid) {
				//std::cout << "added node " << dagnd.mpnd << "  " << (travidvec[dagnd.mpnd.lid()] == curtid) << "  " << curtid << "\n";
				added_nodes++;
			}
		}
	}

	//std::cout << "added nodes : " << added_nodes << "\n";

	return added_nodes;

}

int rewrite_manager_t::rewrite_node(id aid) {

	assert(ntk->node_exists(aid) && ntk->isAnd(aid));

	cutvec_t cuts;
	cut_enumerate(*ntk, aid, cuts, 4);

	int best_cutgain = std::numeric_limits<int>::min();
	for (const auto& cut : cuts) {
		if (cut.inputs.size() == 4) {
			pst.cuts_analyzed++;

			auto tm = _start_wall_timer();

			curcut = &cut;
			curcut_invec = to_vec(cut.inputs);

			if (ntk->nfanouts(cut.root).size() > 1000)
				continue;

			/*int counter = 0;
			for (auto x : curcut_invec) {
				if (!ntk->node_exists(x)) {
					neos_error("cut input " << x << " doesnt exist");
				}
				if (ntk->nfanouts(x).size() == 1)
					counter++;
			}*/
			/*if (counter > 2)
				return 0;*/

			simmap[curcut_invec[0]] = 0xAAAA;
			simmap[curcut_invec[1]] = 0xCCCC;
			simmap[curcut_invec[2]] = 0xF0F0;
			simmap[curcut_invec[3]] = 0xFF00;

			if (ntk->has_const0()) {
				simmap[ntk->get_cconst0id()] = 0x0000;
			}

			curcut_aorder.clear();
			topsort_cut(*ntk, cut, curcut_aorder);

			truth_t rtt = simulate_cut(*ntk);
			curcc = allTabs[rtt].canon_class;

			if (!canonTabs[curcc].is_prac)
				continue;
			// std::cout << "rtt is " << std::bitset<16>(rtt) << "\n";

			auto tm0 = _start_wall_timer();

			truth_t best_tt;
			int cutgain = eval_gain(rtt, best_tt);

			//std::cout << "node " << curcut->root << " gain was: " << cutgain << "\n";
			// zero replacement enabled
			if (cutgain >= replacement_gain_threshold) {

				//std::cout << "cut replacement \n";

				if (random_replacement) {
					if (cutgain == best_cutgain) {
						bestcut_vec.push_back(curcut);
						bestcut_rtt_vec.push_back(rtt);
						bestcut_ctt_vec.push_back(best_tt);
						//std::cout << "found random cut " << bestcut_vec.size() << "\n";
					}
					else if (cutgain > best_cutgain) {
						best_cutgain = cutgain;
						bestcut_vec = {curcut};
						bestcut_rtt_vec = {rtt};
						bestcut_ctt_vec = {best_tt};
					}
				}
				else if (cutgain > best_cutgain) {
					//std::cout << "cut replacement second\n";
					best_cutgain = cutgain;
					bestcut = curcut;
					//bestcut_aorder = curcut_aorder;
					//bestcut_invec = curcut_invec;
					bestcut_rtt = rtt;
					bestcut_ctt = best_tt;
				}

			}
		}
	}

	if (best_cutgain != std::numeric_limits<int>::min()) {
		// std::cout << "replacing cut. ";
		//std::cout << "final best gain : " << best_gain << "\n";
		//std::cout << "egain is " << egain << "\n";

		if (random_replacement) {
			/*std::cout << "performing random replacement among "
					<< bestcut_rtt_vec.size() << "\n";*/
			size_t index = rand() % bestcut_rtt_vec.size();
			bestcut_rtt = bestcut_rtt_vec[index];
			bestcut_ctt = bestcut_ctt_vec[index];
			bestcut = bestcut_vec[index];
		}

		alit nal; bool invert;
		move_to_dag(bestcut_rtt, bestcut_ctt, *bestcut, nal, invert);

		//assert(gain == egain);
		// std::cout << " gain: " << gain << "\n";
		return 1;
	}


	return 0;
}

int rewrite_manager_t::count_unused_cone(const aig_t& net, id root) {

	assert(net.nfanouts(root).empty());

	auto tm = utl::_start_wall_timer();

	int unused = 0;
	idque Q;
	Q.push(root);
	Omap<id, int> refcount;
	refcount[root] = 1;

	while ( !Q.empty() ) {

		id curid = Q.front(); Q.pop();
		const auto& curnd = net.getcNode(curid);

		//std::cout << " curnd " << curid << " " << curnd.fanouts.size() << "\n";

		if (_is_not_in(curid, refcount)) {
			refcount[curid] = curnd.fanouts.size();
		}

		if (--refcount.at(curid) <= 0) {
			//std::cout << "unused " << curid << "\n";
			unused++;
			for ( auto inid : {curnd.fi0(), curnd.fi1()} ) {
				if (inid != -1) {
					//std::cout << "in : " << net.ndname(inid) << "\n";
					Q.push(inid);
				}
			}
		}
	}

	pst.count_unused_time += utl::_stop_wall_timer(tm);

	return unused;

}

void rewrite_manager_t::print_stats() {
	std::cout << "cuts analyzed: " << pst.cuts_analyzed << "\n";
	std::cout << "cuts replaced: " << pst.replaced << "\n";
	std::cout << "cut time: " << pst.cut_time << "\n";
	std::cout << "npn time: " << pst.npn_time << "\n";
	std::cout << "sim time: " << pst.sim_time << "\n";
	std::cout << "dag tsort time: " << pst.dag_tsort_time << "\n";
	std::cout << "tsort time: " << pst.tsort_time << "\n";
	std::cout << "cut sort time: " << pst.cut_sort_time << "\n";
	std::cout << "count time: " << pst.count_unused_time << "\n";
	std::cout << "move time: " << pst.move_time << "\n";
	std::cout << "eval time: " << pst.eval_time << "\n";
	std::cout << "network time: " << pst.network_mod_time << "\n";
	std::cout << "total time: " << pst.total_time << "\n";
	std::cout << "num nodes: " << pst.final_nodes << "/" << pst.original_nodes << " ("
			 << std::setprecision(2) << ((float)(pst.original_nodes - pst.final_nodes) / pst.original_nodes * 100) << "%)\n";
}

} // namespace aig_ns




