/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FILTER_EXPRESSION_GRAMMAR_H
#define OPENDDS_DCPS_FILTER_EXPRESSION_GRAMMAR_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include <cassert>
#include <typeinfo>

#include "yard/yard_base_grammar.hpp"
#include "yard/yard_char_set.hpp"
#include "yard/yard_tree.hpp"
#include "yard/yard_text_grammar.hpp"

namespace OpenDDS {
namespace DCPS {

namespace FilterExpressionGrammar {
  using namespace yard;
  using namespace text_grammar;

  struct WS : CharSetParser<WhiteSpaceCharSet> {};
  template<typename T> struct Tok : Seq<T, Star<WS> > {};

  struct OptPlusMinus : Opt<Or<Char<'+'>, Char<'-'> > > {};
  struct IntValDec : Seq<OptPlusMinus, Plus<Digit>,
    NotAt<Digit>, NotAt<Or<Char<'x'>, Char<'X'>, Char<'.'> > > > {};
  struct IntValHex : Seq<CharSeqIgnoreCase<'0', 'x'>, Plus<HexDigit>,
    NotAlphaNum> {};
  struct IntVal : Or<IntValDec, IntValHex> {};

  struct Quote : CharSetParser<CharSet<'\'', '`'> > {};
  struct CharVal : Seq<Quote, AnyChar, Char<'\''> > {};

  struct OptExp : Opt<Seq<Char<'e'>, OptPlusMinus, Plus<Digit> > > {};
  struct FloatStartWithDigit : Seq<Plus<Digit>, Opt<Char<'.'> >,
    Star<Digit> > {};
  struct FloatStartWithDot : Seq<Char<'.'>, Plus<Digit> > {};
  struct FloatVal : Seq<OptPlusMinus,
    Or<FloatStartWithDigit, FloatStartWithDot>, OptExp, NotAlphaNum> {};

  struct StrVal : Seq<Quote, Star<AnyCharExcept<CharSet<'\n', '\''> > >,
    Char<'\''> > {};

  struct ParamVal : Seq<Char<'%'>, Digit, Opt<Digit> > {};

  struct Param : Tok<Or<Store<IntVal>, Store<CharVal>, Store<FloatVal>,
    Store<StrVal>, Store<ParamVal> > > {}; // EnumVal is parsed as StrVal

  template<typename T> struct Keyword : Tok<Seq<T, NotAlphaNum> > {};
  struct AND : Keyword<CharSeqIgnoreCase<'a', 'n', 'd'> > {};
  struct OR : Keyword<CharSeqIgnoreCase<'o', 'r'> > {};
  struct NOT : Keyword<CharSeqIgnoreCase<'n', 'o', 't'> > {};
  struct BETWEEN
    : Keyword<CharSeqIgnoreCase<'b', 'e', 't', 'w', 'e', 'e', 'n'> > {};

  struct OP_EQ : Tok<Char<'='> > {};
  struct OP_LT : Tok<Seq<Char<'<'>, NotAt<Or<Char<'='>, Char<'>'> > > > > {};
  struct OP_GT : Tok<Seq<Char<'>'>, NotAt<Char<'='> > > > {};
  struct OP_LTEQ : Tok<Seq<Char<'<'>, Char<'='> > > {};
  struct OP_GTEQ : Tok<Seq<Char<'>'>, Char<'='> > > {};
  struct OP_NEQ : Tok<Seq<Char<'<'>, Char<'>'> > > {};
  struct OP_LIKE : Keyword<CharSeqIgnoreCase<'l', 'i', 'k', 'e'> > {};
  struct RelOp : Or<Store<OP_EQ>, Store<OP_LT>, Store<OP_GT>, Store<OP_LTEQ>,
    Store<OP_GTEQ>, Store<OP_NEQ>, Store<OP_LIKE> > {};
  struct LPAREN : Tok<Char<'('> > {};
  struct RPAREN : Tok<Char<')'> > {};

  template<typename R, typename D>
  struct DelimitedList : Seq<R, Star<Seq<D, R> > > {};
  struct FieldName : DelimitedList<Seq<Letter, Star<IdentNextChar> >,
    Char<'.'> > {};
  template<typename T> struct ST : Tok<Store<T> > {}; // "Store-Tokenize"

  struct BetweenPred : Seq<ST<FieldName>, Opt<Store<NOT> >, BETWEEN,
    Param, AND, Param> {};
  struct CmpFnParam : Seq<ST<FieldName>, RelOp, Param> {};
  struct CmpParamFn : Seq<Param, RelOp, ST<FieldName> > {};
  struct CmpBothFn : Seq<ST<FieldName>, RelOp, ST<FieldName> > {};
  struct Pred : Or<Store<BetweenPred>, Store<CmpFnParam>, Store<CmpParamFn>,
    Store<CmpBothFn> > {};

  struct Cond;
  struct CondTail : Opt<Seq<Or<Store<AND>, Store<OR> >, Cond, CondTail> > {};
  struct Cond : Or<Seq<Pred, CondTail>, Seq<Store<NOT>, Cond, CondTail>,
    Seq<LPAREN, Store<Cond>, RPAREN, CondTail> > {};
  struct FilterCompleteInput : Seq<Cond, EndOfInput> {};

  struct ORDERBY
    : Keyword<CharSeqIgnoreCase<'o', 'r', 'd', 'e', 'r', ' ', 'b', 'y'> > {};
  struct Query : Seq<Opt<Cond>, Store<ORDERBY>,
    DelimitedList<ST<FieldName>, Tok<Char<','> > > > {};
  struct QueryCompleteInput : Seq<Query, EndOfInput> {};
}
}
}
#endif
#endif
