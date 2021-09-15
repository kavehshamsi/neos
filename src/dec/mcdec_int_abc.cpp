/*
 * mc.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: kaveh
 */

#include <utl/ext_cmd.h>
#include "dec/mcdec.h"


namespace dec_ns {

using namespace ext;

mcdec_int_abc::mcdec_int_abc(circuit& sim_ckt, circuit& enc_cir, const boolvec& corrkey) :
		mcdec_bmc_int(sim_ckt, enc_cir, corrkey) {
	temp_dir = ext::create_temp_dir();
}

mcdec_int_abc::~mcdec_int_abc() {
	ext::remove_temp_dir();
}

void mcdec_int_abc::_build_mitter_int() {

	std::cout << "abc build_mitter_int\n";
	mcdec_bmc_int::_build_mitter_int();

	for (auto kid : mitt_ckt.keys()) {
		string kname = mitt_ckt.wname(kid);
		if ( _is_primed_name(kname) ) { // k1 and k2 maps
			id enckid = enc_cir->find_wcheck( _unprime_name(kname) );
			enc2mitt_kmap[1][enckid] = kid;
		}
		else {
			id enckid = enc_cir->find_wcheck(kname);
			enc2mitt_kmap[0][enckid] = kid;
		}
	}

	for (auto xid : mitt_ckt.inputs()) {
		if ( !_is_in("_$io", mitt_ckt.wname(xid)) ) {
			id encxid = enc_cir->find_wcheck(mitt_ckt.wname(xid));
			enc2mitt_xmap[encxid] = xid;
		}
		else {
			auto nm = mitt_ckt.wname(xid);
			string oname = nm.substr(0, nm.find("_$io"));
			id encyid = enc_cir->find_wcheck(oname);
			enc2io_ymap[encyid] = xid;
		}
	}

	V1(cout << "mitter circuit\n";
		mitt_ckt.write_bench());

	mitt_cir_file = temp_dir + "mitt_cir.bench";
	bmc_cir = mitt_ckt;
	bmc_cir.trim_circuit_to({mitt_out});
	mc::make_frozen_with_latch(bmc_cir, bmc_cir.keys());
	bmc_cir.write_bench();

	mitt_cunr.set_tr(mitt_ckt, mitt_ckt.keys(), false);
}

void mcdec_int_abc::_exit_with_cleanup() {
	ext::remove_temp_dir();
	exit(EXIT_SUCCESS);
}

void mcdec_int_abc::read_mc_file(const string& bmc_log, int& retstat, int& fr) {

	std::ifstream fin;
	fin.open(bmc_log, std::ios::in);
	string l1;
	assert(fin.is_open());
	fr = -1;
	while ( getline(fin, l1) ) {
		if ( _is_in("was asserted", l1) ) {
			retstat = 1;
			uint ps1 = l1.find("in frame ");
			auto x = l1.substr(ps1, l1.size());
			sscanf(x.c_str(), "in frame %d", &fr);
			std::cout << "frame is : " << fr  << "\n";
			/*uint ps2 = ps1;
			while (l1[ps2++] != '.' && ps2 < l1.size());
			string nm = l1.substr(ps1 + 9, ps2);
			std::cout << "frame is : " << nm;*/
			return;
		}
		//std::cout << "line: " << line << "\n";
	}

	fin.close();
	retstat = 0;
}

void mcdec_int_abc::read_cex_file(const string& cex_file, dis_t& dis, int f) {
	std::ifstream fin;
	fin.open(cex_file, std::ios::in);
	string l1;
	assert(fin.is_open());

	std::cout << "cex len is " << f << "\n";
	dis.xs = std::vector<boolvec>(f + 1, boolvec(num_ins));

	while ( getline(fin, l1) ) {
		std::cout << "cex line: " << l1 << "\n";
		string l2;
		std::stringstream ss(l1);
		while ( getline(ss, l2, ' ') ) {
			auto ps1 = l2.find('@');
			auto ps2 = l2.find('=');
			string nm = l2.substr(0, ps1);
			int fr = stoi(l2.substr(ps1 + 1, ps2 - (ps1 + 1)));
			int v = stoi(l2.substr(ps2 + 1, l2.size()));
			// std::cout << l2 << "\n";
			// std::cout << "nm: " << nm << " fr:" << fr << " v:" << v << "\n";
			id encid = enc_cir->find_wire(nm);
			if (!_is_in("keyinput", nm) && encid != -1 && enc_cir->isInput(encid)) {
				std::cout << "mapping " << nm << "->" << v << " at " << fr << "\n";
				dis.xs.at(fr).at(ewid2ind.at(encid)) = v;
			}
		}
	}
	fin.close();
}

int mcdec_int_abc::_solve_for_dis_int(dis_t& dis) {

	std::cout << "abc solve_for_dis_int\n\n";

	//bmc_cir.write_bench();
	bmc_cir.write_bench(mitt_cir_file);

	string cex_file = temp_dir + "cex.txt";
	string bmc_logfile = temp_dir + "output.log";
	string bmc_script = "read_bench " + mitt_cir_file
			+ "; strash; zero;"
			+ "; bmc -F " + std::to_string(bmc_len)
			+ "; write_cex -a -n " + cex_file + "; quit;";

	ext::abc_run_commands(bmc_script, temp_dir);

	int fr = -1, retstat = -1;
	read_mc_file(bmc_logfile, retstat, fr);

	if (retstat == 0) { // no counter-example
		return retstat;
	}

	read_cex_file(cex_file, dis, fr);
	return retstat;
}


void mcdec_int_abc::_add_dis_constraint_int(dis_t& dis) {

	circuit unrollediockt;
	_get_unrolled_dis_cir(dis, unrollediockt);

	cout << "\n\nUNROLLED CKT after tying:\n\n";
	//unrollediockt.write_bench();

	if (kc_sweep == 0) { // no sweeping simply add to mitter
		bmc_cir.add_circuit_byname(unrollediockt, "");
		bmc_cir.join_outputs(fnct::AND, "kcmittOut");
		//bmc_cir.write_bench();

		//_exit_with_cleanup();
	}
	else if (kc_sweep == 1) {
		kc_ckt.add_circuit_byname(unrollediockt, "");
		kc_ckt.join_outputs(fnct::AND, kc_ckt.get_new_wname());
	}
	else if (kc_sweep > 1) {

	}

}

void mcdec_int_abc::_get_unrolled_dis_cir(dis_t& dis, circuit& unrollediockt) {

	if (dis.length() > (uint)mitt_cunr.cur_len()) {

		std::cout << "iockt extending\n";
		std::cout << "dis len : " << dis.length() << " mitt unrolling frame : " << mitt_cunr.cur_len() << "\n";
		mitt_cunr.extend_to(dis.length());

	}

	// trim to ioc_outs
	circuit ioc_trimed = mitt_cunr.get_zeroinit_uncir();
	idset io_outs;
	for (uint i = 0; i < dis.length(); i++) {
		io_outs.insert(ioc_trimed.find_wcheck(_frame_name("iocOut", i)));
	}
	ioc_trimed.trim_circuit_to(io_outs);
	unrollediockt = ioc_trimed;

	D2(cout << "\n\nUNROLLED CKT:\n\n";
	unrollediockt.write_bench();
	unrollediockt.error_check();)

	// tie inputs
	id2boolHmap inputmap;
	int f = 0;
	for (auto xid : unrollediockt.inputs()) {
		string inname = unrollediockt.wname(xid);
		f = _unframe_name(inname).second;
		if (_is_in("_$io", inname)) {
			string enc_name = inname.substr(0, inname.find("_$io"));
			// std::cout << "enc_name: " << enc_name << "\n";
			int enc_index = ewid2ind.at(enc_cir->find_wcheck(enc_name));
			// cout << unrollediockt.wname(xid) << " : " << dis.ys[f][enc_index] << " frame: " << f << "\n";
			// cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
			inputmap[xid] = dis.ys.at(f).at(enc_index);
		}
		else {
			string enc_name = inname.substr(0, inname.find("_"));
			int enc_index = ewid2ind.at(enc_cir->find_wcheck(enc_name));
			// cout << unrollediockt.wname(xid) << " : " << dis.xs[f][enc_index] << " frame: " << f << "\n";
			// cout << unrollediockt.wname(xid) << " : " << enc_cir->wname(ewid2ind.ta(enc_index)) << "\n";
			inputmap[xid] = dis.xs.at(f).at(enc_index);
		}
	}

	// propagate constants
	unrollediockt.propagate_constants(inputmap);

	// join outputs
	unrollediockt.join_outputs(fnct::AND, "ioend_all" + std::to_string(iter));

	D1( assert(unrollediockt.error_check()); )

}

int mcdec_int_abc::_check_termination_int() {

	std::cout << "abc termination check\n";

	return 0;
}

void mcdec_int_abc::extract_key_int() {

}

}
