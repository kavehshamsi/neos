

#ifndef _READGENLIB_HH
#define _READGENLIB_HH

#include <string>
#include <map>
#include "genlib_parser.tab.hh"
#include "base/circuit.h"
#include "genlib_ast.h"


// Give Flex the prototype of yylex we want ...
# define YY_DECL \
  gl::parser::symbol_type yylex (prs::genlib::readgenlib& drv)
// ... and declare it for the parser's sake.
YY_DECL;

namespace prs {
namespace genlib {

// Conducting the whole scanning and parsing of Calc++.
class readgenlib {
public:
  readgenlib ();
  
  prs::genlib::ast::genlib bast;
  
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
  gl::location location;

  void build_library(ckt::cell_library& cell_lib);
  void print_ast();

};

} // namespace genlib
} // namespace prs

#endif // ! DRIVER_HH
