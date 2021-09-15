

#ifndef _READEXPR_HH
#define _READEXPR_HH

#include <string>
#include <map>
#include "expr_parser.tab.hh"
#include "base/circuit.h"
#include "expr_ast.h"


// Give Flex the prototype of yylex we want ...
# define YY_DECL \
  ex::parser::symbol_type yylex (prs::expr::readexpr& drv)
// ... and declare it for the parser's sake.
YY_DECL;

namespace prs {
namespace expr {

// Conducting the whole scanning and parsing of Calc++.
class readexpr {
public:
  readexpr ();
  
  prs::expr::ast::expr bast;
  
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
  ex::location location;

  void to_circuit(ckt::circuit& retcir);

};

int parse_expr_to_cir(const std::string& expr_str, ckt::circuit& cir);

} // namespace expr
} // namespace prs

#endif // ! DRIVER_HH
