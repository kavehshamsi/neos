/*
 * circuit_words.h
 *
 *  Created on: Jan 13, 2021
 *      Author: kaveh
 */

#ifndef SRC_BASE_CIRCUIT_WORDS_H_
#define SRC_BASE_CIRCUIT_WORDS_H_

#include "base/circuit.h"
#include "base/blocks.h"

namespace ckt {

class cir_word {
public:
	circuit* cir = nullptr;
	idvec wordids;
public:

	cir_word() = delete;

	cir_word(circuit& incir) : cir(&incir) {}

	cir_word(circuit& incir, int num_bits, wtype wt, const string& basename = "") {
		cir = &incir;
		for (int i = 0; i < num_bits; i++) {
			if (basename == "")
				wordids.push_back(incir.add_wire(wt));
			else
				wordids.push_back(incir.add_wire(basename + std::to_string(i), wt));
		}
	}

	template<typename S>
	cir_word(circuit& incir, const S& idcont) {
		cir = &incir;
		for (auto xid : idcont) {
			wordids.push_back(xid);
		}
	}

	cir_word sub(size_t l, size_t h) {
		return cir_word(*cir, idvec(wordids.begin() + l, wordids.begin() + h));
	}

	id operator[](size_t i) const { return wordids[i]; }
	size_t size() const { return wordids.size(); }

	void setwiretype(wtype wt) {
		for (auto xid : wordids) {
			cir->setwiretype(xid, wt);
		}
	}

	friend std::ostream& operator<<(std::ostream& ost, const cir_word& a);
};

inline std::ostream& operator<<(std::ostream& ost, const cir_word& a) {
	ost << "{" << a.cir->to_wstr(a.wordids, ", ") << "}";
	return ost;
}

cir_word reduce_op(const cir_word& x, fnct fn);
cir_word apply_op(const vector<cir_word>& ins, fnct fn);

cir_word operator!(const cir_word& a);
cir_word operator~(const cir_word& a);

cir_word operator&(const cir_word& a, const cir_word& b);
cir_word operator^(const cir_word& a, const cir_word& b);
cir_word operator|(const cir_word& a, const cir_word& b);
cir_word operator+(const cir_word& a, const cir_word& b);
cir_word operator==(const cir_word& a, const cir_word& b);
cir_word operator>(const cir_word& a, const cir_word& b);
cir_word operator<(const cir_word& a, const cir_word& b);
cir_word operator>=(const cir_word& a, const cir_word& b);
cir_word operator<=(const cir_word& a, const cir_word& b);
cir_word concat(const cir_word& a, const cir_word& b);

cir_word operator+(cir_word& a);
cir_word reduce_sum(const cir_word& a);

}


#endif /* SRC_BASE_CIRCUIT_WORDS_H_ */
