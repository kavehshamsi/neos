%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include<string> 
  #include<utility>
  #include "verilog_ast.h"
  namespace prs {
  namespace verilog {
  class readverilog;
  }
  }
}

// The parsing context.
%param { prs::verilog::readverilog& drv }

%locations
%define parse.trace
%define parse.error verbose

%code {
#include "readverilog.h"
}

%define api.prefix {vv}
%define api.location.file none
%define api.token.prefix {TOK_}
%token
  END 0 "end of file"
  ASSIGN "="
  LPAREN  "("
  RPAREN  ")"
  LBRACE  "["
  RBRACE  "]"
  CLBRACE  "{"
  CRBRACE  "}"
  COMMA   ","
  DOT	"."
  SEMICOLON ";"
  COLON ":"
  POUND "#"
  ENDMODULEKW
  MODULEKW
  ASSIGNKW
;

%token <std::string> IDENT "ident"
%token <std::string> PRIMGATE "primitive_gate"
%token <std::string> PORTATT "variable_attribute"
%token <std::string> PORTDIR "port_direction"
%token <prs::verilog::ast::integer> INTEGER
%token <prs::verilog::ast::hex_constant> CONSTHEX "hex_constant"
%token <prs::verilog::ast::bin_constant> CONSTBIN "bin_constant"
%token <prs::verilog::ast::dec_constant> CONSTDEC "dec_constant"
%token <std::string> CONSTSTR "str_constant"

%type <prs::verilog::ast::verilog> verilog
%type <prs::verilog::ast::module> module
%type <prs::verilog::ast::module_header> module_header
%type <prs::verilog::ast::module_port_decl> module_port_decl
%type <prs::verilog::ast::module_port_decl_list> module_port_decl_list
%type <prs::verilog::ast::statement> statement
%type <prs::verilog::ast::statement_list> statement_list
%type <prs::verilog::ast::assignment_stmt> assignment_stmt
%type <prs::verilog::ast::net_decl_stmt> net_decl_stmt
%type <prs::verilog::ast::prim_gate_stmt> prim_gate_stmt
%type <prs::verilog::ast::instance_stmt> instance_stmt
%type <prs::verilog::ast::param_config> param_config
%type <prs::verilog::ast::expr> expr
%type <prs::verilog::ast::constant_expr> constant_expr
%type <prs::verilog::ast::ident_list> ident_list
%type <prs::verilog::ast::conn_pair_list> conn_pair_list
%type <prs::verilog::ast::conn_pair> conn_pair
%type <prs::verilog::ast::range> range
%type <prs::verilog::ast::indexed_expr> indexed_expr
%type <prs::verilog::ast::ranged_expr> ranged_expr
%type <prs::verilog::ast::concat_expr> concat_expr
%type <prs::verilog::ast::expr_list> expr_list
%type <prs::verilog::ast::multi_var_decl> multi_var_decl
%type <prs::verilog::ast::var_decl> var_decl
%type <prs::verilog::ast::ranged_var_decl> ranged_var_decl

%%
%start verilog;
verilog : module { drv.vast.push_back($1); }
    | verilog module { drv.vast.push_back($2); } 
    ;

module : module_header statement_list ENDMODULEKW { $$ = {std::move($1), std::move($2)}; }
	;

module_header : 
	  MODULEKW IDENT { $$.module_name = $2; }
	| MODULEKW IDENT "(" module_port_decl_list ")" { $$ = {std::move($2), std::move($4)}; }
	| module_header ";"  { $$ = std::move($1); }
	;

module_port_decl_list :
	  module_port_decl { $$.push_back($1); }
	| module_port_decl_list "," module_port_decl { $1.push_back($3); $$ = std::move($1); }
	;

module_port_decl : 
	  var_decl { $$ = {"", "", std::move($1)}; }
	| PORTATT var_decl  { $$ = {"", std::move($1), std::move($2)}; }
	| PORTDIR PORTATT var_decl { $$ = {std::move($1), std::move($2), std::move($3)}; }
	;

statement_list :
	  %empty {}
	| statement { $$.push_back($1); }
	| statement_list statement { $1.push_back($2); $$ = std::move($1); }
	;

statement :
 	  statement ";" { $$ = std::move($1); }
    | prim_gate_stmt ";" { $$ = std::move($1); }
    | net_decl_stmt ";" { $$ = std::move($1); }
    | instance_stmt ";" { $$ = std::move($1); }  
    | assignment_stmt ";" { $$ = std::move($1); }
    ;

net_decl_stmt : 
	  PORTATT multi_var_decl  { $$ = {"", std::move($1), std::move($2)}; }
    | PORTDIR multi_var_decl { $$ = {std::move($1), "", std::move($2)}; }
	| PORTDIR PORTATT multi_var_decl { $$ = {std::move($1), std::move($2), std::move($3)}; }
	;
	
var_decl :
	  IDENT { $$ = $1;  }
	| ranged_var_decl { $$ = $1; }
	;
	
ranged_var_decl: 
	  range IDENT { $$ = {std::move($1), std::move($2)}; }
	;
	
multi_var_decl : 
	  ident_list { $$.var_names = $1; }
	| range ident_list { $$ = {std::move($1), std::move($2)}; } 
	;
	
instance_stmt :
	  IDENT param_config IDENT "(" expr_list ")" { $$ = {std::move($1), std::move($2), std::move($3), std::move($5)}; }
	| IDENT param_config IDENT "(" conn_pair_list ")" { $$ = {std::move($1), std::move($2), std::move($3), std::move($5)}; }
	;


param_config :
	%empty {}
	| POUND "(" expr_list ")" { $$ = std::move($3); }
	| POUND "(" conn_pair_list ")" { $$ = std::move($3); }
	;


constant_expr :
	  INTEGER { $$ = std::to_string($1); }
	| "bin_constant" { $$ = std::move($1); }
	| "dec_constant" { $$ = std::move($1); }
	| "hex_constant" { $$ = std::move($1); }
	| "str_constant" { $$ = std::move($1); }
	;

prim_gate_stmt :
	  PRIMGATE IDENT "(" expr "," expr_list ")" { $$ = {std::move($4), std::move($1), std::move($2), std::move($6)}; }
	;

assignment_stmt:
	  ASSIGNKW expr "=" expr { $$ = {std::move($2), std::move($4)}; }
	;

expr_list :
	  expr { $$.push_back($1); }
	| expr_list "," expr { $1.push_back($3); $$ = std::move($1); }
	;

expr :
	  IDENT { $$ = {$1}; }
	| ranged_expr { $$ = {$1}; }
	| indexed_expr { $$ = {$1}; }
	| concat_expr { $$ = {$1}; }
	| constant_expr { $$ = {$1}; }
	;

ranged_expr : IDENT range { $$ = {std::move($2), std::move($1)}; }
	;
	
indexed_expr : IDENT "[" INTEGER "]" { 
		if ($3 < 0)
			throw vv::parser::syntax_error(@3, "cannot use negative index");
		$$ = {$3, $1};
		}
	;

concat_expr :
	  "{" expr_list "}"  { $$ = {$2}; }
	;

range : "[" INTEGER ":" INTEGER "]" { 
		if ($2 < 0 || $4 < 0) 
			throw vv::parser::syntax_error(@2, "negative range is invalid");
		$$ = {std::move($2), std::move($4)}; 
		}
	;

conn_pair : "." IDENT "(" expr ")" { $$ = {std::move($2), std::move($4)}; }
	;

conn_pair_list : 
	  conn_pair { $$.push_back($1); }
	| conn_pair_list "," conn_pair { $1.push_back($3); $$ = std::move($1); }
	;	
	
ident_list :     
    IDENT { $$.push_back($1); }
    | ident_list "," IDENT { $1.push_back($3); $$ = std::move($1); }
    ;
    
%%

void 
vv::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

