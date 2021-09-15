

#ifndef _READBENCH_HH
#define _READBENCH_HH

#include <string>
#include <map>
#include "bench_parser.tab.hh"
#include "bench_ast.h"
#include "base/circuit.h"


// Give Flex the prototype of yylex we want ...
# define YY_DECL \
  bb::parser::symbol_type yylex (prs::bench::readbench& drv)
// ... and declare it for the parser's sake.
YY_DECL;

namespace prs {
namespace bench {

// Conducting the whole scanning and parsing of Calc++.
class readbench {
public:
  readbench ();
  
  prs::bench::ast::bench bast;
  
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
  bb::location location;

  void to_circuit(ckt::circuit& cir);
};

} // namespace bench
} // namespace prs

#endif // ! DRIVER_HH
