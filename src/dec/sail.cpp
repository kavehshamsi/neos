/*
 * sail.cpp
 *
 *  Created on: Oct 4, 2018
 *      Author: kaveh
 */

#include "dec/sail.h"

namespace misc {

void sail(const circuit& sim_ckt, int num_keys) {

	circuit enc_ckt = sim_ckt;

	std::cout << "num keys is : " << num_keys << "\n";

	enc::encrypt::enc_xor_rand(enc_ckt, utl::random_boolvec(num_keys));

	//enc_ckt.write_bench();

	std::map<string, int> kname2xor;
	foreach_keyid(enc_ckt, kid) {
		string kname = enc_ckt.wname(kid);
		id gid = *enc_ckt.wfanout(kid).begin();
		if (enc_ckt.gfunction(gid) == fnct::XOR) {
			kname2xor[kname] = 1;
		}
		else {
			kname2xor[kname] = 0;
		}

	}

	ext::abc_resynthesize(enc_ckt);

	int type1 = 0, type2 = 0, type3 = 0;

	foreach_keyid(enc_ckt, kid) {
		id gid = *enc_ckt.wfanout(kid).begin();
		auto gfun = enc_ckt.gfunction(gid);
		if (!enc_ckt.gfanoutGates(gid).empty()) {
			id gid1 = *enc_ckt.gfanoutGates(gid).begin();
			auto gfun1 = enc_ckt.gfunction(gid1);
			//std::cout << enc_ckt.wname(kid) << " -> " << funct::enum_to_string(gfun) << " -> " << funct::enum_to_string(gfun1) << "\n";
		}
		else {
			//std::cout << enc_ckt.wname(kid) << " -> " << funct::enum_to_string(gfun) << "\n";
		}
		if (gfun == fnct::XOR || gfun == fnct::XNOR) {
			if ((kname2xor.at(enc_ckt.wname(kid)) == 1) ^ (gfun == fnct::XOR)) {
				type2++;
			}
			else {
				type1++;
			}
		}
		else {
			type3++;
		}
	}

	std::cout << "\ntype1: " << type1 << " " << (float)type1/num_keys * 100 << " "
			<< "  type2: " << type2 << " " << (float)type2/num_keys * 100 << " "
			<< "  type3: " << type3 << " " << (float)type3/num_keys * 100 << " " << "\n";
}

} // namespace misc
