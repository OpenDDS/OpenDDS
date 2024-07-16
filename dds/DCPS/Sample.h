/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SAMPLE_H
#define OPENDDS_DCPS_SAMPLE_H

#include "Serializer.h"
#include "TypeSupportImpl.h"
#include "RcHandle_T.h"
#include "FilterEvaluator.h"
#include "XTypes/DynamicDataAdapterFwd.h"

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Sample;
typedef RcHandle<Sample> Sample_rch;
typedef RcHandle<const Sample> ConstSample_rch;

/**
 * Represents a sample that can either be an instance of a C++ generated type
 * from opendds_idl or a DynamicData. This is meant to be used by
 * DataReaderImpl and DataWriterImpl.
 */
class OpenDDS_Dcps_Export Sample : public RcObject {
public:
  enum Mutability {
    Mutable,
    ReadOnly
  };

  enum Extent {
    Full,
    KeyOnly,
    NestedKeyOnly
  };

  Sample()
    : mutability_(Mutable)
    , extent_(Full)
  {
  }

  Sample(Mutability mutability, Extent extent)
    : mutability_(mutability)
    , extent_(extent)
  {
    OPENDDS_ASSERT(extent != NestedKeyOnly);
  }

  virtual ~Sample() {}

  bool read_only() const
  {
    return mutability_ == ReadOnly;
  }

  bool key_only() const
  {
    return extent_ == KeyOnly;
  }

  virtual bool serialize(Serializer& ser) const = 0;
  virtual bool deserialize(Serializer& ser) = 0;
  virtual size_t serialized_size(const Encoding& enc) const = 0;
  virtual bool compare(const Sample& other) const = 0;
  virtual bool to_message_block(ACE_Message_Block& mb) const = 0;
  virtual bool from_message_block(const ACE_Message_Block& mb) = 0;
  virtual Sample_rch copy(Mutability mutability, Extent extent) const = 0;

  Sample_rch copy(Mutability mutability) const
  {
    return copy(mutability, extent_);
  }

#ifndef OPENDDS_SAFETY_PROFILE
  virtual DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr type) const = 0;
#endif

  virtual const void* native_data() const = 0;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual bool eval(FilterEvaluator& evaluator, const DDS::StringSeq& params) const = 0;
#endif

protected:
  Mutability mutability_;
  Extent extent_;
};

struct OpenDDS_Dcps_Export SampleRchCmp {
  bool operator()(const Sample_rch& lhs, const Sample_rch& rhs) const
  {
    return lhs->compare(*rhs);
  }
};

template <typename NativeType>
class Sample_T : public Sample {
public:
  typedef DDSTraits<NativeType> TraitsType;
  typedef MarshalTraits<NativeType> MarshalTraitsType;
  typedef RcHandle<Sample_T<NativeType> > Rch;
  typedef DCPS::KeyOnly<const NativeType> KeyOnlyType;
  typedef DCPS::KeyOnly<NativeType> MutableKeyOnlyType;

  explicit Sample_T(const NativeType& data, Extent extent = Full)
    : Sample(ReadOnly, extent)
    , owns_data_(false)
    , data_(&data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    , dynamic_data_(0)
#endif
  {
  }

  explicit Sample_T(const NativeType* data, Extent extent = Full)
    : Sample(ReadOnly, extent)
    , owns_data_(true)
    , data_(data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    , dynamic_data_(0)
#endif
  {
  }

  explicit Sample_T(NativeType& data, Extent extent = Full)
    : Sample(Mutable, extent)
    , owns_data_(false)
    , data_(&data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    , dynamic_data_(0)
#endif
  {
  }

  explicit Sample_T(NativeType* data, Extent extent = Full)
    : Sample(Mutable, extent)
    , owns_data_(true)
    , data_(data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    , dynamic_data_(0)
#endif
  {
  }

  virtual ~Sample_T()
  {
    if (owns_data_) {
      delete data_;
    }
  }

  const NativeType& data() const
  {
    return *data_;
  }

  NativeType& mutable_data() const
  {
    OPENDDS_ASSERT(!read_only());
    return *const_cast<NativeType*>(data_);
  }

  KeyOnlyType key_only_data() const
  {
    return KeyOnlyType(*data_);
  }

  MutableKeyOnlyType mutable_key_only_data() const
  {
    return MutableKeyOnlyType(mutable_data());
  }

  bool serialize(Serializer& ser) const
  {
    if (key_only()) {
      return ser << key_only_data();
    } else {
      return ser << data();
    }
  }

  bool deserialize(Serializer& ser)
  {
    if (key_only()) {
      return ser >> mutable_key_only_data();
    } else {
      return ser >> mutable_data();
    }
  }

  size_t serialized_size(const Encoding& enc) const
  {
    if (key_only()) {
      return OpenDDS::DCPS::serialized_size(enc, key_only_data());
    } else {
      return OpenDDS::DCPS::serialized_size(enc, data());
    }
  }

  bool compare(const Sample& other) const
  {
    const Sample_T<NativeType>* const other_same_kind =
      dynamic_cast<const Sample_T<NativeType>*>(&other);
    OPENDDS_ASSERT(other_same_kind);
    return typename TraitsType::LessThanType()(*data_, *other_same_kind->data_);
  }

  bool to_message_block(ACE_Message_Block& mb) const
  {
    return MarshalTraitsType::to_message_block(mb, data());
  }

  bool from_message_block(const ACE_Message_Block& mb)
  {
    return MarshalTraitsType::from_message_block(mutable_data(), mb);
  }

  Sample_rch copy(Mutability mutability, Extent extent) const
  {
    NativeType* new_data = new NativeType;
    *new_data = *data_;
    return dynamic_rchandle_cast<Sample>(mutability == ReadOnly ?
      make_rch<Sample_T<NativeType> >(const_cast<const NativeType*>(new_data), extent) :
      make_rch<Sample_T<NativeType> >(new_data, extent));
  }

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr type) const
  {
#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    if (!dynamic_data_ && data_) {
      dynamic_data_ = read_only() ?
        XTypes::get_dynamic_data_adapter<NativeType, NativeType>(type, *data_) :
        XTypes::get_dynamic_data_adapter<NativeType, NativeType>(type, mutable_data());
    }
    return dynamic_data_;
#  else
    ACE_UNUSED_ARG(type);
    return 0;
#  endif
  }
#endif

  const void* native_data() const
  {
    return data_;
  }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  bool eval(FilterEvaluator& evaluator, const DDS::StringSeq& params) const
  {
    return evaluator.eval(*data_, params);
  }
#endif

private:
  const bool owns_data_;
  const NativeType* data_;
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  mutable DDS::DynamicData_var dynamic_data_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
