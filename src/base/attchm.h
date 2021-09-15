/*
 * attchm.h
 *
 *  Created on: Jan 24, 2021
 *      Author: kaveh
 */

#ifndef SRC_BASE_ATTCHM_H_
#define SRC_BASE_ATTCHM_H_

#include "base/base.h"
#include <any>

namespace ckt {

struct attchm_mgr_t {
	std::any mp;

	struct attchm_map_t {
		Hmap<string, id> name2id;
		vector<std::any> attchvec;
	};

	bool empty() const { return !mp.has_value(); }

	const attchm_map_t& getcmap() const {
		assert(mp.has_value());
		return std::any_cast<const attchm_map_t&>(mp);
	}
	attchm_map_t& getmap() {
		assert(mp.has_value());
		return std::any_cast<attchm_map_t&>(mp);
	}

	template<typename T>
	id add_attachment(const std::string& nm, const T& attch) {
		if (empty()) {
			mp = attchm_map_t();
		}
		auto& amp = getmap();
		id aid = get_attachment_id(nm);
		id naid = add_attachment(aid, attch);
		if (aid == -1) {
			amp.name2id[nm] = naid;
		}
		return naid;
	}

	template<typename T>
	id add_attachment(id aid, const T& attch) {
		auto& amp = getmap();
		if (aid == -1) {
			amp.attchvec.push_back(attch);
			aid = amp.attchvec.size() - 1;
		}
		else { // will overwrite old attachmnet
			amp.attchvec.at(aid) = attch;
		}
		return aid;
	}

	id get_attachment_id(const std::string& nm) const {
		if (empty()) return -1;
		auto& amp = getcmap();
		auto it = amp.name2id.find(nm);
		if (it == amp.name2id.end())
			return -1;
		return it->second;
	}

	template<typename T>
	const T& get_cattatchment(const std::string& nm) const {
		auto& amp = getcmap();
		auto it = amp.name2id.find(nm);
		if (it == amp.name2id.end()) {
			throw std::out_of_range("attchment " + nm + " not found");
		}
		else {
			return get_cattatchment<T>(it->second);
		}
	}
	template<typename T>
	const T& get_cattatchment(id aid) const {
		const auto& amp = getcmap();
		return std::any_cast<const T&>(amp.attchvec.at(aid));
	}

	template<typename T>
	T& get_attatchment(const std::string& nm, id index = 0) {
		auto& amp = getmap();
		auto it = amp.name2id.find(nm);
		if (it == amp.name2id.end()) {
			throw std::out_of_range("attchment not found");
		}
		else {
			return std::any_cast<T&>(amp.attchvec.at(it->second));
		}
	}
	template<typename T>
	T& get_attatchment(id aid) {
		auto& amp = getmap();
		return std::any_cast<T&>(amp.attchvec.at(aid));
	}

};

} // namespace ckt

#endif /* SRC_BASE_ATTCHM_H_ */
