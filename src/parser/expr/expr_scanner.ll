%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <cstring> // strerror
# include <string>
# include "readexpr.h"
# include "expr_parser.tab.hh"
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

%option noyywrap nounput noinput batch debug prefix="ex"

id    [a-zA-Z_\.0-9]+
space [ \t\\]

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns (yyleng);
%}

%%
%{
  // A handy shortcut to the location held by the driver
  ex::location& loc = drv.location;
  // Code run each time yylex is called.
  loc.step ();
%}

{space}+   loc.step ();

[\r\n]+        { loc.lines(yyleng); loc.step(); }
		
"("        return ex::parser::make_LPAREN (loc);
")"        return ex::parser::make_RPAREN (loc);
"+"|"|"        return ex::parser::make_PLUS (yytext, loc);
"*"|"&"        return ex::parser::make_MUL (yytext, loc);
"!"|"~"        return ex::parser::make_NOT (yytext, loc);

{id}       return ex::parser::make_IDENT (yytext, loc);

.          {
             throw ex::parser::syntax_error
               (loc, "invalid character: " + std::string(yytext));
           }
           
<<EOF>>    { return ex::parser::make_END (loc); }

%%

void prs::expr::readexpr::scan_begin(FILE* f) {
  yy_flex_debug = trace_scanning;
  yyin = f;
}

void prs::expr::readexpr::scan_end() {
  fclose(yyin);
}
