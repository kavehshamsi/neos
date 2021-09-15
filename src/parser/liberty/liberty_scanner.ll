%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <cstring> // strerror
# include <string>
# include "readliberty.h"
# include "liberty_parser.tab.hh"
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

%option noyywrap nounput noinput batch debug prefix="ll"

id    [a-zA-Z][a-zA-Z_0-9]*
aid   [a-zA-Z0-9_\.-]*
space [ \t\\]

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns (yyleng);
%}

%x comment qtstring

%%
%{
  // A handy shortcut to the location held by the driver
  ll::location& loc = drv.location;
  // Code run each time yylex is called.
  loc.step ();
%}

{space}+   loc.step ();

[\r\n]+        { loc.lines(yyleng); loc.step(); }

","        return ll::parser::make_COMMA  (loc);
"("        return ll::parser::make_LPAREN (loc);
")"        return ll::parser::make_RPAREN (loc);
"="        return ll::parser::make_ASSIGN (loc);
":"        return ll::parser::make_COLON (loc);
";"        return ll::parser::make_SEMICOLON (loc);
"{"        return ll::parser::make_LBRACE (loc);
"}"        return ll::parser::make_RBRACE (loc);
"&"        return ll::parser::make_AND (loc);
"|"        return ll::parser::make_OR (loc);
"*"        return ll::parser::make_MUL (loc);

{aid}       return ll::parser::make_IDENT (yytext, loc);


<INITIAL>\"	   BEGIN(qtstring);
<qtstring>[^\"]*  { return ll::parser::make_QSTRING(yytext, loc); }
<qtstring>\"  BEGIN(INITIAL);

"//".*     { /* DO NOTHING */ }

"/*"         BEGIN(comment);

<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by */
<comment>"\n"           { loc.lines(); loc.step(); }  
<comment>"*"+"/"        BEGIN(INITIAL);

.          {
             throw ll::parser::syntax_error
               (loc, "invalid character: " + std::string(yytext));
           }
           
<<EOF>>    { return ll::parser::make_END (loc); }

%%


void prs::liberty::readliberty::scan_begin(FILE* f) {
  yy_flex_debug = trace_scanning;
  yyin = f;
}

void prs::liberty::readliberty::scan_end() {
  fclose(yyin);
}
