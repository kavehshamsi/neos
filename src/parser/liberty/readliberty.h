

#ifndef _READLIBERTY_HH
#define _READLIBERTY_HH

#include <string>
#include <map>
#include "parser/liberty/liberty_parser.tab.hh"
#include "parser/liberty/liberty_ast.h"
#include "base/circuit.h"

// Give Flex the prototype of yylex we want ...
# define YY_DECL \
  ll::parser::symbol_type yylex (prs::liberty::readliberty& drv)
// ... and declare it for the parser's sake.
YY_DECL;

namespace prs {
namespace liberty {

// Conducting the whole scanning and parsing of Calc++.
class readliberty {
public:
  readliberty ();
  
  liberty_ast::liberty bast;
  
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
  ll::location location;

  void build_library(ckt::cell_library& mgr);
  void print_ast();

};

} // namespace liberty
} // namespace prs

#endif // ! DRIVER_HH
