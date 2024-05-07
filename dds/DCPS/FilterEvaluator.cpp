/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "Definitions.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "FilterEvaluator.h"
#include "FilterExpressionGrammar.h"
#include "AstNodeWrapper.h"
#include "Definitions.h"
#include "SafetyProfileStreams.h"
#include "TypeSupportImpl.h"

#include <ace/ACE.h>

#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <sstream>

namespace {
  const char MOD[] = "MOD";
}

using namespace OpenDDS::DCPS::FilterExpressionGrammar;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

FilterEvaluator::DataForEval::~DataForEval()
{}

FilterEvaluator::DeserializedForEval::~DeserializedForEval()
{}

FilterEvaluator::FilterEvaluator(const char* filter, bool allowOrderBy)
  : extended_grammar_(false)
  , filter_root_(0)
  , number_parameters_(0)
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
      filter_root_ = walkAst(iter);
    }
  }
}

FilterEvaluator::FilterEvaluator(const AstNodeWrapper& yardNode)
  : extended_grammar_(false)
  , filter_root_(walkAst(yardNode))
  , number_parameters_(0)
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

  virtual bool has_non_key_fields(const TypeSupportImpl& ts) const
  {
    for (OPENDDS_VECTOR(EvalNode*)::const_iterator i = children_.begin(); i != children_.end(); ++i) {
      EvalNode* child = *i;
      if (child->has_non_key_fields(ts)) {
        return true;
      }
    }
    return false;
  }

  virtual Value eval(DataForEval& data) = 0;

private:
  static void deleteChild(EvalNode* child)
  {
    delete child;
  }

protected:
  OPENDDS_VECTOR(EvalNode*) children_;
};

class FilterEvaluator::Operand : public FilterEvaluator::EvalNode {
public:
  virtual bool isParameter() const { return false; }
};

Value
FilterEvaluator::DeserializedForEval::lookup(const char* field) const
{
  return meta_.getValue(deserialized_, field);
}

FilterEvaluator::SerializedForEval::SerializedForEval(ACE_Message_Block* data,
                                                      const TypeSupportImpl& type_support,
                                                      const DDS::StringSeq& params,
                                                      Encoding encoding)
  : DataForEval(type_support.getMetaStructForType(), params)
  , serialized_(data)
  , encoding_(encoding)
  , type_support_(type_support)
  , exten_(type_support.base_extensibility())
{}

Value
FilterEvaluator::SerializedForEval::lookup(const char* field) const
{
  const OPENDDS_MAP(OPENDDS_STRING, Value)::const_iterator iter = cache_.find(field);
  if (iter != cache_.end()) {
    return iter->second;
  }
  Message_Block_Ptr mb(serialized_->duplicate());
  Serializer ser(mb.get(), encoding_);
  if (encoding_.is_encapsulated()) {
    EncapsulationHeader encap;
    if (!(ser >> encap)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
        ACE_TEXT("FilterEvaluator::SerializedForEval::lookup: ")
        ACE_TEXT("deserialization of encapsulation header failed.\n")));
      throw std::runtime_error("FilterEvaluator::SerializedForEval::lookup:"
        "deserialization of encapsulation header failed.\n");
    }
    Encoding encoding;
    if (!encap.to_encoding(encoding, exten_)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
        ACE_TEXT("FilterEvaluator::SerializedForEval::lookup: ")
        ACE_TEXT("failed to convert encapsulation header to encoding.\n")));
      throw std::runtime_error("FilterEvaluator::SerializedForEval::lookup:"
        "failed to convert encapsulation header to encoding.\n");
    }
    ser.encoding(encoding);
  }
  const Value v = meta_.getValue(ser, field, &type_support_);
  cache_.insert(std::make_pair(OPENDDS_STRING(field), v));
  return v;
}

FilterEvaluator::~FilterEvaluator()
{
  delete filter_root_;
}

bool FilterEvaluator::has_non_key_fields(const TypeSupportImpl& ts) const
{
  for (OPENDDS_VECTOR(OPENDDS_STRING)::const_iterator i = order_bys_.begin(); i != order_bys_.end(); ++i) {
    if (!ts.is_dcps_key(i->c_str())) {
      return true;
    }
  }

  return filter_root_->has_non_key_fields(ts);
}

namespace {

  class FieldLookup : public FilterEvaluator::Operand {
  public:
    explicit FieldLookup(AstNode* fnNode)
      : fieldName_(toString(fnNode))
    {
    }

    Value eval(FilterEvaluator::DataForEval& data)
    {
      return data.lookup(fieldName_.c_str());
    }

    bool has_non_key_fields(const TypeSupportImpl& ts) const
    {
      return !ts.is_dcps_key(fieldName_.c_str());
    }

    OPENDDS_STRING fieldName_;
  };

  class LiteralInt : public FilterEvaluator::Operand {
  public:
    explicit LiteralInt(AstNode* fnNode)
      : value_(0, true)
    {
      const OPENDDS_STRING strVal = toString(fnNode);
      if (strVal.length() > 2 && strVal[0] == '0'
          && (strVal[1] == 'x' || strVal[1] == 'X')) {
        std::istringstream is(strVal.c_str() + 2);
        ACE_UINT64 val;
        is >> std::hex >> val;
        value_ = Value(val, true);
      } else if (!strVal.empty() && strVal[0] == '-') {
        ACE_INT64 val;
        std::istringstream is(strVal.c_str());
        is >> val;
        value_ = Value(val, true);
      } else {
        ACE_UINT64 val;
        std::istringstream is(strVal.c_str());
        is >> val;
        value_ = Value(val, true);
      }
    }

    Value eval(FilterEvaluator::DataForEval&)
    {
      return value_;
    }

    Value value_;
  };

  class LiteralChar : public FilterEvaluator::Operand {
  public:
    explicit LiteralChar(AstNode* fnNode)
      : value_(toString(fnNode)[1])
    {}

    Value eval(FilterEvaluator::DataForEval&)
    {
      return Value(value_, true);
    }

    char value_;
  };

  class LiteralFloat : public FilterEvaluator::Operand {
  public:
    explicit LiteralFloat(AstNode* fnNode)
      : value_(std::atof(toString(fnNode).c_str()))
    {}

    Value eval(FilterEvaluator::DataForEval&)
    {
      return Value(value_, true);
    }

    double value_;
  };

  class LiteralString : public FilterEvaluator::Operand {
  public:
    explicit LiteralString(AstNode* fnNode)
      : value_(toString(fnNode).substr(1) /* trim left ' */)
    {
      value_.erase(value_.length() - 1); // trim right '
    }

    Value eval(FilterEvaluator::DataForEval&)
    {
      return Value(value_.c_str(), true);
    }

    OPENDDS_STRING value_;
  };

  class Parameter : public FilterEvaluator::Operand {
  public:
    explicit Parameter(AstNode* fnNode)
      : param_(std::atoi(toString(fnNode).c_str() + 1 /* skip % */))
    {}

    bool isParameter() const { return true; }

    Value eval(FilterEvaluator::DataForEval& data)
    {
      return Value(data.params_[static_cast<CORBA::ULong>(param_)], true);
    }

    size_t param() { return param_; }

    size_t param_;
  };

  class Comparison : public FilterEvaluator::EvalNode {
  public:
    enum Operator {OPER_EQ, OPER_LT, OPER_GT, OPER_LTEQ, OPER_GTEQ, OPER_NEQ,
      OPER_LIKE, OPER_INVALID};

    explicit Comparison(AstNode* op, FilterEvaluator::Operand* left, FilterEvaluator::Operand* right)
      : left_(left)
      , right_(right)
    {
      addChild(left_);
      addChild(right_);
      setOperator(op);
    }

    Value eval(FilterEvaluator::DataForEval& data)
    {
      Value left = left_->eval(data);
      Value right = right_->eval(data);
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
      default:
        break;
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
      } else {
        oper_type_ = OPER_INVALID;
      }
    }

    FilterEvaluator::Operand* left_;
    FilterEvaluator::Operand* right_;
    Operator oper_type_;
  };

  class Between : public FilterEvaluator::EvalNode {
  public:
    Between(FilterEvaluator::Operand* field, AstNode* betweenOrNot, FilterEvaluator::Operand* left, FilterEvaluator::Operand* right)
      : invert_(betweenOrNot->TypeMatches<NOT_BETWEEN>())
      , field_(field)
      , left_(left)
      , right_(right)
    {
      addChild(field_);
      addChild(left_);
      addChild(right_);
    }

    Value eval(FilterEvaluator::DataForEval& data)
    {
      Value field = field_->eval(data);
      Value left = left_->eval(data);
      Value right = right_->eval(data);
      bool btwn = !(field < left) && !(right < field);
      return invert_ ? !btwn : btwn;
    }

  private:
    bool invert_;
    FilterEvaluator::Operand* field_;
    FilterEvaluator::Operand* left_;
    FilterEvaluator::Operand* right_;
  };

  class Call : public FilterEvaluator::Operand {
  public:
    enum Operator { OP_MOD };

    explicit Call(const OPENDDS_STRING& name)
    {
      if (name == MOD) {
        op_ = OP_MOD;
      } else {
        throw std::runtime_error("Unknown function: " + std::string(name.c_str ()));
      }
    }

    virtual Value eval(FilterEvaluator::DataForEval& data)
    {
      switch (op_) {
      case OP_MOD:
        {
          if (children_.size() != 2) {
            std::stringstream ss;
            ss << MOD << " expects 2 arguments, given " << children_.size();
            throw std::runtime_error(ss.str ());
          }
          Value left = children_[0]->eval(data);
          Value right = children_[1]->eval(data);
          return left % right;
        }
        break;
      }
      OPENDDS_ASSERT(0);
      return Value(0);
    }

  private:
    Operator op_;
  };

  class Logical : public FilterEvaluator::EvalNode {
  public:
    enum LogicalOp {LG_AND, LG_OR, LG_NOT};

    explicit Logical(EvalNode* child)
      : op_(LG_NOT)
    {
      addChild(child);
    }

    Logical(AstNode* op, EvalNode* left, EvalNode* right)
    {
      addChild(left);
      addChild(right);
      if (op->TypeMatches<AND>()) {
        op_ = LG_AND;
      } else if (op->TypeMatches<OR>()) {
        op_ = LG_OR;
      } else {
        OPENDDS_ASSERT(0);
      }
    }

    Value eval(FilterEvaluator::DataForEval& data)
    {
      Value left = children_[0]->eval(data);
      OPENDDS_ASSERT(left.type_ == Value::VAL_BOOL);
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
      return children_[1]->eval(data);
    }

  private:
    LogicalOp op_;
  };
}

static size_t arity(const FilterEvaluator::AstNodeWrapper& node)
{
  size_t a = 0;
  for (AstNode* iter = node->GetFirstChild(); iter; iter = iter->GetSibling()) {
    ++a;
  }
  return a;
}

static FilterEvaluator::AstNodeWrapper child(const FilterEvaluator::AstNodeWrapper& node, size_t idx)
{
  AstNode* iter = 0;
  for (iter = node->GetFirstChild(); idx != 0; iter = iter->GetSibling(), --idx) {}
  return iter;
}

FilterEvaluator::EvalNode*
FilterEvaluator::walkAst(const FilterEvaluator::AstNodeWrapper& node)
{
  if (node->TypeMatches<CompPredDef>()) {
    Operand* left = walkOperand(child(node, 0));
    const FilterEvaluator::AstNodeWrapper& op = child(node, 1);
    Operand* right = walkOperand(child(node, 2));
    if (left->isParameter() && right->isParameter()) {
      extended_grammar_ = true;
    }
    return new Comparison(op, left, right);
  } else if (node->TypeMatches<BetweenPredDef>()) {
    Operand* field = walkOperand(child(node, 0));
    const FilterEvaluator::AstNodeWrapper& op = child(node, 1);
    Operand* low = walkOperand(child(node, 2));
    Operand* high = walkOperand(child(node, 3));
    return new Between(field, op, low, high);
  } else if (node->TypeMatches<CondDef>() || node->TypeMatches<Cond>()) {
    size_t a = arity(node);
    if (a == 1) {
      return walkAst(child(node, 0));
    } else if (a == 2) {
      OPENDDS_ASSERT(child(node, 0)->TypeMatches<NOT>());
      return new Logical(walkAst(child(node, 1)));
    } else if (a == 3) {
      EvalNode* left = walkAst(child(node, 0));
      const FilterEvaluator::AstNodeWrapper& op = child(node, 1);
      EvalNode* right = walkAst(child(node, 2));
      return new Logical(op, left, right);
    }
  }

  OPENDDS_ASSERT(0);
  return 0;
}

FilterEvaluator::Operand*
FilterEvaluator::walkOperand(const FilterEvaluator::AstNodeWrapper& node)
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
    Parameter* retval = new Parameter(node);
    // Keep track of the highest parameter number
    if (retval->param() + 1 > number_parameters_) {
      number_parameters_ = retval->param() + 1;
    }
    return retval;
  } else if (node->TypeMatches<CallDef>()) {
    if (arity(node) == 1) {
      return walkOperand(child(node, 0));
    } else {
      extended_grammar_ = true;
      Call* call = new Call(toString(child(node, 0)));
      for (AstNode* iter = child(node, 1); iter != 0; iter = iter->GetSibling()) {
        call->addChild(walkOperand(iter));
      }
      return call;
    }
  }
  OPENDDS_ASSERT(0);
  return 0;
}

bool
FilterEvaluator::eval_i(DataForEval& data) const
{
  return filter_root_->eval(data).b_;
}

OPENDDS_VECTOR(OPENDDS_STRING)
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

#ifdef NONNATIVE_LONGDOUBLE
Value::Value(long double ld, bool conversion_preferred)
  : type_(VAL_LNGDUB), conversion_preferred_(conversion_preferred)
{
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld_, ld);
}
#endif

Value::Value(const char* s, bool conversion_preferred)
  : type_(VAL_STRING), s_(ACE_OS::strdup(s))
  , conversion_preferred_(conversion_preferred)
{}

Value::Value(const std::string& s, bool conversion_preferred)
  : type_(VAL_STRING), s_(ACE_OS::strdup(s.c_str()))
  , conversion_preferred_(conversion_preferred)
{}

#ifdef DDS_HAS_WCHAR
Value::Value(ACE_OutputCDR::from_wchar wc, bool conversion_preferred)
  : type_(VAL_INT), i_(wc.val_), conversion_preferred_(conversion_preferred)
{}

Value::Value(const std::wstring& s, bool conversion_preferred)
  : type_(VAL_STRING), s_(ACE_OS::strdup(ACE_Wide_To_Ascii(s.c_str()).char_rep()))
  , conversion_preferred_(conversion_preferred)
{}
#endif

Value::Value(const TAO::String_Manager& s, bool conversion_preferred)
  : type_(VAL_STRING), s_(ACE_OS::strdup(s.in()))
  , conversion_preferred_(conversion_preferred)
{}

Value::Value(const TAO::WString_Manager& s, bool conversion_preferred)
  : type_(VAL_STRING)
#ifdef DDS_HAS_WCHAR
  , s_(ACE_OS::strdup(ACE_Wide_To_Ascii(s.in()).char_rep()))
#else
  , s_(0)
#endif
  , conversion_preferred_(conversion_preferred)
{
#ifndef DDS_HAS_WCHAR
  ACE_UNUSED_ARG(s);
#endif
}

template<> bool& Value::get() { return b_; }
template<> int& Value::get() { return i_; }
template<> unsigned int& Value::get() { return u_; }
template<> ACE_INT64& Value::get() { return l_; }
template<> ACE_UINT64& Value::get() { return m_; }
template<> char& Value::get() { return c_; }
template<> double& Value::get() { return f_; }
template<> ACE_CDR::LongDouble& Value::get() { return ld_; }
template<> const char*& Value::get() { return s_; }

template<> const bool& Value::get() const { return b_; }
template<> const int& Value::get() const { return i_; }
template<> const unsigned int& Value::get() const { return u_; }
template<> const ACE_INT64& Value::get() const { return l_; }
template<> const ACE_UINT64& Value::get() const { return m_; }
template<> const char& Value::get() const { return c_; }
template<> const double& Value::get() const { return f_; }
template<> const ACE_CDR::LongDouble& Value::get() const { return ld_; }
template<> const char* const& Value::get() const { return s_; }

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
      tgt_.get<T>() = s;
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
      return lhs_.get<T>() == rhs;
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
      return lhs_.get<T>() < rhs;
    }

    const Value& lhs_;
  };

  struct Modulus : VisitorBase<Value> {
    explicit Modulus(const Value& lhs) : lhs_(lhs) {}

    bool operator()(const char*&) const
    {
      throw std::runtime_error(std::string(MOD) + " cannot be applied to strings");
    }

    Value operator()(const bool&) const
    {
      throw std::runtime_error(std::string(MOD) + " cannot be applied to booleans");
    }

    template<typename T>
    Value operator()(const T& rhs) const
    {
      return lhs_.get<T>() % rhs;
    }

    Value operator()(const double&) const
    {
      throw std::runtime_error(std::string(MOD) + " cannot be applied to doubles");
    }

    Value operator()(const ACE_CDR::LongDouble&) const
    {
      throw std::runtime_error(std::string(MOD) + " cannot be applied to ACE_CDR::LongDoubles");
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

Value
Value::operator%(const Value& v) const
{
  Value lhs = *this;
  Value rhs = v;
  conversion(lhs, rhs);
  Modulus visitor(lhs);
  return visit(visitor, rhs);
}

bool
Value::like(const Value& v) const
{
  if (type_ != VAL_STRING || v.type_ != VAL_STRING) {
    throw std::runtime_error("'like' operator called on non-string arguments.");
  }
  OPENDDS_STRING pattern(v.s_);
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

    void operator()(ACE_CDR::LongDouble& ld)
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
  OPENDDS_STRING asString;
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
