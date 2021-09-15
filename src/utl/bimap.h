/*
 * bimap.h
 *
 *  Created on: Oct 24, 2019
 *      Author: kaveh
 */

#ifndef SRC_UTL_BIMAP_H_
#define SRC_UTL_BIMAP_H_

#include <unordered_map>
#include <map>
#include <cassert>

namespace utl {

/*
* Bimap : has two maps for creating mapping from a set of pairs
* in both directions. TODO: This is the worst bimap implementation ever.
*/
template<typename F, typename S, template<typename...> class M = std::map>
class bimap {
private:
	M<F, S> f2s_map;
	M<S, F> s2f_map;

public:

	bimap() {}

	// construct from uni-map
	bimap(M<F, S>& umap) {
		for (auto& p : umap) {
			add_pair(p.first, p.second);
		}
	}

	uint size() {
		assert(f2s_map.size() == s2f_map.size());
		return f2s_map.size();
	}

	const S& add_pair(const F& f, const S& s) {
		const auto& sref = f2s_map[f] = s;
		s2f_map[s] = f;
		return sref;
	}

	void remove_byfirst(const F& f) {
		auto it = f2s_map.find(f);
		if ( it != f2s_map.end() ) {
			f2s_map.erase(it);
			s2f_map.erase(it->second);
		}
	}

	void remove_bysecocnd(const S& s) {
		auto it = s2f_map.find(s);
		if ( it != s2f_map.end() ) {
			s2f_map.erase(it);
			f2s_map.erase(it->second);
		}
	}

	void remove_pair(const F& f, const S& s) {
		auto itf = f2s_map.find(f);
		auto its = s2f_map.find(s);
		if (itf != f2s_map.end() && its != s2f_map.end()) {
			f2s_map.erase(itf);
			s2f_map.erase(its);
		}
	}

	const S& at(const F& f) const {
		return f2s_map.at(f);
	}

	const F& ta(const S& s) const {
		return s2f_map.at(s);
	}

	S& at(const F& f) {
		return f2s_map.at(f);
	}

	F& ta(const S& s) {
		return s2f_map.at(s);
	}

	bool is_in_firsts(const F& f, const S* sptr = nullptr) const {
		auto it =  f2s_map.find(f);
		if (it != f2s_map.end()) {
			if (sptr) sptr = &(it->second);
			return true;
		}
		return false;
	}

	bool is_in_seconds(const S& s, const F* fptr = nullptr) const {
		auto it =  s2f_map.find(s);
		if (it != s2f_map.end()) {
			if (fptr) fptr = &(it->second);
			return true;
		}
		return false;
	}

	// get map
	M<F, S>& getmap() {
		return f2s_map;
	}

	// get inverse-map
	M<S, F>& getImap() {
		return s2f_map;
	}

	// get map
	const M<F, S>& getmap() const {
		return f2s_map;
	}

	// get inverse-map
	const M<S, F>& getImap() const {
		return s2f_map;
	}

	void clear() {
		f2s_map.clear();
		s2f_map.clear();
	}
};

template <typename F, typename S>
using Hbimap = bimap<F, S, std::unordered_map>;

template <typename F, typename S>
using Obimap = bimap<F, S, std::map>;

} // namespace utl

#endif /* SRC_UTL_BIMAP_H_ */
