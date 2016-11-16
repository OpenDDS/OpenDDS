/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPIC_EXPRESSION_GRAMMAR_H
#define OPENDDS_DCPS_TOPIC_EXPRESSION_GRAMMAR_H

#ifndef OPENDDS_NO_MULTI_TOPIC

#include "FilterExpressionGrammar.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace TopicExpressionGrammar {
  using namespace yard;
  using namespace text_grammar;
  using namespace GrammarUtils;
  using FilterExpressionGrammar::FieldName;

  struct SELECT : Keyword<CharSeqIgnoreCase<'s', 'e', 'l', 'e', 'c', 't'> > {};
  struct FROM : Keyword<CharSeqIgnoreCase<'f', 'r', 'o', 'm'> > {};
  struct WHERE : Keyword<CharSeqIgnoreCase<'w', 'h', 'e', 'r', 'e'> > {};
  struct INNER : Keyword<CharSeqIgnoreCase<'i', 'n', 'n', 'e', 'r'> > {};
  struct NATURAL
    : Keyword<CharSeqIgnoreCase<'n', 'a', 't', 'u', 'r', 'a', 'l'> > {};
  struct JOIN : Keyword<CharSeqIgnoreCase<'j', 'o', 'i', 'n'> > {};
  struct AS : Keyword<CharSeqIgnoreCase<'a', 's'> > {};

  struct TopicName : Ident {};
  struct NaturalJoin : Or<Seq<INNER, NATURAL, JOIN>,
    Seq<NATURAL, Opt<INNER>, JOIN> > {};
  struct JoinItem : Or<Seq<ST<TopicName>, Opt<Seq<NaturalJoin, JoinItem> > >,
    Seq<LPAREN, ST<TopicName>, NaturalJoin, JoinItem, RPAREN> > {};
  struct Selection : Seq<ST<TopicName>, Opt<Seq<NaturalJoin, JoinItem> > > {};

  struct FieldAlias : Or<Seq<AS, ST<FieldName> >,
    Seq<NotAt<FROM>, ST<FieldName> > > {};
  struct SubjectFieldSpec : Seq<ST<FieldName>, Opt<FieldAlias> > {};
  struct Aggregation : Or<Tok<Char<'*'> >,
    DelimitedList<ST<SubjectFieldSpec>, Tok<Char<','> > > > {};

  struct SelectFrom : Seq<SELECT, Aggregation, FROM, Selection> {};
  struct WhereClause : Seq<WHERE, Store<FilterExpressionGrammar::Cond> > {};
  struct TopicExpr : Seq<SelectFrom, Opt<WhereClause> > {};

  struct TopicCompleteInput : Seq<TopicExpr, Opt<Tok<Char<';'> > >,
    EndOfInput> {};
}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
