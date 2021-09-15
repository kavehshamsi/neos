%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <cstring> // strerror
# include <string>
# include <utility>
# include "readverilog.h"
# include "verilog_parser.tab.hh"
%}

%{
// Pacify warnings in yy_init_buffer (observed with Flex 2.6.4)
// and GCC 6.4.0, 7.3.0.
#if defined __GNUC__ && !defined __clang__ && 6 <= __GNUC__
# pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

// Of course, when compiling C as C++, expect warnings about NULL.
#if defined __clang__
# pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
# pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define FLEX_VERSION (YY_FLEX_MAJOR_VERSION * 100 + YY_FLEX_MINOR_VERSION)

// Old versions of Flex (2.5.35) generate an incomplete documentation comment.
//
//  In file included from src/scan-code-c.c:3:
//  src/scan-code.c:2198:21: error: empty paragraph passed to '@param' command
//        [-Werror,-Wdocumentation]
//   * @param line_number
//     ~~~~~~~~~~~~~~~~~^
//  1 error generated.
#if FLEX_VERSION < 206 && defined __clang__
# pragma clang diagnostic ignored "-Wdocumentation"
#endif

// Old versions of Flex (2.5.35) use 'register'.  Warnings introduced in
// GCC 7 and Clang 6.
#if FLEX_VERSION < 206
# if defined __clang__ && 6 <= __clang_major__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
# elif defined __GNUC__ && 7 <= __GNUC__
#  pragma GCC diagnostic ignored "-Wregister"
# endif
#endif

#if FLEX_VERSION < 206
# if defined __clang__
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wdocumentation"
#  pragma clang diagnostic ignored "-Wshorten-64-to-32"
#  pragma clang diagnostic ignored "-Wsign-conversion"
# elif defined __GNUC__
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
# endif
#endif

%}

%option noyywrap nounput noinput batch debug prefix="vv"

id    [a-zA-Z_][a-zA-Z_0-9]*
escid \\[^ ]+
int   [0-9]+
space [ \t]

%{
	#define YYMAXDEPTH 100
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns (vvleng);
  
  vv::parser::symbol_type
  help_CONSTBIN (const std::string &s, const vv::parser::location_type& loc);
  
  vv::parser::symbol_type
  help_CONSTDEC (const std::string &s, const vv::parser::location_type& loc);
  
  vv::parser::symbol_type
  help_CONSTHEX (const std::string &s, const vv::parser::location_type& loc);
  
%}

%x comment

%%
%{
  // A handy shortcut to the location held by the driver.
  vv::location& loc = drv.location;
  // Code run each time yylex is called.
  loc.step ();
%}

{space}+   loc.step();
[\r\n]+        { loc.lines(vvleng); loc.step(); }

"module"      return vv::parser::make_MODULEKW (loc);
"endmodule"   return vv::parser::make_ENDMODULEKW  (loc);
"assign"      return vv::parser::make_ASSIGNKW (loc);
"input"|"output" return vv::parser::make_PORTDIR (vvtext, loc);
"wire"|"reg"|"tri"        return vv::parser::make_PORTATT (vvtext, loc);

"nand"|"and"|"nor"|"or"|"xnor"|"xor"|"not"|"buf" {
		       		return vv::parser::make_PRIMGATE (vvtext, loc);
		       }
		       
","        return vv::parser::make_COMMA  (loc);
"("        return vv::parser::make_LPAREN (loc);
")"        return vv::parser::make_RPAREN (loc);
"["        return vv::parser::make_LBRACE (loc);
"]"        return vv::parser::make_RBRACE (loc);
"{"        return vv::parser::make_CLBRACE (loc);
"}"        return vv::parser::make_CRBRACE (loc);
"="        return vv::parser::make_ASSIGN (loc);
"."        return vv::parser::make_DOT (loc);
":"        return vv::parser::make_COLON (loc);
";"        return vv::parser::make_SEMICOLON (loc);
"#"        return vv::parser::make_POUND (loc); 
"`".*    { std::cout << "skipping grave_acc " << vvtext << " at " << loc << "\n"; }    
"initial".* { std::cout << "skipping initial " << vvtext << " at " << loc << "\n"; }

{int}	   	         	{ return vv::parser::make_INTEGER(std::stoi(vvtext), loc);  }
\".*\"               	{ return vv::parser::make_CONSTSTR(vvtext, loc); }
[0-9]+"'b"[0-1]+     	{ return help_CONSTBIN(vvtext, loc); }
[0-9]+"'d"[0-9]+   	 	{ return help_CONSTDEC(vvtext, loc); }
[0-9]+"'h"[0-9a-fA-F]+  { return help_CONSTHEX(vvtext, loc); }

{id}|{escid}       return vv::parser::make_IDENT (vvtext, loc);
"//".*     { /* DO NOTHING */ }            


"/*"         BEGIN(comment);

<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by */
<comment>"\n"             
<comment>"*"+"/"        BEGIN(INITIAL);

.          {
             throw vv::parser::syntax_error
               (loc, "invalid character: " + std::string(vvtext));
           }
           
<<EOF>>    { return vv::parser::make_END (loc); }

%%


vv::parser::symbol_type
help_CONSTBIN (const std::string &s, const vv::parser::location_type& loc)
{
  auto div = s.find("'b");
  int n = stoi(s.substr(0, div));
  std::string bin = s.substr(div + 2, s.size() - 1);
  prs::verilog::ast::bin_constant pr = {n, bin}; 
  return vv::parser::make_CONSTBIN (pr, loc);
}

vv::parser::symbol_type
help_CONSTDEC (const std::string &s, const vv::parser::location_type& loc)
{
  auto div = s.find("'d");
  int n = stoi(s.substr(0, div));
  std::string dec = s.substr(div + 2, s.size() - 1);
  prs::verilog::ast::dec_constant pr = {n, dec};
  return vv::parser::make_CONSTDEC (pr, loc);
}

vv::parser::symbol_type
help_CONSTHEX (const std::string &s, const vv::parser::location_type& loc)
{
  auto div = s.find("'h");
  int n = stoi(s.substr(0, div));
  std::string hex = s.substr(div + 2, s.size() - 1);
  prs::verilog::ast::hex_constant pr = {n, hex};
  return vv::parser::make_CONSTHEX (pr, loc);
}


void prs::verilog::readverilog::scan_begin(FILE* f) {
  yy_flex_debug = trace_scanning;
  yyin = f;
}

void prs::verilog::readverilog::scan_end() {
  fclose(yyin);
}
