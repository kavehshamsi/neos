%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "liberty_ast.h"
  namespace prs {
  namespace liberty {
  class readliberty;
  }
  }
}

// The parsing context.
%param { prs::liberty::readliberty& drv }

%locations
%define parse.trace
%define parse.error verbose

%code {
#include "readliberty.h"
}

%define api.prefix {ll}
%define api.token.prefix {TOK_}
%token
  END 0 "end of file"
  ASSIGN  "="
  LPAREN  "("
  RPAREN  ")"
  LBRACE  "{"
  RBRACE  "}"
  AND     "&"
  OR      "|"
  MUL     "*"
  COMMA   "," 
  COLON   ":"
  SEMICOLON   ";"
;

%token <std::string> IDENT "ident"
%token <std::string> QSTRING
%token <int> NUMBER

%type <liberty_ast::liberty> liberty
%type <liberty_ast::node> node
%type <liberty_ast::node_list> node_list
%type <liberty_ast::node_assignment> node_assignment
%type <liberty_ast::node_property> node_property
%type <liberty_ast::named_node> named_node
%type <std::string> qident
%type < std::vector<std::string> > qident_list

%%
%start liberty; 

liberty :  
	node_list { 
		for (auto& x : $1) 
			drv.bast.nls.push_back(x);
		}
	;
    

node : 
	  node ";" { $$ = std::move($1); }
	| node_assignment { $$ = std::move($1); }
	| node_property { $$ = std::move($1); }
	| named_node { $$ = std::move($1); }
	;

node_list : %empty {}
	| node { $$.push_back(std::move($1)); }
	| node_list node { $1.push_back(std::move($2)); $$ = std::move($1); } 
	;

named_node:
	  IDENT "(" qident_list ")" "{" node_list "}" { $$ = {std::move($1), std::move($3), std::move($6)}; } 
	| IDENT "(" qident_list ")" { $$ = {std::move($1), std::move($3), {}}; }
	;

qident : 
	  IDENT { $$ = $1; }
	| QSTRING { $$ = $1; }
	| qident QSTRING { $$ = $1 + $2; }
	| qident IDENT { $$ = $1 + $2; }
	| qident "/" { $$ = $1 + "/"; }
	| qident "*" { $$ = $1 + "*"; }
	;

qident_list : %empty {}
	| qident { $$.push_back(std::move($1)); }
	| qident_list "," qident { $1.push_back(std::move($3)); $$ = std::move($1); }
	;

node_assignment :
	  qident "=" qident { $$ = {std::move($1), std::move($3)}; }
	;
	
node_property:
	  qident ":" qident { $$ = {std::move($1), std::move($3)}; }
	;


%%

void 
ll::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

