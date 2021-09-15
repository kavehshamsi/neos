/*
 * parser.h
 *
 *  Created on: Oct 8, 2017
 *      Author: kaveh
 */

#ifndef SRC_PARSER_H_
#define SRC_PARSER_H_

#define BOOST_SPIRIT_DEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

// forward declaration of circuit
namespace ckt {
class circuit;
class cell_library;
}

namespace prs {

using std::string;
using std::vector;

namespace bench {
bool parse_bench_file(const std::string& filename, ckt::circuit& cir);
bool parse_bench(std::istream& inist, ckt::circuit& ckt);
} // namespace bench

namespace verilog {
bool parse_verilog_file(const std::string& filename, ckt::circuit& cir);
bool parse_verilog(std::istream& inist, ckt::circuit& cir);
} // namespace verilog

namespace expr {
int parse_expr_to_cir(const std::string& expr_str, ckt::circuit& cir);
}

namespace genlib {
int parse_genlib_file(const std::string& filename, ckt::cell_library& cell_lib);
}

namespace liberty {
int parse_liberty_file(const std::string& filename, ckt::cell_library& cell_lib);
}


} // namespace prs

#endif /* SRC_PARSER_H_ */
