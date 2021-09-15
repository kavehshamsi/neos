/*
 * cell_lib.h
 *
 *  Created on: Jun 26, 2020
 *      Author: kaveh
 */

#ifndef SRC_BASE_CELL_LIB_H_
#define SRC_BASE_CELL_LIB_H_


#include "base/base.h"
#include "base/attchm.h"
#include <any>

namespace ckt {

using namespace utl;

class cell_library {
public:
	/*
	 *  for submodules (instances)
	 */
	enum class port_direction {INOUT, IN, OUT};

	struct cell_obj {
		string cell_name;
		Omap<string, id> portname2ind;
		vector<string> port_names;
		vector<port_direction> port_dirs;
		attchm_mgr_t attch;
	};

public:
	idmap<cell_obj> cells;
	Hmap<string, id> cellname2id_map;

public:
	id add_cell(const string& cell_name, const vector<string>& pin_names,
			const vector<port_direction>& pin_dirs = utl::empty<vector<port_direction>>());
	int add_cell_attr(id cellid, const string& attr_nm, const std::any& attr);
	const std::any& get_cell_attr(id cellid, const string& attr_nm) const;
	const cell_obj& getccell(id cellid) const;
	cell_obj& getcell(id cellid);
	void resolve_cell_direction_byname(const std::vector<string>& out_names);
	void read_liberty(const string& filename);
	void print_lib() const;
	void print_cell(id cellid) const;

	template<typename T = std::any>
	const T& get_cell_attr(id cellid, id ind) const {
		return getccell(cellid).attch.get_cattatchment<T>(ind);
	}

	template<typename T = std::any>
	const T& get_cell_attr(id cellid, const string& attr_nm) const {
		return getccell(cellid).attch.get_cattatchment<T>(attr_nm);
	}

};

}


#endif /* SRC_BASE_CELL_LIB_H_ */
