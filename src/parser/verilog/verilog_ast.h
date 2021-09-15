
#ifndef _VERILOG_AST_H
#define _VERILOG_AST_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace prs {
namespace verilog {
namespace ast {

using std::vector;
using std::string;
using boost::variant;
using std::pair;

struct range {
	long int left = -1;
	long int right = -1;
	inline int low() const { return std::min(left, right); }
	inline int high() const { return std::max(left, right); }
};

struct ranged_expr {
	range r;
	string basename;
};

struct indexed_expr {
	long int index;
	string basename;
};

typedef string bare_expr;
struct expr;
typedef vector<string> ident_list;
typedef vector<expr> expr_list;
struct concat_expr { vector<expr> vl; };

struct bin_constant {
	int num_bits;
	string values;
};

struct dec_constant {
	int num_bits;
	string digits;
};

struct hex_constant {
	int num_bits;
	string digits;
};

typedef int64_t integer;

typedef variant<bin_constant, dec_constant, hex_constant, string, integer> constant_expr;

struct expr { variant<ranged_expr, indexed_expr, concat_expr, bare_expr, constant_expr> v; };

struct prim_gate_stmt {
    expr out_net;
    string fun_name;
    string gate_name;
    expr_list in_nets;
};

struct ranged_var_decl {
	range r;
	string varname;
};

typedef string sing_var_decl;
typedef variant<ranged_var_decl, sing_var_decl> var_decl;

struct multi_var_decl {
	range r;
	vector<string> var_names;
};

typedef pair<string, expr> conn_pair;
typedef vector<conn_pair> conn_pair_list;

typedef variant<boost::blank, expr_list, conn_pair_list> param_config;

struct instance_stmt {
	string insttype;
	param_config parameters;
	string instname;
	variant< conn_pair_list, expr_list > ports;
};

struct assignment_stmt {
	expr lhs;
	expr rhs;
};

struct net_decl_stmt {
	string netdir;
	string netatt;
	multi_var_decl mvars;
};

struct module_port_decl {
	string netdir;
	string netatt;
	var_decl vdecl;
};

typedef vector<module_port_decl> module_port_decl_list;

struct module_header {
	string module_name;
	vector<module_port_decl> port_list;
};

typedef variant<instance_stmt, prim_gate_stmt, net_decl_stmt, assignment_stmt> statement;
typedef vector<statement> statement_list;

struct module {
	module_header mh;
	statement_list sts;
};

typedef vector<module> verilog;

} // namespace ast
} // namespace verilog
} // namespace prs

#endif
