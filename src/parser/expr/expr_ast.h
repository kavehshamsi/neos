
#ifndef _EXPR_AST_H
#define _EXPR_AST_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace prs {
namespace expr {
namespace ast {

using std::vector;
using std::string;
using boost::variant;

struct binop_expr;
typedef string unit_expr;

struct binop_expr;
struct unop_expr;

typedef variant< unit_expr,
				boost::recursive_wrapper<binop_expr>,
				boost::recursive_wrapper<unop_expr> > expr;

struct binop_expr {
	expr lhs;
	string op_str;
	expr rhs;
};

struct unop_expr {
	string op_str;
	expr e;
};

} // namespace ast
} // namespace expr
} // namespace prs

#endif
