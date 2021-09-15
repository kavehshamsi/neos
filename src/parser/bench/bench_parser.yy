%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "bench_ast.h"
  namespace prs {
  namespace bench {
  class readbench;
  }
  }
}

// The parsing context.
%param { prs::bench::readbench& drv }

%locations
%define parse.trace
%define parse.error verbose

%code {
#include "readbench.h"
}

%define api.prefix {bb}
%define api.token.prefix {TOK_}
%token
  END 0 "end of file"
  ASSIGN  "="
  LPAREN  "("
  RPAREN  ")"
  COMMA   ","
  INPUT 
  OUTPUT
;

%token <std::string> IDENT "identifier"
%token <int> NUMBER

%type <std::vector<prs::bench::ast::statement>> bench
%type <prs::bench::ast::statement> statement
%type <prs::bench::ast::gate_decl> gate_decl
%type <prs::bench::ast::input_decl> input_decl
%type <prs::bench::ast::output_decl> output_decl
%type <prs::bench::ast::assign_decl> assign_decl
%type <std::vector<std::string>> ident_list


%%
%start bench;
bench : statement { drv.bast.push_back(std::move($1)); }
    | bench statement { drv.bast.push_back(std::move($2)); }
    ;
        
statement : 
      input_decl    { $$ = std::move($1); }
    | output_decl  	{ $$ = std::move($1); }
    | gate_decl     { $$ = std::move($1); }
    | assign_decl   { $$ = std::move($1); }
    ;
    
ident_list :     
      IDENT { $$.push_back(std::move($1)); }
    | ident_list "," IDENT { $1.push_back(std::move($3)); $$ = std::move($1); }
    ;
    
input_decl : INPUT "(" IDENT ")" { $$ = {std::move($3)}; };
output_decl : OUTPUT "(" IDENT ")" { $$ = {std::move($3)}; };
gate_decl : IDENT "=" IDENT "(" ident_list ")" { $$ = {std::move($1), std::move($3), std::move($5)}; };
assign_decl : IDENT "=" IDENT { $$ = {std::move($1), std::move($3)}; }
%%

void 
bb::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

