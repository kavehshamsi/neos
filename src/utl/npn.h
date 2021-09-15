/*
 * npn.h
 *
 *  Created on: Jun 29, 2020
 *      Author: kaveh
 *      Description: header class for NPN boolean truth-table
 *      manipulation/exploration using static std::bitset<N>
 */

#ifndef SRC_UTL_NPN_H_
#define SRC_UTL_NPN_H_

#include <vector>
#include <array>
#include <bitset>
#include <type_traits>
#include <iostream>
#include <cassert>
#include <math.h>

#include <boost/dynamic_bitset.hpp>

namespace utl_npn {

// template meta-programming magic for pow/factorial
template<int N, int M>
struct Pow {
    enum { res = N * Pow<N,M-1>::res};
};

template<int N>
struct Pow<N,0> {
    enum {res = 1};
};

template <int N>
struct Factorial {
    enum { value = N * Factorial<N - 1>::value };
};

template <>
struct Factorial<0> {
    enum { value = 1 };
};

template <int N, int M>
struct Permutations {
    enum { value = Factorial<N>::value / Factorial<N - M>::value };
};

inline size_t factorial(int n) {
	size_t factorial = 1;
    for(int i = 1; i <= n; i++) {
        factorial *= i;
    }
    return factorial;
}

using std::vector;

// NV num variables
template<int NV>
class npn_enumerator {
private:

	static_assert(NV < 7);

	// truth-table length
	static constexpr int NTT = Pow<2, NV>::res;
	// num variable permutations
	static constexpr int NP = Factorial<NV>::value;
	// num negations 2^n
	static constexpr int NN = Pow<2, NV>::res;

	// truth-table permutations
	typedef std::array<int, NTT> tt_trans_t;
	// list of permutation truth-table transformation
	typedef vector<tt_trans_t> perms_list_t;
	// list of negation truth-table transformation
	typedef vector<tt_trans_t> negs_list_t;
	// variable permutation transformation
	typedef std::array<int, NV> var_perm_t;
	// list of varibale permutation transformations
	typedef vector<var_perm_t> var_perms_t;

public:
	// truth-table type
	typedef std::bitset<NTT> truth_t;

private:
	var_perms_t varperms;
	perms_list_t tt_perms;
	negs_list_t tt_negs;

	truth_t rtt;
	bool initialized = false;
public:

	struct npn_indices {
		int flip = 0;
		int pInd = 0;
		int nInd = 0;
	};

	npn_indices ix, lix;
	npn_enumerator(int init = 0) { if (init) build_perm_negs(); }

	truth_t init(truth_t xtt) {
		assert(initialized);
	    rtt = xtt;
	    ix = lix = {0, 0, 0};
	    return rtt;
	}

	// must be called before any enumeration usage
	void build_perm_negs() {
	    if (initialized)
	        return;
		// build all permutations
		build_tt_perms();
		build_tt_negs();
		initialized = true;
	}

	void build_tt_perms() {
		varperms.reserve(NP);
		tt_perms.reserve(NP);
		var_perm_t arr;
		for (uint i = 0; i < NV; i++) arr[i] = i;

		uint per = 0;
		do {
			varperms.push_back(arr);
			tt_perms.push_back(tt_trans_t());
			//std::cout << per << " : " << to_str(arr) << "\n";
			for (uint i = 0; i < NTT; i++) {
				std::bitset<NV> bs = i;
				int num = 0;
				for (int x = 0; x < NV; x++) {
					num += (bs[x] << arr[x]);
				}
				tt_perms[per][i] = num;
				//std::cout << "  " << num << "\n";
			}
			per++;
		}
		while (std::next_permutation(arr.begin(), arr.end()));
	}

	void build_tt_negs() {

		tt_negs.reserve(NN);
		for (uint i = 0; i < NTT; i++) {
			/*std::bitset<4> bs = i;
			std::cout << "for " << bs << "\n";*/
			tt_negs.push_back(tt_trans_t());
			for (uint j = 0; j < NTT; j++) {
				uint npos = j ^ i;
				tt_negs[i][j] = npos;
				//std::cout << "  " << npos << "\n";
			}
		}
	}

	inline static truth_t transform(truth_t tt, const tt_trans_t& arr) {
		truth_t ret;
		for (int i = 0; i < NTT; i++) {
			ret[i] = tt[arr[i]];
		}
		return ret;
	}

	inline static truth_t i_transform(truth_t tt, const tt_trans_t& arr) {
		auto arri = arr;
		for (int i = 0; i < NTT; i++) {
			arri[arr[i]] = i;
		}
		return transform(tt, arri);
	}

	inline truth_t negpermute_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = transform(transform(tt, tt_negs[x.nInd]), tt_perms[x.pInd]);
		if (x.flip)
			ntt.flip();
		return ntt;
	}

	inline truth_t permnegate_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = transform(i_transform(tt, tt_perms[x.pInd]), tt_negs[x.nInd]);
		if (x.flip)
			ntt.flip();
		return ntt;
	}

	bool next_neg(truth_t& ntt) {
		ntt = negpermute_table(rtt, ix);
		lix = ix;

		if (++ix.nInd == NN)
			return false;

		return true;
	}

	bool next_perm(truth_t& ntt) {
		ntt = negpermute_table(rtt, ix);
		lix = ix;

		if (++ix.pInd == NP)
			return false;

		return true;
	}


	bool next_npn(truth_t& ntt) {
		ntt = negpermute_table(rtt, ix);
		lix = ix;
		if (++ix.flip == 2) {
			ix.flip = 0;
			if (++ix.nInd == NN) {
				ix.nInd = 0;
				++ix.pInd;
			}
		}

		if (ix.pInd == NP && ix.nInd == 0 && ix.flip == 0)
			return false;

		return true;
	}

};


// dynamic npn_enumerator
class dyn_npn_enumerator {
private:

	// truth-table permutations
	typedef vector<int> tt_trans_t;
	// list of permutation truth-table transformation
	typedef vector<tt_trans_t> perms_list_t;
	// list of negation truth-table transformation
	typedef vector<tt_trans_t> negs_list_t;
	// variable permutation transformation
	typedef vector<int> var_perm_t;
	// list of varibale permutation transformations
	typedef vector<var_perm_t> var_perms_t;

	typedef boost::dynamic_bitset<> dbitset;

public:

	// num variables
	int NV;
	// truth-table length
	int NTT;
	// num variable permutations
	int NP;
	// num negations 2^n
	int NN;


	// truth-table type
	typedef boost::dynamic_bitset<> truth_t;

	var_perms_t varperms;
	perms_list_t tt_perms;
	negs_list_t tt_negs;

private:
	truth_t rtt;
	bool initialized = false;
public:

	struct npn_indices {
		int flip = 0;
		int pInd = 0;
		int nInd = 0;
	};

	npn_indices ix, lix;
	dyn_npn_enumerator(int NV, int init = 0) : NV(NV), NTT(std::pow(2, NV)),
			NP( factorial(NV) ), NN(NTT) {
		std::cout << "NV " << NV << "\n";
		std::cout << "NP " << NP << "\n";
		std::cout << "NTT " << NTT << "\n";
		if (init)
		    build_perm_negs();
    }

	truth_t init(uint x = 0) {
		truth_t xtt(NV, x);
		return init(xtt);
	}

	truth_t init(truth_t xtt) {
		assert(xtt.size() == NTT);
		assert(initialized);
	    rtt = xtt;
	    ix = lix = {0, 0, 0};
	    return rtt;
	}

	// must be called before any enumeration usage
	void build_perm_negs() {
	    if (initialized)
	        return;
	    // std::cout << "building permutations\n";
		// build all permutations
		build_tt_perms();
		build_tt_negs();
		initialized = true;
		/*std::cout << varperms.size() << "\n";
		std::cout << tt_perms.size() << "\n";
		std::cout << tt_negs.size() << "\n";*/
	}

	void build_tt_perms() {
		varperms.reserve(NP);
		tt_perms.reserve(NP);
		var_perm_t arr(NV);
		for (int i = 0; i < NV; i++) arr[i] = i;

		uint per = 0;
		do {
			varperms.push_back(arr);
			tt_perms.push_back(tt_trans_t(NTT));
			//std::cout << per << " : " << to_str(arr) << "\n";
			for (int i = 0; i < NTT; i++) {
				dbitset bs(NV, i);
				int num = 0;
				for (int x = 0; x < NV; x++) {
					num += (bs[x] << arr[x]);
				}
				tt_perms[per][i] = num;
				//std::cout << "  " << num << "\n";
			}
			per++;
		}
		while (std::next_permutation(arr.begin(), arr.end()));

		assert(varperms.size() == NP);
		assert(tt_perms.size() == NP);
	}

	void build_tt_negs() {

		tt_negs.reserve(NN);
		for (uint i = 0; i < NN; i++) {
			/*std::bitset<4> bs = i;
			std::cout << "for " << bs << "\n";*/
			tt_negs.push_back(tt_trans_t(NTT));
			for (uint j = 0; j < NTT; j++) {
				uint npos = j ^ i;
				tt_negs[i][j] = npos;
				//std::cout << "  " << npos << "\n";
			}
		}
	}

	inline truth_t transform(truth_t tt, const tt_trans_t& arr) {
		truth_t ret(NTT);
		for (int i = 0; i < NTT; i++) {
			ret[i] = tt[arr[i]];
		}
		return ret;
	}

	inline truth_t i_transform(truth_t tt, const tt_trans_t& arr) {
		auto arri = arr;
		for (int i = 0; i < NTT; i++) {
			arri[arr[i]] = i;
		}
		return transform(tt, arri);
	}

	inline truth_t negpermute_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = transform(transform(tt, tt_negs[x.nInd]), tt_perms[x.pInd]);
		if (x.flip)
			ntt.flip();
		return ntt;
	}

	inline truth_t permnegate_table(truth_t tt, const npn_indices& x) {
		truth_t ntt = transform(i_transform(tt, tt_perms[x.pInd]), tt_negs[x.nInd]);
		if (x.flip)
			ntt.flip();
		return ntt;
	}

	bool next_neg(truth_t& ntt) {
		ntt = negpermute_table(rtt, ix);
		lix = ix;

		if (++ix.nInd == NN)
			return false;

		return true;
	}

	bool next_perm(truth_t& ntt) {
		ntt = negpermute_table(rtt, ix);
		lix = ix;

		if (++ix.pInd == NP)
			return false;

		return true;
	}


	bool next_npn(truth_t& ntt) {
		// std::cout << "{ " << ix.flip << " " << ix.nInd << " " << ix.pInd << "}\n";
		ntt = negpermute_table(rtt, ix);
		lix = ix;
		if (++ix.flip == 2) {
			ix.flip = 0;
			if (++ix.nInd == NN) {
				ix.nInd = 0;
				++ix.pInd;
			}
		}

		if (ix.pInd == NP && ix.nInd == 0 && ix.flip == 0)
			return false;

		return true;
	}

};


} // namespace utl_npn


/*
#include <unordered_set>


int main () {

    using namespace utl_npn;

    npn_enumerator<4> npt(1);
    using truth_t = npn_enumerator<4>::truth_t;
    uint16_t x = 2;
    npt.init(x);
    uint i = 0;
    for (truth_t ntt = npt.init(x); npt.next_perm(ntt); ) {
        std::cout << ntt << "  ";
        std::cout << ++i << "\n";
    }

    npt.init(x);
    std::unordered_set<npn_enumerator<4>::truth_t> hset;
    uint32_t npn_class = 0;
    for (uint16_t x = 0; x != 1 << 15; x++) {
        if (hset.find(truth_t(x)) == hset.end()) {
            std::cout << "found new table " << ++npn_class << "\n";
            for (npn_enumerator<4>::truth_t ntt = npt.init(x); npt.next_npn(ntt); ) {
                hset.insert(ntt);
            }
        }
    }



    dyn_npn_enumerator dnpt(4, 1);
    using dtruth_t = dyn_npn_enumerator::truth_t;
    x = 2;
    dnpt.init(dtruth_t(4, x));
    i = 0;
    for (dtruth_t ntt = dnpt.init(dtruth_t(4, x)); dnpt.next_perm(ntt); ) {
        std::cout << ntt << "  ";
        std::cout << ++i << "\n";
    }

    dnpt.init(dtruth_t(4, x));
    std::unordered_set<dtruth_t> dhset;
    npn_class = 0;
    for (uint16_t x = 0; x != 1 << 15; x++) {
        if (dhset.find(dtruth_t(4, x)) == dhset.end()) {
            std::cout << "found new table " << ++npn_class << "\n";
            for (dyn_npn_enumerator::truth_t ntt = dnpt.init(dtruth_t(4, x)); dnpt.next_npn(ntt); ) {
                dhset.insert(ntt);
            }
        }
    }
}
*/



#endif /* SRC_UTL_NPN_H_ */
