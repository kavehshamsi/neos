
#ifndef _BENCH_AST_H
#define _BENCH_AST_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace prs {
namespace bench {
namespace ast {

using std::vector;
using std::string;
using boost::variant;

struct gate_decl {
    string out_name;
    string fun_name;
    vector<string> in_names;
};

struct assign_decl {
    string lhs;
    string rhs;
};

struct input_decl {
    string in_name;
};

struct output_decl {
    string out_name;
};

typedef variant<gate_decl, input_decl, output_decl, assign_decl> statement;
typedef vector<statement> bench;

} // namespace ast
} // namespace bench
} // namespace prs

#endif
