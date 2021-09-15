%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "expr_ast.h"
  namespace prs {
  namespace expr {
  class readexpr;
  }
  }
}

// The parsing context.
%param { prs::expr::readexpr& drv }

%locations
%define parse.trace
%define parse.error verbose

%code {
#include "readexpr.h"
}

%define api.prefix {ex}
%define api.token.prefix {TOK_}
%token
  END 0 "end of file"
  LPAREN  "("
  RPAREN  ")"
;

%token <std::string> IDENT "ident"
%token <std::string> PLUS MUL NOT
%token <float> FLOAT 

%type <prs::expr::ast::expr> expr
%type <prs::expr::ast::binop_expr> binop_expr
%type <prs::expr::ast::unop_expr> unop_expr

%left PLUS
%left MUL
%precedence NOT


%%
%start top;

top : 
	expr { drv.bast = $1; } 
	;
	
expr :  
	  IDENT { $$ = $1; }
	| binop_expr { $$ = $1; }
	| unop_expr { $$ = $1; }
	| "(" expr ")" { $$ = $2; }
	;

unop_expr:
	  NOT expr %prec NOT { $$ = {$1, $2}; }
	;

binop_expr:
	  expr PLUS expr { $$ = {$1, $2, $3}; }
	| expr MUL expr { $$ = {$1, $2, $3}; }
	;

%%

void 
ex::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

