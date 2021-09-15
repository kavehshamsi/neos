
#ifndef _LIBERTY_AST_H
#define _LIBERTY_AST_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace liberty_ast {

using std::vector;
using std::string;
using boost::variant;

typedef std::string ident;
typedef vector<ident> ident_list;

struct node_assignment {
	string lhs;
	string rhs;
};

struct node_property {
	string name;
	string prop;
};

struct named_node;

typedef variant<node_assignment, node_property, named_node> node;

struct named_node {
	string node_name;
	vector<string> node_params;
	vector<node> children;
};

typedef vector<node> node_list;

struct liberty { node_list nls; };

} // namespace bench_ast

#endif
