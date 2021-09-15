%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "genlib_ast.h"
  namespace prs {
  namespace genlib {
  class readgenlib;
  }
  }
}

// The parsing context.
%param { prs::genlib::readgenlib& drv }

%locations
%define parse.trace
%define parse.error verbose

%code {
#include "readgenlib.h"
}

%define api.prefix {gl}
%define api.token.prefix {TOK_}
%token
  END 0 "end of file"
  ASSIGN  "="
  LPAREN  "("
  RPAREN  ")"
  PLUS    "+"
  MUL     "*"
  EXC     "!"
  COMMA   "," 
  COLON   ":"
  SEMICOLON  ";"
  GATE
  LATCH
  PIN
;

%token <std::string> IDENT "ident"
%token <float> FLOAT 

%type <prs::genlib::ast::genlib> genlib
%type <prs::genlib::ast::def> def
%type <prs::genlib::ast::def_list> def_list
%type <prs::genlib::ast::pin_def> pin_def
%type <prs::genlib::ast::pin_def_list> pin_def_list
%type <prs::genlib::ast::latch_def> latch_def
%type <prs::genlib::ast::gate_def> gate_def
%type <std::string> qident
%type < std::vector<std::string> > qident_list

%%
%start genlib; 

genlib :  
	def_list { 
		for (auto& x : $1) 
			drv.bast.nls.push_back(x);
		}
	;

def_list : %empty {}
	| def { $$.push_back($1); }
	| def_list def { $1.push_back(std::move($2)); $$ = std::move($1); }

def : 
	  gate_def { $$ = std::move($1); }
	| latch_def { $$ = std::move($1); }

latch_def:
	LATCH IDENT { $$ = {std::move($2)}; }
	;

gate_def:
	GATE IDENT FLOAT IDENT "=" qident ";" pin_def_list 
	{ $$ = {std::move($2), std::move($3), std::move($4), std::move($6), std::move($8)}; }
	;

pin_def_list : %empty {}
	| pin_def { $$.push_back(std::move($1)); }
	| pin_def_list pin_def { $1.push_back(std::move($2)); $$ = std::move($1); }
	;

pin_def:
	  PIN IDENT IDENT FLOAT FLOAT FLOAT FLOAT FLOAT FLOAT
	{ $$ = {std::move($2), std::move($3), std::move($4),
	 std::move($5), std::move($6), std::move($7), std::move($8), std::move($9) }; }
	| PIN "*" IDENT FLOAT FLOAT FLOAT FLOAT FLOAT FLOAT
	{ $$ = {"*", std::move($3), std::move($4), std::move($5),
	 std::move($6), std::move($7), std::move($8), std::move($9)}; }
	;

qident :
	  %empty {} 
	| IDENT { $$ = $1; }
	| qident { $$ = $1; }
	| qident IDENT { $$ = std::move($1) + std::move($2); }
	| qident "*" { $$ = std::move($1) + "*"; }
	| qident "+" { $$ = std::move($1) + "+"; }
	| qident "(" { $$ = std::move($1) + "("; }
	| qident ")" { $$ = std::move($1) + ")"; }
	| qident "!" { $$ = std::move($1) + "!"; }
	;
	
qident_list : %empty {}
	| qident { $$.push_back(std::move($1)); }
	| qident_list "," qident { $1.push_back(std::move($3)); $$ = std::move($1); }
	;

%%

void 
gl::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

