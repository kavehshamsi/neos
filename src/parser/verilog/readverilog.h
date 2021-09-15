

#ifndef _READVERILOG_HH
#define _READVERILOG_HH

#include <string>
#include "verilog_parser.tab.hh"
#include "verilog_ast.h"
#include "base/circuit.h"

// Give Flex the prototype of yylex we want ...
# define YY_DECL \
  vv::parser::symbol_type yylex (prs::verilog::readverilog& drv)
// ... and declare it for the parser's sake.
YY_DECL;

namespace prs {
namespace verilog {

// Conducting the whole scanning and parsing of Calc++.
class readverilog {
public:
	readverilog();

	prs::verilog::ast::verilog vast;

	// Run the parser on file F.  Return 0 on success.
	int parse_file(const std::string& f);
	int parse_string(const std::string& instr);
	// The name of the file being parsed.
	std::string file;
	// Whether to generate parser debug traces.
	bool trace_parsing;

	// Handling the scanner.
	void scan_begin(FILE* fp);
	void scan_end();
	// Whether to generate scanner debug traces.
	bool trace_scanning;
	// The token's location used by the scanner.
	vv::location location;

	void to_circuit(ckt::circuit& cir);
};


} // namespace verilog
} // namespace prs

#endif // ! DRIVER_HH
