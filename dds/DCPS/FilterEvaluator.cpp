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

#include "yard/yard_parser.hpp"

#include <ace/ACE.h>

#include <stdexcept>
#include <cstring>
#include <sstream>

using namespace OpenDDS::DCPS::FilterExpressionGrammar;

namespace {
  std::string toString(yard::TreeBuildingParser<char>::Node* iter)
  {
    return std::string(iter->GetFirstToken(), iter->GetLastToken());
  }
}

namespace OpenDDS {
namespace DCPS {

typedef yard::TreeBuildingParser<char>::Node AstNode;

/// keeps the details of yard out of the FilterEvaluator header file
struct FilterEvaluator::AstNodeWrapper {
  AstNodeWrapper(AstNode* ptr)
    : ptr_(ptr) {}
  operator AstNode*() const { return ptr_; }
  AstNode* operator->() const { return ptr_; }
  AstNode* ptr_;
};

FilterEvaluator::FilterEvaluator(const char* filter, bool allowOrderBy)
  : filter_root_(0)
{
  const char* out = filter + std::strlen(filter);
  yard::SimpleTextParser parser(filter, out);
  if (allowOrderBy) {
    if (!parser.Parse<QueryCompleteInput>()) {
      throw std::runtime_error("Invalid expression");
    }
  } else {
    if (!parser.Parse<FilterCompleteInput>()) {
      throw std::runtime_error("Invalid expression");
    }
  }

  AstNode* root = parser.GetAstRoot();
  bool found_order_by = false;
  for (AstNode* iter = root->GetFirstChild(); iter; iter = iter->GetSibling()) {
    if (iter->TypeMatches<ORDERBY>()) {
      found_order_by = true;
    } else if (found_order_by && iter->TypeMatches<FieldName>()) {
      order_bys_.push_back(toString(iter));
    } else {
      filter_root_ = walkAst(iter, filter_root_);
    }
  }
}

class FilterEvaluator::EvalNode {
public:
  void addChild(EvalNode* n)
  {
    children_.push_back(n);
  }

  virtual ~EvalNode()
  {
    for_each(children_.begin(), children_.end(), deleteChild);
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
      return value_;
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
      return value_;
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
      return value_;
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
      return value_.c_str();
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
      Value v = params[param_].in();
      v.convertable_ = true;
      return v;
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
      case LG_OR:
        if (left.b_) return true;
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
  Value v = filter_root_->eval(sample, meta, params);
  return v.b_;
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

Value::Value(bool b)
  : type_(VAL_BOOL), b_(b), convertable_(false)
{}

Value::Value(int i)
  : type_(VAL_INT), i_(i), convertable_(false)
{}

Value::Value(char c)
  : type_(VAL_CHAR), c_(c), convertable_(false)
{}

Value::Value(double f)
  : type_(VAL_FLOAT), f_(f), convertable_(false)
{}

Value::Value(const char* s)
  : type_(VAL_STRING), s_(ACE_OS::strdup(s)), convertable_(false)
{}

Value::~Value()
{
  if (type_ == VAL_STRING) ACE_OS::free((void*)s_);
}

Value::Value(const Value& v)
  : type_(v.type_), convertable_(v.convertable_)
{
  switch (type_) {
  case VAL_BOOL:
    b_ = v.b_; break;
  case VAL_INT:
    i_ = v.i_; break;
  case VAL_CHAR:
    c_ = v.c_; break;
  case VAL_FLOAT:
    f_ = v.f_; break;
  case VAL_STRING:
    s_ = ACE_OS::strdup(v.s_); break;
  }
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
  switch (type_) {
  case VAL_BOOL:
    std::swap(b_, v.b_); break;
  case VAL_INT:
    std::swap(i_, v.i_); break;
  case VAL_CHAR:
    std::swap(c_, v.c_); break;
  case VAL_FLOAT:
    std::swap(f_, v.f_); break;
  case VAL_STRING:
    std::swap(s_, v.s_); break;
  }
}

bool
Value::operator==(const Value& v) const
{
  Value lhs = *this;
  Value rhs = v;
  conversion(lhs, rhs);

  switch (lhs.type_) {
  case VAL_BOOL:
    return lhs.b_ == rhs.b_;
  case VAL_INT:
    return lhs.i_ == rhs.i_;
  case VAL_CHAR:
    return lhs.c_ == rhs.c_;
  case VAL_FLOAT:
    return lhs.f_ == rhs.f_;
  case VAL_STRING:
    return std::strcmp(lhs.s_, rhs.s_) == 0;
  }
  return false; // not reached
}

bool
Value::operator<(const Value& v) const
{
  Value lhs = *this;
  Value rhs = v;
  conversion(lhs, rhs);

  switch (lhs.type_) {
  case VAL_BOOL:
    return lhs.b_ < rhs.b_;
  case VAL_INT:
    return lhs.i_ < rhs.i_;
  case VAL_CHAR:
    return lhs.c_ < rhs.c_;
  case VAL_FLOAT:
    return lhs.f_ < rhs.f_;
  case VAL_STRING:
    return std::strcmp(lhs.s_, rhs.s_) < 0;
  }
  return false; // not reached
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

void
Value::convert(Value::Type t)
{
  std::ostringstream oss;
  switch (type_) {
  case VAL_BOOL:
    oss << b_; break;
  case VAL_INT:
    oss << i_; break;
  case VAL_CHAR:
    oss << c_; break;
  case VAL_FLOAT:
    oss << f_; break;
  }
  std::string asString = (type_ == VAL_STRING) ? s_ : oss.str();

  std::istringstream iss(asString);
  Value newval(*this);
  newval.type_ = t;
  newval.convertable_ = false;
  switch (t) {
  case VAL_BOOL:
    iss >> newval.b_; break;
  case VAL_INT:
    iss >> newval.i_; break;
  case VAL_CHAR:
    iss >> newval.c_; break;
  case VAL_FLOAT:
    iss >> newval.f_; break;
  case VAL_STRING:
    newval.s_ = ACE_OS::strdup(asString.c_str()); break;
  }
  swap(newval);
}

void
Value::conversion(Value& lhs, Value& rhs)
{
  if (lhs.type_ != rhs.type_) {
    if (lhs.convertable_) {
      lhs.convert(rhs.type_);
    } else if (rhs.convertable_) {
      rhs.convert(lhs.type_);
    } else {
      throw std::runtime_error("Types don't match and aren't convertable.");
    }
  }
}


// -- will be generated --

template<>
struct MetaStructImpl<DDS::TopicBuiltinTopicData> : MetaStruct {
  Value getValue(const void* stru, const char* fieldSpec) const
  {
    const DDS::TopicBuiltinTopicData& typed =
      *static_cast<const DDS::TopicBuiltinTopicData*>(stru);
    if (std::string(fieldSpec) == "name") {
      return typed.name.in();
    }
    return 0;
  }
};

MetaStructImpl<DDS::TopicBuiltinTopicData> meta_DDS_TopicBuiltinTopicData;

template<>
const MetaStruct& getMetaStruct<DDS::TopicBuiltinTopicData>()
{
  return meta_DDS_TopicBuiltinTopicData;
}

}
}

#endif
