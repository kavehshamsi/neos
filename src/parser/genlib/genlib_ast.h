
#ifndef _GENLIB_AST_H
#define _GENLIB_AST_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace prs {
namespace genlib {
namespace ast {

using std::vector;
using std::string;
using boost::variant;

typedef std::string ident;
typedef vector<ident> ident_list;

struct pin_def {
	string pin_name;
	string phase;
	float input_load;
	float max_load;
	float rise_block_delay;
	float rise_fanout_delay;
	float fall_block_delay;
	float fall_fanout_delay;
};

typedef vector<pin_def> pin_def_list;

struct gate_def {
	string cell_name;
	float cell_area;
	string cell_out;
	string cell_fun;
	pin_def_list pins;
};

struct latch_def {
	string latch_name;
	// TODO: implement latch
};

typedef variant<gate_def, latch_def> def;
typedef vector<def> def_list;

struct genlib { def_list nls; };

} // namespace ast
} // namespace genlib
} // namespace prs

#endif
