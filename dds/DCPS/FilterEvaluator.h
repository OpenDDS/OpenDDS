/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FILTER_EVALUATOR_H
#define OPENDDS_DCPS_FILTER_EVALUATOR_H

#include "Definitions.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DdsDcpsInfrastructureC.h"
#include "PoolAllocator.h"
#include "Comparator_T.h"
#include "RcObject.h"

#include <dds/DdsDynamicDataC.h>

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MetaStruct;
class TypeSupportImpl;

template<typename T>
const MetaStruct& getMetaStruct();

template<typename T>
struct MarshalTraits;

struct OpenDDS_Dcps_Export Value {
  Value(bool b, bool conversion_preferred = false);
  Value(int i, bool conversion_preferred = false);
  Value(unsigned int u, bool conversion_preferred = false);
  Value(ACE_INT64 l, bool conversion_preferred = false);
  Value(ACE_UINT64 m, bool conversion_preferred = false);
  Value(char c, bool conversion_preferred = false);
  Value(double f, bool conversion_preferred = false);
  Value(ACE_CDR::LongDouble ld, bool conversion_preferred = false);
#ifdef NONNATIVE_LONGDOUBLE
  Value(long double ld, bool conversion_preferred = false);
#endif
  Value(const char* s, bool conversion_preferred = false);
  Value(const std::string& s, bool conversion_preferred = false);
#ifdef DDS_HAS_WCHAR
  Value(ACE_OutputCDR::from_wchar wc, bool conversion_preferred = false);
  Value(const std::wstring& s, bool conversion_preferred = false);
#endif
  Value(const TAO::String_Manager& s, bool conversion_preferred = false);
  Value(const TAO::WString_Manager& s, bool conversion_preferred = false);

  ~Value();
  Value(const Value& v);
  Value& operator=(const Value& v);
  void swap(Value& other);

  bool operator==(const Value& v) const;
  bool operator<(const Value& v) const;
  bool like(const Value& v) const;
  Value operator%(const Value& v) const;

  enum Type {VAL_BOOL, VAL_INT, VAL_UINT, VAL_I64, VAL_UI64, VAL_FLOAT,
             VAL_LNGDUB, VAL_LARGEST_NUMERIC = VAL_LNGDUB,
             VAL_CHAR, VAL_STRING};
  bool convert(Type t);
  static void conversion(Value& lhs, Value& rhs);
  template<typename T> T& get();
  template<typename T> const T& get() const;

  Type type_;
  union {
    bool b_;
    int i_;
    unsigned int u_;
    ACE_INT64 l_;
    ACE_UINT64 m_;
    char c_;
    double f_;
    ACE_CDR::LongDouble ld_;
    const char* s_;
  };
  bool conversion_preferred_;
};

class OpenDDS_Dcps_Export FilterEvaluator : public virtual RcObject {
public:

  struct AstNodeWrapper;

  FilterEvaluator(const char* filter, bool allowOrderBy);

  explicit FilterEvaluator(const AstNodeWrapper& yardNode);

  ~FilterEvaluator();

  OPENDDS_VECTOR(OPENDDS_STRING) getOrderBys() const;

  bool hasFilter() const;

  bool usesExtendedGrammar() const { return extended_grammar_; }

  size_t number_parameters() const { return number_parameters_; }

  bool has_non_key_fields(const TypeSupportImpl& ts) const;

  /**
   * Returns true if the unserialized sample matches the filter.
   */
  template<typename T>
  bool eval(const T& sample, const DDS::StringSeq& params) const
  {
    DeserializedForEval data(&sample, getMetaStruct<T>(), params);
    return eval_i(data);
  }

  /**
   * Returns true if the serialized sample matches the filter.
   */
  bool eval(ACE_Message_Block* serializedSample, Encoding encoding,
            const TypeSupportImpl& typeSupport,
            const DDS::StringSeq& params) const
  {
    SerializedForEval data(serializedSample, typeSupport, params, encoding);
    return eval_i(data);
  }

  class EvalNode;
  class Operand;

  struct OpenDDS_Dcps_Export DataForEval {
    DataForEval(const MetaStruct& meta, const DDS::StringSeq& params)
      : meta_(meta), params_(params) {}
    virtual ~DataForEval();
    virtual Value lookup(const char* field) const = 0;
    const MetaStruct& meta_;
    const DDS::StringSeq& params_;
  private:
    DataForEval(const DataForEval&);
    DataForEval& operator=(const DataForEval&);
  };

private:
  FilterEvaluator(const FilterEvaluator&);
  FilterEvaluator& operator=(const FilterEvaluator&);

  EvalNode* walkAst(const AstNodeWrapper& node);
  Operand* walkOperand(const AstNodeWrapper& node);

  struct OpenDDS_Dcps_Export DeserializedForEval : DataForEval {
    DeserializedForEval(const void* data, const MetaStruct& meta,
                        const DDS::StringSeq& params)
      : DataForEval(meta, params), deserialized_(data) {}
    virtual ~DeserializedForEval();
    Value lookup(const char* field) const;
    const void* const deserialized_;
  };

  struct SerializedForEval : DataForEval {
    SerializedForEval(ACE_Message_Block* data, const TypeSupportImpl& type_support,
                      const DDS::StringSeq& params, Encoding encoding);
    Value lookup(const char* field) const;
    ACE_Message_Block* serialized_;
    Encoding encoding_;
    const TypeSupportImpl& type_support_;
    mutable OPENDDS_MAP(OPENDDS_STRING, Value) cache_;
    Extensibility exten_;
  };

  bool eval_i(DataForEval& data) const;

  bool extended_grammar_;
  EvalNode* filter_root_;
  OPENDDS_VECTOR(OPENDDS_STRING) order_bys_;
  /// Number of parameters used in the filter, this should
  /// match the number of values passed when evaluating the filter
  size_t number_parameters_;

};

class OpenDDS_Dcps_Export MetaStruct {
public:
  virtual ~MetaStruct();

  virtual Value getValue(const void* stru, const char* fieldSpec) const = 0;
  virtual Value getValue(Serializer& ser, const char* fieldSpec, const TypeSupportImpl* ts = 0) const = 0;

  virtual ComparatorBase::Ptr create_qc_comparator(const char* fieldSpec,
    ComparatorBase::Ptr next) const = 0;

  ComparatorBase::Ptr create_qc_comparator(const char* fieldSpec) const
  { return create_qc_comparator(fieldSpec, ComparatorBase::Ptr()); }

#ifndef OPENDDS_NO_MULTI_TOPIC
  virtual size_t numDcpsKeys() const = 0;

  virtual bool compare(const void* lhs, const void* rhs,
                       const char* fieldSpec) const = 0;

  virtual const char** getFieldNames() const = 0;

  virtual void assign(void* lhs, const char* lhsFieldSpec,
                      const void* rhs, const char* rhsFieldSpec,
                      const MetaStruct& rhsMeta) const = 0;

  virtual const void* getRawField(const void* stru,
                                  const char* fieldSpec) const = 0;

  virtual void* allocate() const = 0;
  virtual void deallocate(void* stru) const = 0;
#endif /* OPENDDS_NO_MULTI_TOPIC */
};

/// Each user-defined struct type will have an instantiation of this template
/// generated by opendds_idl.
template<typename T>
struct MetaStructImpl;

}  }

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#endif
