/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "FilterEvaluator.h"
#include "FilterExpressionGrammar.h"
#include "AstNodeWrapper.h"

#include <ace/ACE.h>

#include <stdexcept>
#include <cstring>
#include <sstream>
#include <algorithm>

using namespace OpenDDS::DCPS::FilterExpressionGrammar;

namespace OpenDDS {
namespace DCPS {

FilterEvaluator::FilterEvaluator(const char* filter, bool allowOrderBy)
  : filter_root_(0)
{
  const char* out = filter + std::strlen(filter);
  yard::SimpleTextParser parser(filter, out);
  if (!(allowOrderBy ? parser.Parse<QueryCompleteInput>()
      : parser.Parse<FilterCompleteInput>())) {
    reportErrors(parser, filter);
  }

  bool found_order_by = false;
  for (AstNode* iter = parser.GetAstRoot()->GetFirstChild(); iter;
      iter = iter->GetSibling()) {
    if (iter->TypeMatches<ORDERBY>()) {
      found_order_by = true;
    } else if (found_order_by && iter->TypeMatches<FieldName>()) {
      order_bys_.push_back(toString(iter));
    } else {
      filter_root_ = walkAst(iter, filter_root_);
    }
  }
}

FilterEvaluator::FilterEvaluator(const AstNodeWrapper& yardNode)
  : filter_root_(walkAst(yardNode, NULL))
{
}

class FilterEvaluator::EvalNode {
public:
  void addChild(EvalNode* n)
  {
    children_.push_back(n);
  }

  virtual ~EvalNode()
  {
    std::for_each(children_.begin(), children_.end(), deleteChild);
  }

  virtual Value eval(const void* sample, const MetaStruct& meta,
                     const DDS::StringSeq& params) = 0;

private:
  static void deleteChild(EvalNode* child)
  {
    delete child;
  }

protected:
  std::vector<EvalNode*> children_;
};

FilterEvaluator::~FilterEvaluator()
{
  delete filter_root_;
}

namespace {

  class Operand : public FilterEvaluator::EvalNode {
  };

  class FieldLookup : public Operand {
  public:
    explicit FieldLookup(AstNode* fnNode)
      : fieldName_(toString(fnNode))
    {}

    Value eval(const void* sample, const MetaStruct& meta,
               const DDS::StringSeq&)
    {
      return meta.getValue(sample, fieldName_.c_str());
    }

    std::string fieldName_;
  };

  class LiteralInt : public Operand {
  public:
    explicit LiteralInt(AstNode* fnNode)
    {
      std::string strVal = toString(fnNode);
      if (strVal.length() > 2 && strVal[0] == '0'
          && (strVal[1] == 'x' || strVal[1] == 'X')) {
        std::istringstream is(strVal.c_str() + 2);
        is >> std::hex >> value_;
      } else {
        value_ = std::atoi(strVal.c_str());
      }
    }

    Value eval(const void*, const MetaStruct&, const DDS::StringSeq&)
    {
      return Value(value_, true);
    }

    int value_;
  };

  class LiteralChar : public Operand {
  public:
    explicit LiteralChar(AstNode* fnNode)
      : value_(toString(fnNode)[1])
    {}

    Value eval(const void*, const MetaStruct&, const DDS::StringSeq&)
    {
      return Value(value_, true);
    }

    char value_;
  };

  class LiteralFloat : public Operand {
  public:
    explicit LiteralFloat(AstNode* fnNode)
      : value_(std::atof(toString(fnNode).c_str()))
    {}

    Value eval(const void*, const MetaStruct&, const DDS::StringSeq&)
    {
      return Value(value_, true);
    }

    double value_;
  };

  class LiteralString : public Operand {
  public:
    explicit LiteralString(AstNode* fnNode)
      : value_(toString(fnNode).substr(1) /* trim left ' */)
    {
      value_.erase(value_.length() - 1); // trim right '
    }

    Value eval(const void*, const MetaStruct&, const DDS::StringSeq&)
    {
      return Value(value_.c_str(), true);
    }

    std::string value_;
  };

  class Parameter : public Operand {
  public:
    explicit Parameter(AstNode* fnNode)
      : param_(std::atoi(toString(fnNode).c_str() + 1 /* skip % */))
    {}

    Value eval(const void*, const MetaStruct&, const DDS::StringSeq& params)
    {
      return Value(params[param_], true);
    }

    int param_;
  };

  Operand* createOperand(AstNode* node)
  {
    if (node->TypeMatches<FieldName>()) {
      return new FieldLookup(node);
    } else if (node->TypeMatches<IntVal>()) {
      return new LiteralInt(node);
    } else if (node->TypeMatches<CharVal>()) {
      return new LiteralChar(node);
    } else if (node->TypeMatches<FloatVal>()) {
      return new LiteralFloat(node);
    } else if (node->TypeMatches<StrVal>()) {
      return new LiteralString(node);
    } else if (node->TypeMatches<ParamVal>()) {
      return new Parameter(node);
    }
    return 0;
  }

  class Comparison : public FilterEvaluator::EvalNode {
  public:
    enum Operator {OPER_EQ, OPER_LT, OPER_GT, OPER_LTEQ, OPER_GTEQ, OPER_NEQ,
      OPER_LIKE};

    explicit Comparison(AstNode* cmpNode)
    {
      AstNode* iter = cmpNode->GetFirstChild();
      left_ = createOperand(iter);
      addChild(left_);
      iter = iter->GetSibling();
      setOperator(iter);
      iter = iter->GetSibling();
      right_ = createOperand(iter);
      addChild(right_);
    }

    Value eval(const void* sample, const MetaStruct& meta,
               const DDS::StringSeq& params)
    {
      Value left = left_->eval(sample, meta, params);
      Value right = right_->eval(sample, meta, params);
      switch (oper_type_) {
      case OPER_EQ:
        return left == right;
      case OPER_LT:
        return left < right;
      case OPER_GT:
        return right < left;
      case OPER_LTEQ:
        return !(right < left);
      case OPER_GTEQ:
        return !(left < right);
      case OPER_NEQ:
        return !(left == right);
      case OPER_LIKE:
        return left.like(right);
      }
      return false; // not reached
    }

  private:
    void setOperator(AstNode* node)
    {
      if (node->TypeMatches<OP_EQ>()) {
        oper_type_ = OPER_EQ;
      } else if (node->TypeMatches<OP_LT>()) {
        oper_type_ = OPER_LT;
      } else if (node->TypeMatches<OP_GT>()) {
        oper_type_ = OPER_GT;
      } else if (node->TypeMatches<OP_LTEQ>()) {
        oper_type_ = OPER_LTEQ;
      } else if (node->TypeMatches<OP_GTEQ>()) {
        oper_type_ = OPER_GTEQ;
      } else if (node->TypeMatches<OP_NEQ>()) {
        oper_type_ = OPER_NEQ;
      } else if (node->TypeMatches<OP_LIKE>()) {
        oper_type_ = OPER_LIKE;
      }
    }

    Operand* left_;
    Operand* right_;
    Operator oper_type_;
  };

  class Between : public FilterEvaluator::EvalNode {
  public:
    explicit Between(AstNode* btwNode)
      : invert_(false)
    {
      AstNode* iter = btwNode->GetFirstChild();
      field_ = new FieldLookup(iter);
      addChild(field_);
      iter = iter->GetSibling();
      if (iter->TypeMatches<NOT>()) {
        invert_ = true;
        iter = iter->GetSibling();
      }
      left_ = createOperand(iter);
      addChild(left_);
      iter = iter->GetSibling();
      right_ = createOperand(iter);
      addChild(right_);
    }

    Value eval(const void* sample, const MetaStruct& meta,
               const DDS::StringSeq& params)
    {
      Value field = field_->eval(sample, meta, params);
      Value left = left_->eval(sample, meta, params);
      Value right = right_->eval(sample, meta, params);
      bool btwn = !(field < left) && !(right < field);
      return invert_ ? !btwn : btwn;
    }

  private:
    bool invert_;
    FieldLookup* field_;
    Operand* left_;
    Operand* right_;
  };

  class Logical : public FilterEvaluator::EvalNode {
  public:
    enum LogicalOp {LG_AND, LG_OR, LG_NOT};

    Logical(LogicalOp op, EvalNode*& left)
      : op_(op)
    {
      if (op_ != LG_NOT) {
        addChild(left);
        left = 0;
      }
    }

    Value eval(const void* sample, const MetaStruct& meta,
               const DDS::StringSeq& params)
    {
      Value left = children_[0]->eval(sample, meta, params);
      assert(left.type_ == Value::VAL_BOOL);
      switch (op_) {
      case LG_NOT:
        return !left.b_;
      case LG_AND:
        if (!left.b_) return false;
        break;
      case LG_OR:
        if (left.b_) return true;
        break;
      }
      return children_[1]->eval(sample, meta, params);
    }

  private:
    LogicalOp op_;
  };
}

FilterEvaluator::EvalNode*
FilterEvaluator::walkAst(const FilterEvaluator::AstNodeWrapper& node,
                         EvalNode* prev)
{
  EvalNode* eval = 0;
  if (node->TypeMatches<CmpFnParam>() || node->TypeMatches<CmpParamFn>()
      || node->TypeMatches<CmpBothFn>()) {
    eval = new Comparison(node);
  } else if (node->TypeMatches<BetweenPred>()) {
    eval = new Between(node);
  } else if (node->TypeMatches<AND>()) {
    eval = new Logical(Logical::LG_AND, prev);
  } else if (node->TypeMatches<OR>()) {
    eval = new Logical(Logical::LG_OR, prev);
  } else if (node->TypeMatches<NOT>()) {
    eval = new Logical(Logical::LG_NOT, prev);
  } else if (node->TypeMatches<Cond>()) {
    for (AstNode* iter = node->GetFirstChild(); iter;
        iter = iter->GetSibling()) {
      eval = walkAst(iter, eval);
    }
  } else {
    assert(0);
  }

  if (prev) {
    prev->addChild(eval);
    return prev;
  }
  return eval;
}

bool
FilterEvaluator::eval_i(const void* sample, const MetaStruct& meta,
                        const DDS::StringSeq& params) const
{
  return filter_root_->eval(sample, meta, params).b_;
}

std::vector<std::string>
FilterEvaluator::getOrderBys() const
{
  return order_bys_;
}

bool
FilterEvaluator::hasFilter() const
{
  return filter_root_ != 0;
}

Value::Value(bool b, bool conversion_preferred)
  : type_(VAL_BOOL), b_(b), conversion_preferred_(conversion_preferred)
{}

Value::Value(int i, bool conversion_preferred)
  : type_(VAL_INT), i_(i), conversion_preferred_(conversion_preferred)
{}

Value::Value(unsigned int u, bool conversion_preferred)
  : type_(VAL_UINT), u_(u), conversion_preferred_(conversion_preferred)
{}

Value::Value(ACE_INT64 l, bool conversion_preferred)
  : type_(VAL_I64), l_(l), conversion_preferred_(conversion_preferred)
{}

Value::Value(ACE_UINT64 m, bool conversion_preferred)
  : type_(VAL_UI64), m_(m), conversion_preferred_(conversion_preferred)
{}

Value::Value(char c, bool conversion_preferred)
  : type_(VAL_CHAR), c_(c), conversion_preferred_(conversion_preferred)
{}

Value::Value(double f, bool conversion_preferred)
  : type_(VAL_FLOAT), f_(f), conversion_preferred_(conversion_preferred)
{}

Value::Value(ACE_CDR::LongDouble ld, bool conversion_preferred)
  : type_(VAL_LNGDUB), ld_(ld), conversion_preferred_(conversion_preferred)
{}

Value::Value(const char* s, bool conversion_preferred)
  : type_(VAL_STRING), s_(ACE_OS::strdup(s))
  , conversion_preferred_(conversion_preferred)
{}

template<> bool& Value::get() { return b_; }
template<> int& Value::get() { return i_; }
template<> unsigned int& Value::get() { return u_; }
template<> ACE_INT64& Value::get() { return l_; }
template<> ACE_UINT64& Value::get() { return m_; }
template<> char& Value::get() { return c_; }
template<> double& Value::get() { return f_; }
template<> ACE_CDR::LongDouble& Value::get() { return ld_; }
template<> const char*& Value::get() { return s_; }

#ifdef OPENDDS_GCC33
template<> const bool& Value::get<bool>() const { return b_; }
template<> const int& Value::get<int>() const { return i_; }
template<> const unsigned int& Value::get<unsigned int>() const { return u_; }
template<> const ACE_INT64& Value::get<ACE_INT64>() const { return l_; }
template<> const ACE_UINT64& Value::get<ACE_UINT64>() const { return m_; }
template<> const char& Value::get<char>() const { return c_; }
template<> const double& Value::get<double>() const { return f_; }
template<> const ACE_CDR::LongDouble& Value::get<ACE_CDR::LongDouble>() const
  { return ld_; }
template<> const char* const& Value::get<const char*>() const { return s_; }
#else
template<> const bool& Value::get() const { return b_; }
template<> const int& Value::get() const { return i_; }
template<> const unsigned int& Value::get() const { return u_; }
template<> const ACE_INT64& Value::get() const { return l_; }
template<> const ACE_UINT64& Value::get() const { return m_; }
template<> const char& Value::get() const { return c_; }
template<> const double& Value::get() const { return f_; }
template<> const ACE_CDR::LongDouble& Value::get() const { return ld_; }
template<> const char* const& Value::get() const { return s_; }
#endif

Value::~Value()
{
  if (type_ == VAL_STRING) ACE_OS::free((void*)s_);
}

namespace {
  template<typename Visitor, typename Val>
  typename Visitor::result_type visit(Visitor& vis, Val& val)
  {
    switch (val.type_) {
    case Value::VAL_BOOL:
      return vis(val.b_);
    case Value::VAL_INT:
      return vis(val.i_);
    case Value::VAL_UINT:
      return vis(val.u_);
    case Value::VAL_I64:
      return vis(val.l_);
    case Value::VAL_UI64:
      return vis(val.m_);
    case Value::VAL_FLOAT:
      return vis(val.f_);
    case Value::VAL_LNGDUB:
      return vis(val.ld_);
    case Value::VAL_CHAR:
      return vis(val.c_);
    case Value::VAL_STRING:
      return vis(val.s_);
    default:
      throw std::runtime_error("Unexpected type of Value");
    }
  }

  template<typename ResultType = void>
  struct VisitorBase {
    typedef ResultType result_type;
  };

  struct Assign : VisitorBase<> {
    explicit Assign(Value& target, bool steal = false)
      : tgt_(target), steal_(steal) {}

    void operator()(const char* s)
    {
      tgt_.s_ = steal_ ? s : ACE_OS::strdup(s);
    }

    template<typename T> void operator()(const T& s)
    {
      tgt_. OPENDDS_GCC33_TEMPLATE_NON_DEPENDENT get<T>() = s;
    }

    Value& tgt_;
    bool steal_;
  };
}

Value::Value(const Value& v)
  : type_(v.type_), conversion_preferred_(v.conversion_preferred_)
{
  Assign visitor(*this);
  visit(visitor, v);
}

Value&
Value::operator=(const Value& v)
{
  Value cpy(v);
  swap(cpy);
  return *this;
}

void
Value::swap(Value& v)
{
  Value t(v);

  if (v.type_ == VAL_STRING) {
    ACE_OS::free((void*)v.s_);
  }

  Assign visitor1(v, true);
  visit(visitor1, *this);
  if (type_ == VAL_STRING) {
    s_ = 0;
  }

  Assign visitor2(*this, true);
  visit(visitor2, t);
  if (t.type_ == VAL_STRING) {
    t.s_ = 0;
  }

  std::swap(conversion_preferred_, v.conversion_preferred_);
  std::swap(type_, v.type_);
}

namespace {
  struct Equals : VisitorBase<bool> {
    explicit Equals(const Value& lhs) : lhs_(lhs) {}

    bool operator()(const char* s) const
    {
      return std::strcmp(lhs_.s_, s) == 0;
    }

    template<typename T> bool operator()(const T& rhs) const
    {
      return lhs_. OPENDDS_GCC33_TEMPLATE_NON_DEPENDENT get<T>() == rhs;
    }

    const Value& lhs_;
  };

  struct Less : VisitorBase<bool> {
    explicit Less(const Value& lhs) : lhs_(lhs) {}

    bool operator()(const char* s) const
    {
      return std::strcmp(lhs_.s_, s) < 0;
    }

    template<typename T> bool operator()(const T& rhs) const
    {
      return lhs_. OPENDDS_GCC33_TEMPLATE_NON_DEPENDENT get<T>() < rhs;
    }

    const Value& lhs_;
  };
}

bool
Value::operator==(const Value& v) const
{
  Value lhs = *this;
  Value rhs = v;
  conversion(lhs, rhs);
  Equals visitor(lhs);
  return visit(visitor, rhs);
}

bool
Value::operator<(const Value& v) const
{
  Value lhs = *this;
  Value rhs = v;
  conversion(lhs, rhs);
  Less visitor(lhs);
  return visit(visitor, rhs);
}

bool
Value::like(const Value& v) const
{
  if (type_ != VAL_STRING || v.type_ != VAL_STRING) {
    throw std::runtime_error("'like' operator called on non-string arguments.");
  }
  std::string pattern(v.s_);
  // escape ? or * in the pattern string so they are not wildcards
  for (size_t i = pattern.find_first_of("?*"); i < pattern.length();
      i = pattern.find_first_of("?*", i + 1)) {
    pattern.insert(i++, 1, '\\');
  }
  // translate _ and % wildcards into those used by ACE::wild_match() (?, *)
  for (size_t i = pattern.find_first_of("_%"); i < pattern.length();
      i = pattern.find_first_of("_%", i + 1)) {
    pattern[i] = (pattern[i] == '_') ? '?' : '*';
  }
  return ACE::wild_match(s_, pattern.c_str(), true, true);
}

namespace {
  struct StreamInsert : VisitorBase<> {
    explicit StreamInsert(std::ostream& os) : os_(os) {}

    template<typename T> void operator()(const T& t)
    {
      os_ << t;
    }

    std::ostream& os_;
  };

  struct StreamExtract : VisitorBase<> {
    explicit StreamExtract(std::istream& is) : is_(is) {}

    void operator()(const char*) {}
    // not called.  prevents instantiation of the following with T = const char*

    void operator()(ACE_CDR::LongDouble ld)
    {
#ifdef NONNATIVE_LONGDOUBLE
      ACE_CDR::LongDouble::NativeImpl ni;
      is_ >> ni;
      ld.assign(ni);
#else
      is_ >> ld;
#endif
    }

    template<typename T> void operator()(T& t)
    {
      is_ >> t;
    }

    std::istream& is_;
  };
}

bool
Value::convert(Value::Type t)
{
  std::string asString;
  if (type_ == VAL_STRING) {
    asString = s_;
  } else {
    std::ostringstream oss;
    StreamInsert visitor(oss);
    visit(visitor, *this);
    asString = oss.str();
  }

  Value newval = 0;
  newval.type_ = t;
  newval.conversion_preferred_ = false;
  if (t == VAL_STRING) {
    newval.s_ = ACE_OS::strdup(asString.c_str());
    swap(newval);
    return true;
  } else {
    std::istringstream iss(asString);
    StreamExtract visitor(iss);
    visit(visitor, newval);
    if (iss.eof() && !iss.bad()) {
      swap(newval);
      return true;
    }
    return false;
  }
}

void
Value::conversion(Value& lhs, Value& rhs)
{
  if (lhs.type_ == rhs.type_) {
    return;
  }
  bool ok = false;
  Value& smaller = (lhs.type_ < rhs.type_) ? lhs : rhs;
  Value& larger = (lhs.type_ < rhs.type_) ? rhs : lhs;
  if (smaller.conversion_preferred_) {
    ok = smaller.convert(larger.type_);
  } else if (larger.conversion_preferred_) {
    ok = larger.convert(smaller.type_);
  } else if (smaller.type_ == VAL_CHAR && larger.type_ == VAL_STRING) {
    ok = smaller.convert(VAL_STRING);
  } else if (larger.type_ <= VAL_LARGEST_NUMERIC) {
    ok = smaller.convert(larger.type_);
  } else if (larger.type_ == VAL_STRING) {
    if (larger.convert(smaller.type_)) {
      ok = true;
    } else {
      ok = smaller.convert(VAL_STRING);
    }
  }
  if (!ok) {
    throw std::runtime_error("Types don't match and aren't convertible.");
  }
}

MetaStruct::~MetaStruct()
{
}

}
}

#endif
