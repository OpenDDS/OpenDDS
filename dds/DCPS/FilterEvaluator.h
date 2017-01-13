/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FILTER_EVALUATOR_H
#define OPENDDS_DCPS_FILTER_EVALUATOR_H

#include "dds/DCPS/Definitions.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/PoolAllocator.h"
#include "Comparator_T.h"
#include "RcObject_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MetaStruct;

template<typename T>
const MetaStruct& getMetaStruct();

struct OpenDDS_Dcps_Export Value {
  Value(bool b, bool conversion_preferred = false);
  Value(int i, bool conversion_preferred = false);
  Value(unsigned int u, bool conversion_preferred = false);
  Value(ACE_INT64 l, bool conversion_preferred = false);
  Value(ACE_UINT64 m, bool conversion_preferred = false);
  Value(char c, bool conversion_preferred = false);
  Value(double f, bool conversion_preferred = false);
  Value(ACE_CDR::LongDouble ld, bool conversion_preferred = false);
  Value(const char* s, bool conversion_preferred = false);
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

class OpenDDS_Dcps_Export FilterEvaluator : public RcObject<ACE_SYNCH_MUTEX> {
public:

  struct AstNodeWrapper;

  FilterEvaluator(const char* filter, bool allowOrderBy);

  explicit FilterEvaluator(const AstNodeWrapper& yardNode);

  ~FilterEvaluator();

  OPENDDS_VECTOR(OPENDDS_STRING) getOrderBys() const;

  bool hasFilter() const;

  bool usesExtendedGrammar() const { return extended_grammar_; }

  template<typename T>
  bool eval(const T& sample, const DDS::StringSeq& params) const
  {
    DeserializedForEval data(&sample, getMetaStruct<T>(), params);
    return eval_i(data);
  }

  bool eval(ACE_Message_Block* serializedSample, bool swap_bytes,
            bool cdr_encap, const MetaStruct& meta,
            const DDS::StringSeq& params) const
  {
    SerializedForEval data(serializedSample, meta, params,
                           swap_bytes, cdr_encap);
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
    SerializedForEval(ACE_Message_Block* data, const MetaStruct& meta,
                      const DDS::StringSeq& params, bool swap, bool cdr)
      : DataForEval(meta, params), serialized_(data), swap_(swap), cdr_(cdr) {}
    Value lookup(const char* field) const;
    ACE_Message_Block* serialized_;
    bool swap_, cdr_;
    mutable OPENDDS_MAP(OPENDDS_STRING, Value) cache_;
  };

  bool eval_i(DataForEval& data) const;

  bool extended_grammar_;
  EvalNode* filter_root_;
  OPENDDS_VECTOR(OPENDDS_STRING) order_bys_;
};

class OpenDDS_Dcps_Export MetaStruct {
public:
  virtual ~MetaStruct();

  virtual Value getValue(const void* stru, const char* fieldSpec) const = 0;
  virtual Value getValue(Serializer& ser, const char* fieldSpec) const = 0;

  virtual ComparatorBase::Ptr create_qc_comparator(const char* fieldSpec,
    ComparatorBase::Ptr next) const = 0;

  ComparatorBase::Ptr create_qc_comparator(const char* fieldSpec) const
  { return create_qc_comparator(fieldSpec, ComparatorBase::Ptr()); }

  virtual const char** getFieldNames() const = 0;

  virtual size_t numDcpsKeys() const = 0;

  virtual bool compare(const void* lhs, const void* rhs,
                       const char* fieldSpec) const = 0;

  virtual void assign(void* lhs, const char* lhsFieldSpec,
                      const void* rhs, const char* rhsFieldSpec,
                      const MetaStruct& rhsMeta) const = 0;

  virtual const void* getRawField(const void* stru,
                                  const char* fieldSpec) const = 0;

  virtual void* allocate() const = 0;
  virtual void deallocate(void* stru) const = 0;
};

/// Each user-defined struct type will have an instantiation of this template
/// generated by opendds_idl.
template<typename T>
struct MetaStructImpl;

}  }

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#endif
