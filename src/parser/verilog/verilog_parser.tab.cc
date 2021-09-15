// A Bison parser, made by GNU Bison 3.5.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2019 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.


// Take the name prefix into account.
#define yylex   vvlex



#include "verilog_parser.tab.hh"


// Unqualified %code blocks.
#line 27 "src/parser/verilog/verilog_parser.yy"

#include "readverilog.h"

#line 51 "src/parser/verilog/verilog_parser.tab.cc"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if VVDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !VVDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !VVDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

namespace vv {
#line 142 "src/parser/verilog/verilog_parser.tab.cc"


  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (prs::verilog::readverilog& drv_yyarg)
#if VVDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      drv (drv_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_number_type
  parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 43: // assignment_stmt
        value.YY_MOVE_OR_COPY< prs::verilog::ast::assignment_stmt > (YY_MOVE (that.value));
        break;

      case 24: // "bin_constant"
        value.YY_MOVE_OR_COPY< prs::verilog::ast::bin_constant > (YY_MOVE (that.value));
        break;

      case 48: // concat_expr
        value.YY_MOVE_OR_COPY< prs::verilog::ast::concat_expr > (YY_MOVE (that.value));
        break;

      case 50: // conn_pair
        value.YY_MOVE_OR_COPY< prs::verilog::ast::conn_pair > (YY_MOVE (that.value));
        break;

      case 51: // conn_pair_list
        value.YY_MOVE_OR_COPY< prs::verilog::ast::conn_pair_list > (YY_MOVE (that.value));
        break;

      case 41: // constant_expr
        value.YY_MOVE_OR_COPY< prs::verilog::ast::constant_expr > (YY_MOVE (that.value));
        break;

      case 25: // "dec_constant"
        value.YY_MOVE_OR_COPY< prs::verilog::ast::dec_constant > (YY_MOVE (that.value));
        break;

      case 45: // expr
        value.YY_MOVE_OR_COPY< prs::verilog::ast::expr > (YY_MOVE (that.value));
        break;

      case 44: // expr_list
        value.YY_MOVE_OR_COPY< prs::verilog::ast::expr_list > (YY_MOVE (that.value));
        break;

      case 23: // "hex_constant"
        value.YY_MOVE_OR_COPY< prs::verilog::ast::hex_constant > (YY_MOVE (that.value));
        break;

      case 52: // ident_list
        value.YY_MOVE_OR_COPY< prs::verilog::ast::ident_list > (YY_MOVE (that.value));
        break;

      case 47: // indexed_expr
        value.YY_MOVE_OR_COPY< prs::verilog::ast::indexed_expr > (YY_MOVE (that.value));
        break;

      case 39: // instance_stmt
        value.YY_MOVE_OR_COPY< prs::verilog::ast::instance_stmt > (YY_MOVE (that.value));
        break;

      case 22: // INTEGER
        value.YY_MOVE_OR_COPY< prs::verilog::ast::integer > (YY_MOVE (that.value));
        break;

      case 29: // module
        value.YY_MOVE_OR_COPY< prs::verilog::ast::module > (YY_MOVE (that.value));
        break;

      case 30: // module_header
        value.YY_MOVE_OR_COPY< prs::verilog::ast::module_header > (YY_MOVE (that.value));
        break;

      case 32: // module_port_decl
        value.YY_MOVE_OR_COPY< prs::verilog::ast::module_port_decl > (YY_MOVE (that.value));
        break;

      case 31: // module_port_decl_list
        value.YY_MOVE_OR_COPY< prs::verilog::ast::module_port_decl_list > (YY_MOVE (that.value));
        break;

      case 38: // multi_var_decl
        value.YY_MOVE_OR_COPY< prs::verilog::ast::multi_var_decl > (YY_MOVE (that.value));
        break;

      case 35: // net_decl_stmt
        value.YY_MOVE_OR_COPY< prs::verilog::ast::net_decl_stmt > (YY_MOVE (that.value));
        break;

      case 40: // param_config
        value.YY_MOVE_OR_COPY< prs::verilog::ast::param_config > (YY_MOVE (that.value));
        break;

      case 42: // prim_gate_stmt
        value.YY_MOVE_OR_COPY< prs::verilog::ast::prim_gate_stmt > (YY_MOVE (that.value));
        break;

      case 49: // range
        value.YY_MOVE_OR_COPY< prs::verilog::ast::range > (YY_MOVE (that.value));
        break;

      case 46: // ranged_expr
        value.YY_MOVE_OR_COPY< prs::verilog::ast::ranged_expr > (YY_MOVE (that.value));
        break;

      case 37: // ranged_var_decl
        value.YY_MOVE_OR_COPY< prs::verilog::ast::ranged_var_decl > (YY_MOVE (that.value));
        break;

      case 34: // statement
        value.YY_MOVE_OR_COPY< prs::verilog::ast::statement > (YY_MOVE (that.value));
        break;

      case 33: // statement_list
        value.YY_MOVE_OR_COPY< prs::verilog::ast::statement_list > (YY_MOVE (that.value));
        break;

      case 36: // var_decl
        value.YY_MOVE_OR_COPY< prs::verilog::ast::var_decl > (YY_MOVE (that.value));
        break;

      case 28: // verilog
        value.YY_MOVE_OR_COPY< prs::verilog::ast::verilog > (YY_MOVE (that.value));
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 43: // assignment_stmt
        value.move< prs::verilog::ast::assignment_stmt > (YY_MOVE (that.value));
        break;

      case 24: // "bin_constant"
        value.move< prs::verilog::ast::bin_constant > (YY_MOVE (that.value));
        break;

      case 48: // concat_expr
        value.move< prs::verilog::ast::concat_expr > (YY_MOVE (that.value));
        break;

      case 50: // conn_pair
        value.move< prs::verilog::ast::conn_pair > (YY_MOVE (that.value));
        break;

      case 51: // conn_pair_list
        value.move< prs::verilog::ast::conn_pair_list > (YY_MOVE (that.value));
        break;

      case 41: // constant_expr
        value.move< prs::verilog::ast::constant_expr > (YY_MOVE (that.value));
        break;

      case 25: // "dec_constant"
        value.move< prs::verilog::ast::dec_constant > (YY_MOVE (that.value));
        break;

      case 45: // expr
        value.move< prs::verilog::ast::expr > (YY_MOVE (that.value));
        break;

      case 44: // expr_list
        value.move< prs::verilog::ast::expr_list > (YY_MOVE (that.value));
        break;

      case 23: // "hex_constant"
        value.move< prs::verilog::ast::hex_constant > (YY_MOVE (that.value));
        break;

      case 52: // ident_list
        value.move< prs::verilog::ast::ident_list > (YY_MOVE (that.value));
        break;

      case 47: // indexed_expr
        value.move< prs::verilog::ast::indexed_expr > (YY_MOVE (that.value));
        break;

      case 39: // instance_stmt
        value.move< prs::verilog::ast::instance_stmt > (YY_MOVE (that.value));
        break;

      case 22: // INTEGER
        value.move< prs::verilog::ast::integer > (YY_MOVE (that.value));
        break;

      case 29: // module
        value.move< prs::verilog::ast::module > (YY_MOVE (that.value));
        break;

      case 30: // module_header
        value.move< prs::verilog::ast::module_header > (YY_MOVE (that.value));
        break;

      case 32: // module_port_decl
        value.move< prs::verilog::ast::module_port_decl > (YY_MOVE (that.value));
        break;

      case 31: // module_port_decl_list
        value.move< prs::verilog::ast::module_port_decl_list > (YY_MOVE (that.value));
        break;

      case 38: // multi_var_decl
        value.move< prs::verilog::ast::multi_var_decl > (YY_MOVE (that.value));
        break;

      case 35: // net_decl_stmt
        value.move< prs::verilog::ast::net_decl_stmt > (YY_MOVE (that.value));
        break;

      case 40: // param_config
        value.move< prs::verilog::ast::param_config > (YY_MOVE (that.value));
        break;

      case 42: // prim_gate_stmt
        value.move< prs::verilog::ast::prim_gate_stmt > (YY_MOVE (that.value));
        break;

      case 49: // range
        value.move< prs::verilog::ast::range > (YY_MOVE (that.value));
        break;

      case 46: // ranged_expr
        value.move< prs::verilog::ast::ranged_expr > (YY_MOVE (that.value));
        break;

      case 37: // ranged_var_decl
        value.move< prs::verilog::ast::ranged_var_decl > (YY_MOVE (that.value));
        break;

      case 34: // statement
        value.move< prs::verilog::ast::statement > (YY_MOVE (that.value));
        break;

      case 33: // statement_list
        value.move< prs::verilog::ast::statement_list > (YY_MOVE (that.value));
        break;

      case 36: // var_decl
        value.move< prs::verilog::ast::var_decl > (YY_MOVE (that.value));
        break;

      case 28: // verilog
        value.move< prs::verilog::ast::verilog > (YY_MOVE (that.value));
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.move< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 43: // assignment_stmt
        value.copy< prs::verilog::ast::assignment_stmt > (that.value);
        break;

      case 24: // "bin_constant"
        value.copy< prs::verilog::ast::bin_constant > (that.value);
        break;

      case 48: // concat_expr
        value.copy< prs::verilog::ast::concat_expr > (that.value);
        break;

      case 50: // conn_pair
        value.copy< prs::verilog::ast::conn_pair > (that.value);
        break;

      case 51: // conn_pair_list
        value.copy< prs::verilog::ast::conn_pair_list > (that.value);
        break;

      case 41: // constant_expr
        value.copy< prs::verilog::ast::constant_expr > (that.value);
        break;

      case 25: // "dec_constant"
        value.copy< prs::verilog::ast::dec_constant > (that.value);
        break;

      case 45: // expr
        value.copy< prs::verilog::ast::expr > (that.value);
        break;

      case 44: // expr_list
        value.copy< prs::verilog::ast::expr_list > (that.value);
        break;

      case 23: // "hex_constant"
        value.copy< prs::verilog::ast::hex_constant > (that.value);
        break;

      case 52: // ident_list
        value.copy< prs::verilog::ast::ident_list > (that.value);
        break;

      case 47: // indexed_expr
        value.copy< prs::verilog::ast::indexed_expr > (that.value);
        break;

      case 39: // instance_stmt
        value.copy< prs::verilog::ast::instance_stmt > (that.value);
        break;

      case 22: // INTEGER
        value.copy< prs::verilog::ast::integer > (that.value);
        break;

      case 29: // module
        value.copy< prs::verilog::ast::module > (that.value);
        break;

      case 30: // module_header
        value.copy< prs::verilog::ast::module_header > (that.value);
        break;

      case 32: // module_port_decl
        value.copy< prs::verilog::ast::module_port_decl > (that.value);
        break;

      case 31: // module_port_decl_list
        value.copy< prs::verilog::ast::module_port_decl_list > (that.value);
        break;

      case 38: // multi_var_decl
        value.copy< prs::verilog::ast::multi_var_decl > (that.value);
        break;

      case 35: // net_decl_stmt
        value.copy< prs::verilog::ast::net_decl_stmt > (that.value);
        break;

      case 40: // param_config
        value.copy< prs::verilog::ast::param_config > (that.value);
        break;

      case 42: // prim_gate_stmt
        value.copy< prs::verilog::ast::prim_gate_stmt > (that.value);
        break;

      case 49: // range
        value.copy< prs::verilog::ast::range > (that.value);
        break;

      case 46: // ranged_expr
        value.copy< prs::verilog::ast::ranged_expr > (that.value);
        break;

      case 37: // ranged_var_decl
        value.copy< prs::verilog::ast::ranged_var_decl > (that.value);
        break;

      case 34: // statement
        value.copy< prs::verilog::ast::statement > (that.value);
        break;

      case 33: // statement_list
        value.copy< prs::verilog::ast::statement_list > (that.value);
        break;

      case 36: // var_decl
        value.copy< prs::verilog::ast::var_decl > (that.value);
        break;

      case 28: // verilog
        value.copy< prs::verilog::ast::verilog > (that.value);
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.copy< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 43: // assignment_stmt
        value.move< prs::verilog::ast::assignment_stmt > (that.value);
        break;

      case 24: // "bin_constant"
        value.move< prs::verilog::ast::bin_constant > (that.value);
        break;

      case 48: // concat_expr
        value.move< prs::verilog::ast::concat_expr > (that.value);
        break;

      case 50: // conn_pair
        value.move< prs::verilog::ast::conn_pair > (that.value);
        break;

      case 51: // conn_pair_list
        value.move< prs::verilog::ast::conn_pair_list > (that.value);
        break;

      case 41: // constant_expr
        value.move< prs::verilog::ast::constant_expr > (that.value);
        break;

      case 25: // "dec_constant"
        value.move< prs::verilog::ast::dec_constant > (that.value);
        break;

      case 45: // expr
        value.move< prs::verilog::ast::expr > (that.value);
        break;

      case 44: // expr_list
        value.move< prs::verilog::ast::expr_list > (that.value);
        break;

      case 23: // "hex_constant"
        value.move< prs::verilog::ast::hex_constant > (that.value);
        break;

      case 52: // ident_list
        value.move< prs::verilog::ast::ident_list > (that.value);
        break;

      case 47: // indexed_expr
        value.move< prs::verilog::ast::indexed_expr > (that.value);
        break;

      case 39: // instance_stmt
        value.move< prs::verilog::ast::instance_stmt > (that.value);
        break;

      case 22: // INTEGER
        value.move< prs::verilog::ast::integer > (that.value);
        break;

      case 29: // module
        value.move< prs::verilog::ast::module > (that.value);
        break;

      case 30: // module_header
        value.move< prs::verilog::ast::module_header > (that.value);
        break;

      case 32: // module_port_decl
        value.move< prs::verilog::ast::module_port_decl > (that.value);
        break;

      case 31: // module_port_decl_list
        value.move< prs::verilog::ast::module_port_decl_list > (that.value);
        break;

      case 38: // multi_var_decl
        value.move< prs::verilog::ast::multi_var_decl > (that.value);
        break;

      case 35: // net_decl_stmt
        value.move< prs::verilog::ast::net_decl_stmt > (that.value);
        break;

      case 40: // param_config
        value.move< prs::verilog::ast::param_config > (that.value);
        break;

      case 42: // prim_gate_stmt
        value.move< prs::verilog::ast::prim_gate_stmt > (that.value);
        break;

      case 49: // range
        value.move< prs::verilog::ast::range > (that.value);
        break;

      case 46: // ranged_expr
        value.move< prs::verilog::ast::ranged_expr > (that.value);
        break;

      case 37: // ranged_var_decl
        value.move< prs::verilog::ast::ranged_var_decl > (that.value);
        break;

      case 34: // statement
        value.move< prs::verilog::ast::statement > (that.value);
        break;

      case 33: // statement_list
        value.move< prs::verilog::ast::statement_list > (that.value);
        break;

      case 36: // var_decl
        value.move< prs::verilog::ast::var_decl > (that.value);
        break;

      case 28: // verilog
        value.move< prs::verilog::ast::verilog > (that.value);
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if VVDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if VVDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // VVDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (drv));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", static_cast<state_type> (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 43: // assignment_stmt
        yylhs.value.emplace< prs::verilog::ast::assignment_stmt > ();
        break;

      case 24: // "bin_constant"
        yylhs.value.emplace< prs::verilog::ast::bin_constant > ();
        break;

      case 48: // concat_expr
        yylhs.value.emplace< prs::verilog::ast::concat_expr > ();
        break;

      case 50: // conn_pair
        yylhs.value.emplace< prs::verilog::ast::conn_pair > ();
        break;

      case 51: // conn_pair_list
        yylhs.value.emplace< prs::verilog::ast::conn_pair_list > ();
        break;

      case 41: // constant_expr
        yylhs.value.emplace< prs::verilog::ast::constant_expr > ();
        break;

      case 25: // "dec_constant"
        yylhs.value.emplace< prs::verilog::ast::dec_constant > ();
        break;

      case 45: // expr
        yylhs.value.emplace< prs::verilog::ast::expr > ();
        break;

      case 44: // expr_list
        yylhs.value.emplace< prs::verilog::ast::expr_list > ();
        break;

      case 23: // "hex_constant"
        yylhs.value.emplace< prs::verilog::ast::hex_constant > ();
        break;

      case 52: // ident_list
        yylhs.value.emplace< prs::verilog::ast::ident_list > ();
        break;

      case 47: // indexed_expr
        yylhs.value.emplace< prs::verilog::ast::indexed_expr > ();
        break;

      case 39: // instance_stmt
        yylhs.value.emplace< prs::verilog::ast::instance_stmt > ();
        break;

      case 22: // INTEGER
        yylhs.value.emplace< prs::verilog::ast::integer > ();
        break;

      case 29: // module
        yylhs.value.emplace< prs::verilog::ast::module > ();
        break;

      case 30: // module_header
        yylhs.value.emplace< prs::verilog::ast::module_header > ();
        break;

      case 32: // module_port_decl
        yylhs.value.emplace< prs::verilog::ast::module_port_decl > ();
        break;

      case 31: // module_port_decl_list
        yylhs.value.emplace< prs::verilog::ast::module_port_decl_list > ();
        break;

      case 38: // multi_var_decl
        yylhs.value.emplace< prs::verilog::ast::multi_var_decl > ();
        break;

      case 35: // net_decl_stmt
        yylhs.value.emplace< prs::verilog::ast::net_decl_stmt > ();
        break;

      case 40: // param_config
        yylhs.value.emplace< prs::verilog::ast::param_config > ();
        break;

      case 42: // prim_gate_stmt
        yylhs.value.emplace< prs::verilog::ast::prim_gate_stmt > ();
        break;

      case 49: // range
        yylhs.value.emplace< prs::verilog::ast::range > ();
        break;

      case 46: // ranged_expr
        yylhs.value.emplace< prs::verilog::ast::ranged_expr > ();
        break;

      case 37: // ranged_var_decl
        yylhs.value.emplace< prs::verilog::ast::ranged_var_decl > ();
        break;

      case 34: // statement
        yylhs.value.emplace< prs::verilog::ast::statement > ();
        break;

      case 33: // statement_list
        yylhs.value.emplace< prs::verilog::ast::statement_list > ();
        break;

      case 36: // var_decl
        yylhs.value.emplace< prs::verilog::ast::var_decl > ();
        break;

      case 28: // verilog
        yylhs.value.emplace< prs::verilog::ast::verilog > ();
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        yylhs.value.emplace< std::string > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2:
#line 91 "src/parser/verilog/verilog_parser.yy"
                 { drv.vast.push_back(yystack_[0].value.as < prs::verilog::ast::module > ()); }
#line 1189 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 3:
#line 92 "src/parser/verilog/verilog_parser.yy"
                     { drv.vast.push_back(yystack_[0].value.as < prs::verilog::ast::module > ()); }
#line 1195 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 4:
#line 95 "src/parser/verilog/verilog_parser.yy"
                                                  { yylhs.value.as < prs::verilog::ast::module > () = {std::move(yystack_[2].value.as < prs::verilog::ast::module_header > ()), std::move(yystack_[1].value.as < prs::verilog::ast::statement_list > ())}; }
#line 1201 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 5:
#line 99 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::module_header > ().module_name = yystack_[0].value.as < std::string > (); }
#line 1207 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 6:
#line 100 "src/parser/verilog/verilog_parser.yy"
                                                       { yylhs.value.as < prs::verilog::ast::module_header > () = {std::move(yystack_[3].value.as < std::string > ()), std::move(yystack_[1].value.as < prs::verilog::ast::module_port_decl_list > ())}; }
#line 1213 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 7:
#line 101 "src/parser/verilog/verilog_parser.yy"
                             { yylhs.value.as < prs::verilog::ast::module_header > () = std::move(yystack_[1].value.as < prs::verilog::ast::module_header > ()); }
#line 1219 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 8:
#line 105 "src/parser/verilog/verilog_parser.yy"
                           { yylhs.value.as < prs::verilog::ast::module_port_decl_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::module_port_decl > ()); }
#line 1225 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 9:
#line 106 "src/parser/verilog/verilog_parser.yy"
                                                     { yystack_[2].value.as < prs::verilog::ast::module_port_decl_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::module_port_decl > ()); yylhs.value.as < prs::verilog::ast::module_port_decl_list > () = std::move(yystack_[2].value.as < prs::verilog::ast::module_port_decl_list > ()); }
#line 1231 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 10:
#line 110 "src/parser/verilog/verilog_parser.yy"
                   { yylhs.value.as < prs::verilog::ast::module_port_decl > () = {"", "", std::move(yystack_[0].value.as < prs::verilog::ast::var_decl > ())}; }
#line 1237 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 11:
#line 111 "src/parser/verilog/verilog_parser.yy"
                            { yylhs.value.as < prs::verilog::ast::module_port_decl > () = {"", std::move(yystack_[1].value.as < std::string > ()), std::move(yystack_[0].value.as < prs::verilog::ast::var_decl > ())}; }
#line 1243 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 12:
#line 112 "src/parser/verilog/verilog_parser.yy"
                                   { yylhs.value.as < prs::verilog::ast::module_port_decl > () = {std::move(yystack_[2].value.as < std::string > ()), std::move(yystack_[1].value.as < std::string > ()), std::move(yystack_[0].value.as < prs::verilog::ast::var_decl > ())}; }
#line 1249 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 13:
#line 116 "src/parser/verilog/verilog_parser.yy"
                 {}
#line 1255 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 14:
#line 117 "src/parser/verilog/verilog_parser.yy"
                    { yylhs.value.as < prs::verilog::ast::statement_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::statement > ()); }
#line 1261 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 15:
#line 118 "src/parser/verilog/verilog_parser.yy"
                                   { yystack_[1].value.as < prs::verilog::ast::statement_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::statement > ()); yylhs.value.as < prs::verilog::ast::statement_list > () = std::move(yystack_[1].value.as < prs::verilog::ast::statement_list > ()); }
#line 1267 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 16:
#line 122 "src/parser/verilog/verilog_parser.yy"
                        { yylhs.value.as < prs::verilog::ast::statement > () = std::move(yystack_[1].value.as < prs::verilog::ast::statement > ()); }
#line 1273 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 17:
#line 123 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::statement > () = std::move(yystack_[1].value.as < prs::verilog::ast::prim_gate_stmt > ()); }
#line 1279 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 18:
#line 124 "src/parser/verilog/verilog_parser.yy"
                        { yylhs.value.as < prs::verilog::ast::statement > () = std::move(yystack_[1].value.as < prs::verilog::ast::net_decl_stmt > ()); }
#line 1285 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 19:
#line 125 "src/parser/verilog/verilog_parser.yy"
                        { yylhs.value.as < prs::verilog::ast::statement > () = std::move(yystack_[1].value.as < prs::verilog::ast::instance_stmt > ()); }
#line 1291 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 20:
#line 126 "src/parser/verilog/verilog_parser.yy"
                          { yylhs.value.as < prs::verilog::ast::statement > () = std::move(yystack_[1].value.as < prs::verilog::ast::assignment_stmt > ()); }
#line 1297 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 21:
#line 130 "src/parser/verilog/verilog_parser.yy"
                                  { yylhs.value.as < prs::verilog::ast::net_decl_stmt > () = {"", std::move(yystack_[1].value.as < std::string > ()), std::move(yystack_[0].value.as < prs::verilog::ast::multi_var_decl > ())}; }
#line 1303 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 22:
#line 131 "src/parser/verilog/verilog_parser.yy"
                             { yylhs.value.as < prs::verilog::ast::net_decl_stmt > () = {std::move(yystack_[1].value.as < std::string > ()), "", std::move(yystack_[0].value.as < prs::verilog::ast::multi_var_decl > ())}; }
#line 1309 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 23:
#line 132 "src/parser/verilog/verilog_parser.yy"
                                         { yylhs.value.as < prs::verilog::ast::net_decl_stmt > () = {std::move(yystack_[2].value.as < std::string > ()), std::move(yystack_[1].value.as < std::string > ()), std::move(yystack_[0].value.as < prs::verilog::ast::multi_var_decl > ())}; }
#line 1315 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 24:
#line 136 "src/parser/verilog/verilog_parser.yy"
                { yylhs.value.as < prs::verilog::ast::var_decl > () = yystack_[0].value.as < std::string > ();  }
#line 1321 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 25:
#line 137 "src/parser/verilog/verilog_parser.yy"
                          { yylhs.value.as < prs::verilog::ast::var_decl > () = yystack_[0].value.as < prs::verilog::ast::ranged_var_decl > (); }
#line 1327 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 26:
#line 141 "src/parser/verilog/verilog_parser.yy"
                      { yylhs.value.as < prs::verilog::ast::ranged_var_decl > () = {std::move(yystack_[1].value.as < prs::verilog::ast::range > ()), std::move(yystack_[0].value.as < std::string > ())}; }
#line 1333 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 27:
#line 145 "src/parser/verilog/verilog_parser.yy"
                     { yylhs.value.as < prs::verilog::ast::multi_var_decl > ().var_names = yystack_[0].value.as < prs::verilog::ast::ident_list > (); }
#line 1339 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 28:
#line 146 "src/parser/verilog/verilog_parser.yy"
                           { yylhs.value.as < prs::verilog::ast::multi_var_decl > () = {std::move(yystack_[1].value.as < prs::verilog::ast::range > ()), std::move(yystack_[0].value.as < prs::verilog::ast::ident_list > ())}; }
#line 1345 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 29:
#line 150 "src/parser/verilog/verilog_parser.yy"
                                                     { yylhs.value.as < prs::verilog::ast::instance_stmt > () = {std::move(yystack_[5].value.as < std::string > ()), std::move(yystack_[4].value.as < prs::verilog::ast::param_config > ()), std::move(yystack_[3].value.as < std::string > ()), std::move(yystack_[1].value.as < prs::verilog::ast::expr_list > ())}; }
#line 1351 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 30:
#line 151 "src/parser/verilog/verilog_parser.yy"
                                                          { yylhs.value.as < prs::verilog::ast::instance_stmt > () = {std::move(yystack_[5].value.as < std::string > ()), std::move(yystack_[4].value.as < prs::verilog::ast::param_config > ()), std::move(yystack_[3].value.as < std::string > ()), std::move(yystack_[1].value.as < prs::verilog::ast::conn_pair_list > ())}; }
#line 1357 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 31:
#line 156 "src/parser/verilog/verilog_parser.yy"
               {}
#line 1363 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 32:
#line 157 "src/parser/verilog/verilog_parser.yy"
                                  { yylhs.value.as < prs::verilog::ast::param_config > () = std::move(yystack_[1].value.as < prs::verilog::ast::expr_list > ()); }
#line 1369 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 33:
#line 158 "src/parser/verilog/verilog_parser.yy"
                                       { yylhs.value.as < prs::verilog::ast::param_config > () = std::move(yystack_[1].value.as < prs::verilog::ast::conn_pair_list > ()); }
#line 1375 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 34:
#line 163 "src/parser/verilog/verilog_parser.yy"
                  { yylhs.value.as < prs::verilog::ast::constant_expr > () = std::to_string(yystack_[0].value.as < prs::verilog::ast::integer > ()); }
#line 1381 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 35:
#line 164 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::constant_expr > () = std::move(yystack_[0].value.as < prs::verilog::ast::bin_constant > ()); }
#line 1387 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 36:
#line 165 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::constant_expr > () = std::move(yystack_[0].value.as < prs::verilog::ast::dec_constant > ()); }
#line 1393 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 37:
#line 166 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::constant_expr > () = std::move(yystack_[0].value.as < prs::verilog::ast::hex_constant > ()); }
#line 1399 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 38:
#line 167 "src/parser/verilog/verilog_parser.yy"
                         { yylhs.value.as < prs::verilog::ast::constant_expr > () = std::move(yystack_[0].value.as < std::string > ()); }
#line 1405 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 39:
#line 171 "src/parser/verilog/verilog_parser.yy"
                                                    { yylhs.value.as < prs::verilog::ast::prim_gate_stmt > () = {std::move(yystack_[3].value.as < prs::verilog::ast::expr > ()), std::move(yystack_[6].value.as < std::string > ()), std::move(yystack_[5].value.as < std::string > ()), std::move(yystack_[1].value.as < prs::verilog::ast::expr_list > ())}; }
#line 1411 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 40:
#line 175 "src/parser/verilog/verilog_parser.yy"
                                 { yylhs.value.as < prs::verilog::ast::assignment_stmt > () = {std::move(yystack_[2].value.as < prs::verilog::ast::expr > ()), std::move(yystack_[0].value.as < prs::verilog::ast::expr > ())}; }
#line 1417 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 41:
#line 179 "src/parser/verilog/verilog_parser.yy"
               { yylhs.value.as < prs::verilog::ast::expr_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::expr > ()); }
#line 1423 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 42:
#line 180 "src/parser/verilog/verilog_parser.yy"
                             { yystack_[2].value.as < prs::verilog::ast::expr_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::expr > ()); yylhs.value.as < prs::verilog::ast::expr_list > () = std::move(yystack_[2].value.as < prs::verilog::ast::expr_list > ()); }
#line 1429 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 43:
#line 184 "src/parser/verilog/verilog_parser.yy"
                { yylhs.value.as < prs::verilog::ast::expr > () = {yystack_[0].value.as < std::string > ()}; }
#line 1435 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 44:
#line 185 "src/parser/verilog/verilog_parser.yy"
                      { yylhs.value.as < prs::verilog::ast::expr > () = {yystack_[0].value.as < prs::verilog::ast::ranged_expr > ()}; }
#line 1441 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 45:
#line 186 "src/parser/verilog/verilog_parser.yy"
                       { yylhs.value.as < prs::verilog::ast::expr > () = {yystack_[0].value.as < prs::verilog::ast::indexed_expr > ()}; }
#line 1447 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 46:
#line 187 "src/parser/verilog/verilog_parser.yy"
                      { yylhs.value.as < prs::verilog::ast::expr > () = {yystack_[0].value.as < prs::verilog::ast::concat_expr > ()}; }
#line 1453 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 47:
#line 188 "src/parser/verilog/verilog_parser.yy"
                        { yylhs.value.as < prs::verilog::ast::expr > () = {yystack_[0].value.as < prs::verilog::ast::constant_expr > ()}; }
#line 1459 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 48:
#line 191 "src/parser/verilog/verilog_parser.yy"
                          { yylhs.value.as < prs::verilog::ast::ranged_expr > () = {std::move(yystack_[0].value.as < prs::verilog::ast::range > ()), std::move(yystack_[1].value.as < std::string > ())}; }
#line 1465 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 49:
#line 194 "src/parser/verilog/verilog_parser.yy"
                                     { 
		if (yystack_[1].value.as < prs::verilog::ast::integer > () < 0)
			throw vv::parser::syntax_error(yystack_[1].location, "cannot use negative index");
		yylhs.value.as < prs::verilog::ast::indexed_expr > () = {yystack_[1].value.as < prs::verilog::ast::integer > (), yystack_[3].value.as < std::string > ()};
		}
#line 1475 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 50:
#line 202 "src/parser/verilog/verilog_parser.yy"
                             { yylhs.value.as < prs::verilog::ast::concat_expr > () = {yystack_[1].value.as < prs::verilog::ast::expr_list > ()}; }
#line 1481 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 51:
#line 205 "src/parser/verilog/verilog_parser.yy"
                                    { 
		if (yystack_[3].value.as < prs::verilog::ast::integer > () < 0 || yystack_[1].value.as < prs::verilog::ast::integer > () < 0) 
			throw vv::parser::syntax_error(yystack_[3].location, "negative range is invalid");
		yylhs.value.as < prs::verilog::ast::range > () = {std::move(yystack_[3].value.as < prs::verilog::ast::integer > ()), std::move(yystack_[1].value.as < prs::verilog::ast::integer > ())}; 
		}
#line 1491 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 52:
#line 212 "src/parser/verilog/verilog_parser.yy"
                                   { yylhs.value.as < prs::verilog::ast::conn_pair > () = {std::move(yystack_[3].value.as < std::string > ()), std::move(yystack_[1].value.as < prs::verilog::ast::expr > ())}; }
#line 1497 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 53:
#line 216 "src/parser/verilog/verilog_parser.yy"
                    { yylhs.value.as < prs::verilog::ast::conn_pair_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::conn_pair > ()); }
#line 1503 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 54:
#line 217 "src/parser/verilog/verilog_parser.yy"
                                       { yystack_[2].value.as < prs::verilog::ast::conn_pair_list > ().push_back(yystack_[0].value.as < prs::verilog::ast::conn_pair > ()); yylhs.value.as < prs::verilog::ast::conn_pair_list > () = std::move(yystack_[2].value.as < prs::verilog::ast::conn_pair_list > ()); }
#line 1509 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 55:
#line 221 "src/parser/verilog/verilog_parser.yy"
          { yylhs.value.as < prs::verilog::ast::ident_list > ().push_back(yystack_[0].value.as < std::string > ()); }
#line 1515 "src/parser/verilog/verilog_parser.tab.cc"
    break;

  case 56:
#line 222 "src/parser/verilog/verilog_parser.yy"
                           { yystack_[2].value.as < prs::verilog::ast::ident_list > ().push_back(yystack_[0].value.as < std::string > ()); yylhs.value.as < prs::verilog::ast::ident_list > () = std::move(yystack_[2].value.as < prs::verilog::ast::ident_list > ()); }
#line 1521 "src/parser/verilog/verilog_parser.tab.cc"
    break;


#line 1525 "src/parser/verilog/verilog_parser.tab.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yy_error_token_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yy_error_token_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = static_cast<state_type> (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    std::ptrdiff_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */
    if (!yyla.empty ())
      {
        symbol_number_type yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];

        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yy_error_token_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -63;

  const signed char parser::yytable_ninf_ = -1;

  const signed char
  parser::yypact_[] =
  {
      -5,     1,     7,   -63,    42,    13,   -63,   -63,   -63,    26,
      29,    27,     9,     0,    51,    35,    43,    45,    46,    52,
       8,    26,    59,   -63,   -63,   -63,   -63,   -63,   -63,    10,
     -63,   -63,   -63,    79,    66,    81,    64,   -63,   -63,    73,
      77,     9,   -63,   -63,    35,   -63,   -63,   -63,   -63,   -63,
     -63,    24,    74,    36,   -63,   -63,   -63,    75,    -1,   -63,
      67,   -63,    26,    14,    84,    26,    82,    77,    78,   -63,
     -63,    24,   -63,     8,   -63,   -63,    26,     3,   -63,    80,
      68,   -63,    69,    14,    87,    83,   -63,   -63,   -63,   -63,
     -63,    88,   -63,   -63,    89,    70,    71,    26,    92,    26,
     -63,   -63,   -63,    72,   -63,    96,   -63,   -63
  };

  const signed char
  parser::yydefact_[] =
  {
       0,     0,     0,     2,    13,     5,     1,     3,     7,     0,
      31,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,    43,    34,    37,    35,    36,    38,    47,     0,
      44,    45,    46,     0,     0,     0,     0,    55,    21,     0,
      27,     0,    22,     4,    15,    16,    18,    19,    17,    20,
      24,     0,     0,     0,     8,    10,    25,     0,     0,    41,
       0,    48,     0,     0,     0,     0,     0,    28,     0,    23,
      11,     0,     6,     0,    26,    50,     0,     0,    40,     0,
       0,    53,     0,     0,     0,     0,    56,    12,     9,    42,
      49,     0,    32,    33,     0,     0,     0,     0,     0,     0,
      54,    29,    30,     0,    51,     0,    39,    52
  };

  const signed char
  parser::yypgoto_[] =
  {
     -63,   -63,   100,   -63,   -63,    30,   -63,    90,   -63,   -47,
     -63,    -8,   -63,   -63,   -63,   -63,   -63,   -62,    -9,   -63,
     -63,   -63,   -10,    12,    25,    76
  };

  const signed char
  parser::yydefgoto_[] =
  {
      -1,     2,     3,     4,    53,    54,    14,    15,    16,    55,
      56,    38,    17,    34,    28,    18,    19,    58,    59,    30,
      31,    32,    57,    81,    82,    40
  };

  const signed char
  parser::yytable_[] =
  {
      29,    80,    39,    39,    70,    42,    36,     6,    75,    76,
      90,     1,    61,    62,    36,    36,    85,    20,    37,     5,
      41,    95,    21,     1,    87,    79,    50,    37,    51,    52,
      36,    39,    22,    69,    21,   103,    23,    24,    25,    26,
      27,    72,    50,    33,    22,    35,    73,    45,    23,    24,
      25,    26,    27,    78,     8,    46,    84,    47,    48,     9,
      10,    11,    12,    13,    49,    60,    43,    89,     9,    10,
      11,    12,    13,    92,    93,   101,   102,   106,    76,    94,
      76,    94,    76,    63,    64,    65,    66,    68,    83,    77,
     105,    37,    99,    74,    71,    85,    86,    97,    91,   104,
      79,   107,     7,    88,    44,    98,   100,     0,    96,     0,
       0,     0,     0,     0,     0,    67
  };

  const signed char
  parser::yycheck_[] =
  {
       9,    63,    12,    13,    51,    13,     6,     0,     9,    10,
       7,    16,    22,     3,     6,     6,    13,     4,    18,    18,
      20,    83,     8,    16,    71,    11,    18,    18,    20,    21,
       6,    41,    18,    41,     8,    97,    22,    23,    24,    25,
      26,     5,    18,    14,    18,    18,    10,    12,    22,    23,
      24,    25,    26,    62,    12,    12,    65,    12,    12,    17,
      18,    19,    20,    21,    12,     6,    15,    76,    17,    18,
      19,    20,    21,     5,     5,     5,     5,     5,    10,    10,
      10,    10,    10,     4,    18,     4,    22,    10,     4,    22,
      99,    18,     4,    18,    20,    13,    18,    10,    18,     7,
      11,     5,     2,    73,    14,    22,    94,    -1,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    39
  };

  const signed char
  parser::yystos_[] =
  {
       0,    16,    28,    29,    30,    18,     0,    29,    12,    17,
      18,    19,    20,    21,    33,    34,    35,    39,    42,    43,
       4,     8,    18,    22,    23,    24,    25,    26,    41,    45,
      46,    47,    48,    14,    40,    18,     6,    18,    38,    49,
      52,    20,    38,    15,    34,    12,    12,    12,    12,    12,
      18,    20,    21,    31,    32,    36,    37,    49,    44,    45,
       6,    49,     3,     4,    18,     4,    22,    52,    10,    38,
      36,    20,     5,    10,    18,     9,    10,    22,    45,    11,
      44,    50,    51,     4,    45,    13,    18,    36,    32,    45,
       7,    18,     5,     5,    10,    44,    51,    10,    22,     4,
      50,     5,     5,    44,     7,    45,     5,     5
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    27,    28,    28,    29,    30,    30,    30,    31,    31,
      32,    32,    32,    33,    33,    33,    34,    34,    34,    34,
      34,    35,    35,    35,    36,    36,    37,    38,    38,    39,
      39,    40,    40,    40,    41,    41,    41,    41,    41,    42,
      43,    44,    44,    45,    45,    45,    45,    45,    46,    47,
      48,    49,    50,    51,    51,    52,    52
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     1,     2,     3,     2,     5,     2,     1,     3,
       1,     2,     3,     0,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     1,     1,     2,     1,     2,     6,
       6,     0,     4,     4,     1,     1,     1,     1,     1,     7,
       4,     1,     3,     1,     1,     1,     1,     1,     2,     4,
       3,     5,     5,     1,     3,     1,     3
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "$undefined", "\"=\"", "\"(\"", "\")\"",
  "\"[\"", "\"]\"", "\"{\"", "\"}\"", "\",\"", "\".\"", "\";\"", "\":\"",
  "\"#\"", "ENDMODULEKW", "MODULEKW", "ASSIGNKW", "\"ident\"",
  "\"primitive_gate\"", "\"variable_attribute\"", "\"port_direction\"",
  "INTEGER", "\"hex_constant\"", "\"bin_constant\"", "\"dec_constant\"",
  "\"str_constant\"", "$accept", "verilog", "module", "module_header",
  "module_port_decl_list", "module_port_decl", "statement_list",
  "statement", "net_decl_stmt", "var_decl", "ranged_var_decl",
  "multi_var_decl", "instance_stmt", "param_config", "constant_expr",
  "prim_gate_stmt", "assignment_stmt", "expr_list", "expr", "ranged_expr",
  "indexed_expr", "concat_expr", "range", "conn_pair", "conn_pair_list",
  "ident_list", YY_NULLPTR
  };

#if VVDEBUG
  const unsigned char
  parser::yyrline_[] =
  {
       0,    91,    91,    92,    95,    99,   100,   101,   105,   106,
     110,   111,   112,   116,   117,   118,   122,   123,   124,   125,
     126,   130,   131,   132,   136,   137,   141,   145,   146,   150,
     151,   156,   157,   158,   163,   164,   165,   166,   167,   171,
     175,   179,   180,   184,   185,   186,   187,   188,   191,   194,
     202,   205,   212,   216,   217,   221,   222
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // VVDEBUG


} // vv
#line 1984 "src/parser/verilog/verilog_parser.tab.cc"

#line 225 "src/parser/verilog/verilog_parser.yy"


void 
vv::parser::error (const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}

