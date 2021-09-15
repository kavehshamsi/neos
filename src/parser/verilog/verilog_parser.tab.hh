// A Bison parser, made by GNU Bison 3.5.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file src/parser/verilog/verilog_parser.tab.hh
 ** Define the vv::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.

#ifndef YY_VV_SRC_PARSER_VERILOG_VERILOG_PARSER_TAB_HH_INCLUDED
# define YY_VV_SRC_PARSER_VERILOG_VERILOG_PARSER_TAB_HH_INCLUDED
// "%code requires" blocks.
#line 9 "src/parser/verilog/verilog_parser.yy"

  #include<string> 
  #include<utility>
  #include "verilog_ast.h"
  namespace prs {
  namespace verilog {
  class readverilog;
  }
  }

#line 59 "src/parser/verilog/verilog_parser.tab.hh"

# include <cassert>
# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif

#include <typeinfo>
#ifndef YY_ASSERT
# include <cassert>
# define YY_ASSERT assert
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef VVDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define VVDEBUG 1
#  else
#   define VVDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define VVDEBUG 1
# endif /* ! defined YYDEBUG */
#endif  /* ! defined VVDEBUG */

namespace vv {
#line 201 "src/parser/verilog/verilog_parser.tab.hh"


  /// A point in a source file.
  class position
  {
  public:
    /// Type for line and column numbers.
    typedef int counter_type;

    /// Construct a position.
    explicit position (std::string* f = YY_NULLPTR,
                       counter_type l = 1,
                       counter_type c = 1)
      : filename (f)
      , line (l)
      , column (c)
    {}


    /// Initialization.
    void initialize (std::string* fn = YY_NULLPTR,
                     counter_type l = 1,
                     counter_type c = 1)
    {
      filename = fn;
      line = l;
      column = c;
    }

    /** \name Line and Column related manipulators
     ** \{ */
    /// (line related) Advance to the COUNT next lines.
    void lines (counter_type count = 1)
    {
      if (count)
        {
          column = 1;
          line = add_ (line, count, 1);
        }
    }

    /// (column related) Advance to the COUNT next columns.
    void columns (counter_type count = 1)
    {
      column = add_ (column, count, 1);
    }
    /** \} */

    /// File name to which this position refers.
    std::string* filename;
    /// Current line number.
    counter_type line;
    /// Current column number.
    counter_type column;

  private:
    /// Compute max (min, lhs+rhs).
    static counter_type add_ (counter_type lhs, counter_type rhs, counter_type min)
    {
      return lhs + rhs < min ? min : lhs + rhs;
    }
  };

  /// Add \a width columns, in place.
  inline position&
  operator+= (position& res, position::counter_type width)
  {
    res.columns (width);
    return res;
  }

  /// Add \a width columns.
  inline position
  operator+ (position res, position::counter_type width)
  {
    return res += width;
  }

  /// Subtract \a width columns, in place.
  inline position&
  operator-= (position& res, position::counter_type width)
  {
    return res += -width;
  }

  /// Subtract \a width columns.
  inline position
  operator- (position res, position::counter_type width)
  {
    return res -= width;
  }

  /// Compare two position objects.
  inline bool
  operator== (const position& pos1, const position& pos2)
  {
    return (pos1.line == pos2.line
            && pos1.column == pos2.column
            && (pos1.filename == pos2.filename
                || (pos1.filename && pos2.filename
                    && *pos1.filename == *pos2.filename)));
  }

  /// Compare two position objects.
  inline bool
  operator!= (const position& pos1, const position& pos2)
  {
    return !(pos1 == pos2);
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param pos a reference to the position to redirect
   */
  template <typename YYChar>
  std::basic_ostream<YYChar>&
  operator<< (std::basic_ostream<YYChar>& ostr, const position& pos)
  {
    if (pos.filename)
      ostr << *pos.filename << ':';
    return ostr << pos.line << '.' << pos.column;
  }

  /// Two points in a source file.
  class location
  {
  public:
    /// Type for line and column numbers.
    typedef position::counter_type counter_type;

    /// Construct a location from \a b to \a e.
    location (const position& b, const position& e)
      : begin (b)
      , end (e)
    {}

    /// Construct a 0-width location in \a p.
    explicit location (const position& p = position ())
      : begin (p)
      , end (p)
    {}

    /// Construct a 0-width location in \a f, \a l, \a c.
    explicit location (std::string* f,
                       counter_type l = 1,
                       counter_type c = 1)
      : begin (f, l, c)
      , end (f, l, c)
    {}


    /// Initialization.
    void initialize (std::string* f = YY_NULLPTR,
                     counter_type l = 1,
                     counter_type c = 1)
    {
      begin.initialize (f, l, c);
      end = begin;
    }

    /** \name Line and Column related manipulators
     ** \{ */
  public:
    /// Reset initial location to final location.
    void step ()
    {
      begin = end;
    }

    /// Extend the current location to the COUNT next columns.
    void columns (counter_type count = 1)
    {
      end += count;
    }

    /// Extend the current location to the COUNT next lines.
    void lines (counter_type count = 1)
    {
      end.lines (count);
    }
    /** \} */


  public:
    /// Beginning of the located region.
    position begin;
    /// End of the located region.
    position end;
  };

  /// Join two locations, in place.
  inline location&
  operator+= (location& res, const location& end)
  {
    res.end = end.end;
    return res;
  }

  /// Join two locations.
  inline location
  operator+ (location res, const location& end)
  {
    return res += end;
  }

  /// Add \a width columns to the end position, in place.
  inline location&
  operator+= (location& res, location::counter_type width)
  {
    res.columns (width);
    return res;
  }

  /// Add \a width columns to the end position.
  inline location
  operator+ (location res, location::counter_type width)
  {
    return res += width;
  }

  /// Subtract \a width columns to the end position, in place.
  inline location&
  operator-= (location& res, location::counter_type width)
  {
    return res += -width;
  }

  /// Subtract \a width columns to the end position.
  inline location
  operator- (location res, location::counter_type width)
  {
    return res -= width;
  }

  /// Compare two location objects.
  inline bool
  operator== (const location& loc1, const location& loc2)
  {
    return loc1.begin == loc2.begin && loc1.end == loc2.end;
  }

  /// Compare two location objects.
  inline bool
  operator!= (const location& loc1, const location& loc2)
  {
    return !(loc1 == loc2);
  }

  /** \brief Intercept output stream redirection.
   ** \param ostr the destination output stream
   ** \param loc a reference to the location to redirect
   **
   ** Avoid duplicate information.
   */
  template <typename YYChar>
  std::basic_ostream<YYChar>&
  operator<< (std::basic_ostream<YYChar>& ostr, const location& loc)
  {
    location::counter_type end_col
      = 0 < loc.end.column ? loc.end.column - 1 : 0;
    ostr << loc.begin;
    if (loc.end.filename
        && (!loc.begin.filename
            || *loc.begin.filename != *loc.end.filename))
      ostr << '-' << loc.end.filename << ':' << loc.end.line << '.' << end_col;
    else if (loc.begin.line < loc.end.line)
      ostr << '-' << loc.end.line << '.' << end_col;
    else if (loc.begin.column < end_col)
      ostr << '-' << end_col;
    return ostr;
  }


  /// A Bison parser.
  class parser
  {
  public:
#ifndef VVSTYPE
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class semantic_type
  {
  public:
    /// Type of *this.
    typedef semantic_type self_type;

    /// Empty construction.
    semantic_type () YY_NOEXCEPT
      : yybuffer_ ()
      , yytypeid_ (YY_NULLPTR)
    {}

    /// Construct and fill.
    template <typename T>
    semantic_type (YY_RVREF (T) t)
      : yytypeid_ (&typeid (T))
    {
      YY_ASSERT (sizeof (T) <= size);
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

    /// Destruction, allowed only if empty.
    ~semantic_type () YY_NOEXCEPT
    {
      YY_ASSERT (!yytypeid_);
    }

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == *that.yytypeid_);
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
      yytypeid_ = YY_NULLPTR;
    }

  private:
    /// Prohibit blind copies.
    self_type& operator= (const self_type&);
    semantic_type (const self_type&);

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yybuffer_.yyraw;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yybuffer_.yyraw;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // assignment_stmt
      char dummy1[sizeof (prs::verilog::ast::assignment_stmt)];

      // "bin_constant"
      char dummy2[sizeof (prs::verilog::ast::bin_constant)];

      // concat_expr
      char dummy3[sizeof (prs::verilog::ast::concat_expr)];

      // conn_pair
      char dummy4[sizeof (prs::verilog::ast::conn_pair)];

      // conn_pair_list
      char dummy5[sizeof (prs::verilog::ast::conn_pair_list)];

      // constant_expr
      char dummy6[sizeof (prs::verilog::ast::constant_expr)];

      // "dec_constant"
      char dummy7[sizeof (prs::verilog::ast::dec_constant)];

      // expr
      char dummy8[sizeof (prs::verilog::ast::expr)];

      // expr_list
      char dummy9[sizeof (prs::verilog::ast::expr_list)];

      // "hex_constant"
      char dummy10[sizeof (prs::verilog::ast::hex_constant)];

      // ident_list
      char dummy11[sizeof (prs::verilog::ast::ident_list)];

      // indexed_expr
      char dummy12[sizeof (prs::verilog::ast::indexed_expr)];

      // instance_stmt
      char dummy13[sizeof (prs::verilog::ast::instance_stmt)];

      // INTEGER
      char dummy14[sizeof (prs::verilog::ast::integer)];

      // module
      char dummy15[sizeof (prs::verilog::ast::module)];

      // module_header
      char dummy16[sizeof (prs::verilog::ast::module_header)];

      // module_port_decl
      char dummy17[sizeof (prs::verilog::ast::module_port_decl)];

      // module_port_decl_list
      char dummy18[sizeof (prs::verilog::ast::module_port_decl_list)];

      // multi_var_decl
      char dummy19[sizeof (prs::verilog::ast::multi_var_decl)];

      // net_decl_stmt
      char dummy20[sizeof (prs::verilog::ast::net_decl_stmt)];

      // param_config
      char dummy21[sizeof (prs::verilog::ast::param_config)];

      // prim_gate_stmt
      char dummy22[sizeof (prs::verilog::ast::prim_gate_stmt)];

      // range
      char dummy23[sizeof (prs::verilog::ast::range)];

      // ranged_expr
      char dummy24[sizeof (prs::verilog::ast::ranged_expr)];

      // ranged_var_decl
      char dummy25[sizeof (prs::verilog::ast::ranged_var_decl)];

      // statement
      char dummy26[sizeof (prs::verilog::ast::statement)];

      // statement_list
      char dummy27[sizeof (prs::verilog::ast::statement_list)];

      // var_decl
      char dummy28[sizeof (prs::verilog::ast::var_decl)];

      // verilog
      char dummy29[sizeof (prs::verilog::ast::verilog)];

      // "ident"
      // "primitive_gate"
      // "variable_attribute"
      // "port_direction"
      // "str_constant"
      char dummy30[sizeof (std::string)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me;
      /// A buffer large enough to store any of the semantic values.
      char yyraw[size];
    } yybuffer_;

    /// Whether the content is built: if defined, the name of the stored type.
    const std::type_info *yytypeid_;
  };

#else
    typedef VVSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Tokens.
    struct token
    {
      enum yytokentype
      {
        TOK_END = 0,
        TOK_ASSIGN = 258,
        TOK_LPAREN = 259,
        TOK_RPAREN = 260,
        TOK_LBRACE = 261,
        TOK_RBRACE = 262,
        TOK_CLBRACE = 263,
        TOK_CRBRACE = 264,
        TOK_COMMA = 265,
        TOK_DOT = 266,
        TOK_SEMICOLON = 267,
        TOK_COLON = 268,
        TOK_POUND = 269,
        TOK_ENDMODULEKW = 270,
        TOK_MODULEKW = 271,
        TOK_ASSIGNKW = 272,
        TOK_IDENT = 273,
        TOK_PRIMGATE = 274,
        TOK_PORTATT = 275,
        TOK_PORTDIR = 276,
        TOK_INTEGER = 277,
        TOK_CONSTHEX = 278,
        TOK_CONSTBIN = 279,
        TOK_CONSTDEC = 280,
        TOK_CONSTSTR = 281
      };
    };

    /// (External) token type, as returned by yylex.
    typedef token::yytokentype token_type;

    /// Symbol type: an internal symbol number.
    typedef int symbol_number_type;

    /// The symbol type number to denote an empty symbol.
    enum { empty_symbol = -2 };

    /// Internal symbol number for tokens (subsumed by symbol_number_type).
    typedef signed char token_number_type;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol type
    /// via type_get ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ()
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that);
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::assignment_stmt&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::assignment_stmt& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::bin_constant&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::bin_constant& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::concat_expr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::concat_expr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::conn_pair&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::conn_pair& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::conn_pair_list&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::conn_pair_list& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::constant_expr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::constant_expr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::dec_constant&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::dec_constant& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::expr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::expr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::expr_list&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::expr_list& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::hex_constant&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::hex_constant& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::ident_list&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::ident_list& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::indexed_expr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::indexed_expr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::instance_stmt&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::instance_stmt& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::integer&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::integer& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::module&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::module& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::module_header&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::module_header& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::module_port_decl&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::module_port_decl& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::module_port_decl_list&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::module_port_decl_list& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::multi_var_decl&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::multi_var_decl& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::net_decl_stmt&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::net_decl_stmt& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::param_config&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::param_config& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::prim_gate_stmt&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::prim_gate_stmt& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::range&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::range& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::ranged_expr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::ranged_expr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::ranged_var_decl&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::ranged_var_decl& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::statement&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::statement& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::statement_list&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::statement_list& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::var_decl&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::var_decl& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, prs::verilog::ast::verilog&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const prs::verilog::ast::verilog& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }

      /// Destroy contents, and record that is empty.
      void clear ()
      {
        // User destructor.
        symbol_number_type yytype = this->type_get ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yytype)
        {
       default:
          break;
        }

        // Type destructor.
switch (yytype)
    {
      case 43: // assignment_stmt
        value.template destroy< prs::verilog::ast::assignment_stmt > ();
        break;

      case 24: // "bin_constant"
        value.template destroy< prs::verilog::ast::bin_constant > ();
        break;

      case 48: // concat_expr
        value.template destroy< prs::verilog::ast::concat_expr > ();
        break;

      case 50: // conn_pair
        value.template destroy< prs::verilog::ast::conn_pair > ();
        break;

      case 51: // conn_pair_list
        value.template destroy< prs::verilog::ast::conn_pair_list > ();
        break;

      case 41: // constant_expr
        value.template destroy< prs::verilog::ast::constant_expr > ();
        break;

      case 25: // "dec_constant"
        value.template destroy< prs::verilog::ast::dec_constant > ();
        break;

      case 45: // expr
        value.template destroy< prs::verilog::ast::expr > ();
        break;

      case 44: // expr_list
        value.template destroy< prs::verilog::ast::expr_list > ();
        break;

      case 23: // "hex_constant"
        value.template destroy< prs::verilog::ast::hex_constant > ();
        break;

      case 52: // ident_list
        value.template destroy< prs::verilog::ast::ident_list > ();
        break;

      case 47: // indexed_expr
        value.template destroy< prs::verilog::ast::indexed_expr > ();
        break;

      case 39: // instance_stmt
        value.template destroy< prs::verilog::ast::instance_stmt > ();
        break;

      case 22: // INTEGER
        value.template destroy< prs::verilog::ast::integer > ();
        break;

      case 29: // module
        value.template destroy< prs::verilog::ast::module > ();
        break;

      case 30: // module_header
        value.template destroy< prs::verilog::ast::module_header > ();
        break;

      case 32: // module_port_decl
        value.template destroy< prs::verilog::ast::module_port_decl > ();
        break;

      case 31: // module_port_decl_list
        value.template destroy< prs::verilog::ast::module_port_decl_list > ();
        break;

      case 38: // multi_var_decl
        value.template destroy< prs::verilog::ast::multi_var_decl > ();
        break;

      case 35: // net_decl_stmt
        value.template destroy< prs::verilog::ast::net_decl_stmt > ();
        break;

      case 40: // param_config
        value.template destroy< prs::verilog::ast::param_config > ();
        break;

      case 42: // prim_gate_stmt
        value.template destroy< prs::verilog::ast::prim_gate_stmt > ();
        break;

      case 49: // range
        value.template destroy< prs::verilog::ast::range > ();
        break;

      case 46: // ranged_expr
        value.template destroy< prs::verilog::ast::ranged_expr > ();
        break;

      case 37: // ranged_var_decl
        value.template destroy< prs::verilog::ast::ranged_var_decl > ();
        break;

      case 34: // statement
        value.template destroy< prs::verilog::ast::statement > ();
        break;

      case 33: // statement_list
        value.template destroy< prs::verilog::ast::statement_list > ();
        break;

      case 36: // var_decl
        value.template destroy< prs::verilog::ast::var_decl > ();
        break;

      case 28: // verilog
        value.template destroy< prs::verilog::ast::verilog > ();
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.template destroy< std::string > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_type
    {
      /// Default constructor.
      by_type ();

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_type (by_type&& that);
#endif

      /// Copy constructor.
      by_type (const by_type& that);

      /// The symbol type as needed by the constructor.
      typedef token_type kind_type;

      /// Constructor from (external) token numbers.
      by_type (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol type from \a that.
      void move (by_type& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The symbol type.
      /// \a empty_symbol when empty.
      /// An int, not token_number_type, to be able to store empty_symbol.
      int type;
    };

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_type>
    {
      /// Superclass.
      typedef basic_symbol<by_type> super_type;

      /// Empty symbol.
      symbol_type () {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type(token_type (tok), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_END || tok == token::TOK_ASSIGN || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_CLBRACE || tok == token::TOK_CRBRACE || tok == token::TOK_COMMA || tok == token::TOK_DOT || tok == token::TOK_SEMICOLON || tok == token::TOK_COLON || tok == token::TOK_POUND || tok == token::TOK_ENDMODULEKW || tok == token::TOK_MODULEKW || tok == token::TOK_ASSIGNKW);
      }
#else
      symbol_type (int tok, const location_type& l)
        : super_type(token_type (tok), l)
      {
        YY_ASSERT (tok == token::TOK_END || tok == token::TOK_ASSIGN || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_CLBRACE || tok == token::TOK_CRBRACE || tok == token::TOK_COMMA || tok == token::TOK_DOT || tok == token::TOK_SEMICOLON || tok == token::TOK_COLON || tok == token::TOK_POUND || tok == token::TOK_ENDMODULEKW || tok == token::TOK_MODULEKW || tok == token::TOK_ASSIGNKW);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, prs::verilog::ast::bin_constant v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_CONSTBIN);
      }
#else
      symbol_type (int tok, const prs::verilog::ast::bin_constant& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_CONSTBIN);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, prs::verilog::ast::dec_constant v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_CONSTDEC);
      }
#else
      symbol_type (int tok, const prs::verilog::ast::dec_constant& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_CONSTDEC);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, prs::verilog::ast::hex_constant v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_CONSTHEX);
      }
#else
      symbol_type (int tok, const prs::verilog::ast::hex_constant& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_CONSTHEX);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, prs::verilog::ast::integer v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_INTEGER);
      }
#else
      symbol_type (int tok, const prs::verilog::ast::integer& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_INTEGER);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, std::string v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_IDENT || tok == token::TOK_PRIMGATE || tok == token::TOK_PORTATT || tok == token::TOK_PORTDIR || tok == token::TOK_CONSTSTR);
      }
#else
      symbol_type (int tok, const std::string& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_IDENT || tok == token::TOK_PRIMGATE || tok == token::TOK_PORTATT || tok == token::TOK_PORTDIR || tok == token::TOK_CONSTSTR);
      }
#endif
    };

    /// Build a parser object.
    parser (prs::verilog::readverilog& drv_yyarg);
    virtual ~parser ();

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if VVDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    // Implementation of make_symbol for each symbol type.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::TOK_END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::TOK_END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (location_type l)
      {
        return symbol_type (token::TOK_ASSIGN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const location_type& l)
      {
        return symbol_type (token::TOK_ASSIGN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (location_type l)
      {
        return symbol_type (token::TOK_LPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_LPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (location_type l)
      {
        return symbol_type (token::TOK_RPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_RPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACE (location_type l)
      {
        return symbol_type (token::TOK_LBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACE (location_type l)
      {
        return symbol_type (token::TOK_RBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CLBRACE (location_type l)
      {
        return symbol_type (token::TOK_CLBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_CLBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_CLBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CRBRACE (location_type l)
      {
        return symbol_type (token::TOK_CRBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_CRBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_CRBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::TOK_COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::TOK_COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOT (location_type l)
      {
        return symbol_type (token::TOK_DOT, std::move (l));
      }
#else
      static
      symbol_type
      make_DOT (const location_type& l)
      {
        return symbol_type (token::TOK_DOT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMICOLON (location_type l)
      {
        return symbol_type (token::TOK_SEMICOLON, std::move (l));
      }
#else
      static
      symbol_type
      make_SEMICOLON (const location_type& l)
      {
        return symbol_type (token::TOK_SEMICOLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COLON (location_type l)
      {
        return symbol_type (token::TOK_COLON, std::move (l));
      }
#else
      static
      symbol_type
      make_COLON (const location_type& l)
      {
        return symbol_type (token::TOK_COLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_POUND (location_type l)
      {
        return symbol_type (token::TOK_POUND, std::move (l));
      }
#else
      static
      symbol_type
      make_POUND (const location_type& l)
      {
        return symbol_type (token::TOK_POUND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ENDMODULEKW (location_type l)
      {
        return symbol_type (token::TOK_ENDMODULEKW, std::move (l));
      }
#else
      static
      symbol_type
      make_ENDMODULEKW (const location_type& l)
      {
        return symbol_type (token::TOK_ENDMODULEKW, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MODULEKW (location_type l)
      {
        return symbol_type (token::TOK_MODULEKW, std::move (l));
      }
#else
      static
      symbol_type
      make_MODULEKW (const location_type& l)
      {
        return symbol_type (token::TOK_MODULEKW, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGNKW (location_type l)
      {
        return symbol_type (token::TOK_ASSIGNKW, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGNKW (const location_type& l)
      {
        return symbol_type (token::TOK_ASSIGNKW, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENT (std::string v, location_type l)
      {
        return symbol_type (token::TOK_IDENT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENT (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_IDENT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PRIMGATE (std::string v, location_type l)
      {
        return symbol_type (token::TOK_PRIMGATE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PRIMGATE (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_PRIMGATE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PORTATT (std::string v, location_type l)
      {
        return symbol_type (token::TOK_PORTATT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PORTATT (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_PORTATT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PORTDIR (std::string v, location_type l)
      {
        return symbol_type (token::TOK_PORTDIR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PORTDIR (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_PORTDIR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTEGER (prs::verilog::ast::integer v, location_type l)
      {
        return symbol_type (token::TOK_INTEGER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INTEGER (const prs::verilog::ast::integer& v, const location_type& l)
      {
        return symbol_type (token::TOK_INTEGER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONSTHEX (prs::verilog::ast::hex_constant v, location_type l)
      {
        return symbol_type (token::TOK_CONSTHEX, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONSTHEX (const prs::verilog::ast::hex_constant& v, const location_type& l)
      {
        return symbol_type (token::TOK_CONSTHEX, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONSTBIN (prs::verilog::ast::bin_constant v, location_type l)
      {
        return symbol_type (token::TOK_CONSTBIN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONSTBIN (const prs::verilog::ast::bin_constant& v, const location_type& l)
      {
        return symbol_type (token::TOK_CONSTBIN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONSTDEC (prs::verilog::ast::dec_constant v, location_type l)
      {
        return symbol_type (token::TOK_CONSTDEC, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONSTDEC (const prs::verilog::ast::dec_constant& v, const location_type& l)
      {
        return symbol_type (token::TOK_CONSTDEC, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONSTSTR (std::string v, location_type l)
      {
        return symbol_type (token::TOK_CONSTSTR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONSTSTR (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_CONSTSTR, v, l);
      }
#endif


  private:
    /// This class is not copyable.
    parser (const parser&);
    parser& operator= (const parser&);

    /// Stored state numbers (used for stacks).
    typedef signed char state_type;

    /// Generate an error message.
    /// \param yystate   the state where the error occurred.
    /// \param yyla      the lookahead token.
    virtual std::string yysyntax_error_ (state_type yystate,
                                         const symbol_type& yyla) const;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token number \a t to a symbol number.
    /// In theory \a t should be a token_type, but character literals
    /// are valid, yet not members of the token_type enum.
    static token_number_type yytranslate_ (int t);

    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const signed char yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const signed char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const signed char yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const signed char yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const signed char yytable_[];

    static const signed char yycheck_[];

    // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
    // symbol of state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
    static const signed char yyr1_[];

    // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
    static const signed char yyr2_[];


    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);


    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#if VVDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const unsigned char yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol type, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol type as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol type from \a that.
      void move (by_state& that);

      /// The (internal) type number (corresponding to \a state).
      /// \a empty_symbol when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::reverse_iterator iterator;
      typedef typename S::const_reverse_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200)
        : seq_ (n)
      {}

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      std::ptrdiff_t
      ssize () const YY_NOEXCEPT
      {
        return std::ptrdiff_t (size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.rbegin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.rend ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range)
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
      stack (const stack&);
      stack& operator= (const stack&);
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1);

    /// Some specific tokens.
    static const token_number_type yy_error_token_ = 1;
    static const token_number_type yy_undef_token_ = 2;

    /// Constants.
    enum
    {
      yyeof_ = 0,
      yylast_ = 115,     ///< Last index in yytable_.
      yynnts_ = 26,  ///< Number of nonterminal symbols.
      yyfinal_ = 6, ///< Termination state number.
      yyntokens_ = 27  ///< Number of tokens.
    };


    // User arguments.
    prs::verilog::readverilog& drv;
  };

  inline
  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const token_number_type
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26
    };
    const int user_token_number_max_ = 281;

    if (t <= 0)
      return yyeof_;
    else if (t <= user_token_number_max_)
      return translate_table[t];
    else
      return yy_undef_token_;
  }

  // basic_symbol.
#if 201103L <= YY_CPLUSPLUS
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (basic_symbol&& that)
    : Base (std::move (that))
    , value ()
    , location (std::move (that.location))
  {
    switch (this->type_get ())
    {
      case 43: // assignment_stmt
        value.move< prs::verilog::ast::assignment_stmt > (std::move (that.value));
        break;

      case 24: // "bin_constant"
        value.move< prs::verilog::ast::bin_constant > (std::move (that.value));
        break;

      case 48: // concat_expr
        value.move< prs::verilog::ast::concat_expr > (std::move (that.value));
        break;

      case 50: // conn_pair
        value.move< prs::verilog::ast::conn_pair > (std::move (that.value));
        break;

      case 51: // conn_pair_list
        value.move< prs::verilog::ast::conn_pair_list > (std::move (that.value));
        break;

      case 41: // constant_expr
        value.move< prs::verilog::ast::constant_expr > (std::move (that.value));
        break;

      case 25: // "dec_constant"
        value.move< prs::verilog::ast::dec_constant > (std::move (that.value));
        break;

      case 45: // expr
        value.move< prs::verilog::ast::expr > (std::move (that.value));
        break;

      case 44: // expr_list
        value.move< prs::verilog::ast::expr_list > (std::move (that.value));
        break;

      case 23: // "hex_constant"
        value.move< prs::verilog::ast::hex_constant > (std::move (that.value));
        break;

      case 52: // ident_list
        value.move< prs::verilog::ast::ident_list > (std::move (that.value));
        break;

      case 47: // indexed_expr
        value.move< prs::verilog::ast::indexed_expr > (std::move (that.value));
        break;

      case 39: // instance_stmt
        value.move< prs::verilog::ast::instance_stmt > (std::move (that.value));
        break;

      case 22: // INTEGER
        value.move< prs::verilog::ast::integer > (std::move (that.value));
        break;

      case 29: // module
        value.move< prs::verilog::ast::module > (std::move (that.value));
        break;

      case 30: // module_header
        value.move< prs::verilog::ast::module_header > (std::move (that.value));
        break;

      case 32: // module_port_decl
        value.move< prs::verilog::ast::module_port_decl > (std::move (that.value));
        break;

      case 31: // module_port_decl_list
        value.move< prs::verilog::ast::module_port_decl_list > (std::move (that.value));
        break;

      case 38: // multi_var_decl
        value.move< prs::verilog::ast::multi_var_decl > (std::move (that.value));
        break;

      case 35: // net_decl_stmt
        value.move< prs::verilog::ast::net_decl_stmt > (std::move (that.value));
        break;

      case 40: // param_config
        value.move< prs::verilog::ast::param_config > (std::move (that.value));
        break;

      case 42: // prim_gate_stmt
        value.move< prs::verilog::ast::prim_gate_stmt > (std::move (that.value));
        break;

      case 49: // range
        value.move< prs::verilog::ast::range > (std::move (that.value));
        break;

      case 46: // ranged_expr
        value.move< prs::verilog::ast::ranged_expr > (std::move (that.value));
        break;

      case 37: // ranged_var_decl
        value.move< prs::verilog::ast::ranged_var_decl > (std::move (that.value));
        break;

      case 34: // statement
        value.move< prs::verilog::ast::statement > (std::move (that.value));
        break;

      case 33: // statement_list
        value.move< prs::verilog::ast::statement_list > (std::move (that.value));
        break;

      case 36: // var_decl
        value.move< prs::verilog::ast::var_decl > (std::move (that.value));
        break;

      case 28: // verilog
        value.move< prs::verilog::ast::verilog > (std::move (that.value));
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.move< std::string > (std::move (that.value));
        break;

      default:
        break;
    }

  }
#endif

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->type_get ())
    {
      case 43: // assignment_stmt
        value.copy< prs::verilog::ast::assignment_stmt > (YY_MOVE (that.value));
        break;

      case 24: // "bin_constant"
        value.copy< prs::verilog::ast::bin_constant > (YY_MOVE (that.value));
        break;

      case 48: // concat_expr
        value.copy< prs::verilog::ast::concat_expr > (YY_MOVE (that.value));
        break;

      case 50: // conn_pair
        value.copy< prs::verilog::ast::conn_pair > (YY_MOVE (that.value));
        break;

      case 51: // conn_pair_list
        value.copy< prs::verilog::ast::conn_pair_list > (YY_MOVE (that.value));
        break;

      case 41: // constant_expr
        value.copy< prs::verilog::ast::constant_expr > (YY_MOVE (that.value));
        break;

      case 25: // "dec_constant"
        value.copy< prs::verilog::ast::dec_constant > (YY_MOVE (that.value));
        break;

      case 45: // expr
        value.copy< prs::verilog::ast::expr > (YY_MOVE (that.value));
        break;

      case 44: // expr_list
        value.copy< prs::verilog::ast::expr_list > (YY_MOVE (that.value));
        break;

      case 23: // "hex_constant"
        value.copy< prs::verilog::ast::hex_constant > (YY_MOVE (that.value));
        break;

      case 52: // ident_list
        value.copy< prs::verilog::ast::ident_list > (YY_MOVE (that.value));
        break;

      case 47: // indexed_expr
        value.copy< prs::verilog::ast::indexed_expr > (YY_MOVE (that.value));
        break;

      case 39: // instance_stmt
        value.copy< prs::verilog::ast::instance_stmt > (YY_MOVE (that.value));
        break;

      case 22: // INTEGER
        value.copy< prs::verilog::ast::integer > (YY_MOVE (that.value));
        break;

      case 29: // module
        value.copy< prs::verilog::ast::module > (YY_MOVE (that.value));
        break;

      case 30: // module_header
        value.copy< prs::verilog::ast::module_header > (YY_MOVE (that.value));
        break;

      case 32: // module_port_decl
        value.copy< prs::verilog::ast::module_port_decl > (YY_MOVE (that.value));
        break;

      case 31: // module_port_decl_list
        value.copy< prs::verilog::ast::module_port_decl_list > (YY_MOVE (that.value));
        break;

      case 38: // multi_var_decl
        value.copy< prs::verilog::ast::multi_var_decl > (YY_MOVE (that.value));
        break;

      case 35: // net_decl_stmt
        value.copy< prs::verilog::ast::net_decl_stmt > (YY_MOVE (that.value));
        break;

      case 40: // param_config
        value.copy< prs::verilog::ast::param_config > (YY_MOVE (that.value));
        break;

      case 42: // prim_gate_stmt
        value.copy< prs::verilog::ast::prim_gate_stmt > (YY_MOVE (that.value));
        break;

      case 49: // range
        value.copy< prs::verilog::ast::range > (YY_MOVE (that.value));
        break;

      case 46: // ranged_expr
        value.copy< prs::verilog::ast::ranged_expr > (YY_MOVE (that.value));
        break;

      case 37: // ranged_var_decl
        value.copy< prs::verilog::ast::ranged_var_decl > (YY_MOVE (that.value));
        break;

      case 34: // statement
        value.copy< prs::verilog::ast::statement > (YY_MOVE (that.value));
        break;

      case 33: // statement_list
        value.copy< prs::verilog::ast::statement_list > (YY_MOVE (that.value));
        break;

      case 36: // var_decl
        value.copy< prs::verilog::ast::var_decl > (YY_MOVE (that.value));
        break;

      case 28: // verilog
        value.copy< prs::verilog::ast::verilog > (YY_MOVE (that.value));
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.copy< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }



  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->type_get ())
    {
      case 43: // assignment_stmt
        value.move< prs::verilog::ast::assignment_stmt > (YY_MOVE (s.value));
        break;

      case 24: // "bin_constant"
        value.move< prs::verilog::ast::bin_constant > (YY_MOVE (s.value));
        break;

      case 48: // concat_expr
        value.move< prs::verilog::ast::concat_expr > (YY_MOVE (s.value));
        break;

      case 50: // conn_pair
        value.move< prs::verilog::ast::conn_pair > (YY_MOVE (s.value));
        break;

      case 51: // conn_pair_list
        value.move< prs::verilog::ast::conn_pair_list > (YY_MOVE (s.value));
        break;

      case 41: // constant_expr
        value.move< prs::verilog::ast::constant_expr > (YY_MOVE (s.value));
        break;

      case 25: // "dec_constant"
        value.move< prs::verilog::ast::dec_constant > (YY_MOVE (s.value));
        break;

      case 45: // expr
        value.move< prs::verilog::ast::expr > (YY_MOVE (s.value));
        break;

      case 44: // expr_list
        value.move< prs::verilog::ast::expr_list > (YY_MOVE (s.value));
        break;

      case 23: // "hex_constant"
        value.move< prs::verilog::ast::hex_constant > (YY_MOVE (s.value));
        break;

      case 52: // ident_list
        value.move< prs::verilog::ast::ident_list > (YY_MOVE (s.value));
        break;

      case 47: // indexed_expr
        value.move< prs::verilog::ast::indexed_expr > (YY_MOVE (s.value));
        break;

      case 39: // instance_stmt
        value.move< prs::verilog::ast::instance_stmt > (YY_MOVE (s.value));
        break;

      case 22: // INTEGER
        value.move< prs::verilog::ast::integer > (YY_MOVE (s.value));
        break;

      case 29: // module
        value.move< prs::verilog::ast::module > (YY_MOVE (s.value));
        break;

      case 30: // module_header
        value.move< prs::verilog::ast::module_header > (YY_MOVE (s.value));
        break;

      case 32: // module_port_decl
        value.move< prs::verilog::ast::module_port_decl > (YY_MOVE (s.value));
        break;

      case 31: // module_port_decl_list
        value.move< prs::verilog::ast::module_port_decl_list > (YY_MOVE (s.value));
        break;

      case 38: // multi_var_decl
        value.move< prs::verilog::ast::multi_var_decl > (YY_MOVE (s.value));
        break;

      case 35: // net_decl_stmt
        value.move< prs::verilog::ast::net_decl_stmt > (YY_MOVE (s.value));
        break;

      case 40: // param_config
        value.move< prs::verilog::ast::param_config > (YY_MOVE (s.value));
        break;

      case 42: // prim_gate_stmt
        value.move< prs::verilog::ast::prim_gate_stmt > (YY_MOVE (s.value));
        break;

      case 49: // range
        value.move< prs::verilog::ast::range > (YY_MOVE (s.value));
        break;

      case 46: // ranged_expr
        value.move< prs::verilog::ast::ranged_expr > (YY_MOVE (s.value));
        break;

      case 37: // ranged_var_decl
        value.move< prs::verilog::ast::ranged_var_decl > (YY_MOVE (s.value));
        break;

      case 34: // statement
        value.move< prs::verilog::ast::statement > (YY_MOVE (s.value));
        break;

      case 33: // statement_list
        value.move< prs::verilog::ast::statement_list > (YY_MOVE (s.value));
        break;

      case 36: // var_decl
        value.move< prs::verilog::ast::var_decl > (YY_MOVE (s.value));
        break;

      case 28: // verilog
        value.move< prs::verilog::ast::verilog > (YY_MOVE (s.value));
        break;

      case 18: // "ident"
      case 19: // "primitive_gate"
      case 20: // "variable_attribute"
      case 21: // "port_direction"
      case 26: // "str_constant"
        value.move< std::string > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_type.
  inline
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  parser::by_type::by_type (by_type&& that)
    : type (that.type)
  {
    that.clear ();
  }
#endif

  inline
  parser::by_type::by_type (const by_type& that)
    : type (that.type)
  {}

  inline
  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  parser::by_type::type_get () const YY_NOEXCEPT
  {
    return type;
  }

} // vv
#line 2832 "src/parser/verilog/verilog_parser.tab.hh"





#endif // !YY_VV_SRC_PARSER_VERILOG_VERILOG_PARSER_TAB_HH_INCLUDED
