/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ABSTRACT_SAMPLE_H
#define OPENDDS_DCPS_ABSTRACT_SAMPLE_H

#include "Serializer.h"
#include "TypeSupportImpl.h"
#include "RcHandle_T.h"
#include "FilterEvaluator.h"
#include "XTypes/DynamicDataAdapter.h"

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class AbstractSample;
typedef RcHandle<AbstractSample> AbstractSample_rch;

enum SampleMutability {
  SampleMutable,
  SampleReadOnly
};

enum SampleExtent {
  SampleFull,
  SampleKeyOnly
};

/**
 * Represents a sample that can either be an instance of a C++ generated type
 * from opendds_idl or a DynamicData. This is meant to be used by
 * DataReaderImpl and DataWriterImpl.
 */
class OpenDDS_Dcps_Export AbstractSample : public virtual RcObject {
public:
  AbstractSample(SampleMutability mutability, SampleExtent extent)
  : mutability_(mutability)
  , extent_(extent)
  {
  }

  virtual ~AbstractSample() {}

  bool read_only() const
  {
    return mutability_ == SampleReadOnly;
  }

  bool key_only() const
  {
    return extent_ == SampleKeyOnly;
  }

  virtual bool serialize(Serializer& ser) const = 0;
  virtual bool deserialize(Serializer& ser) const = 0;
  virtual size_t serialized_size(const Encoding& enc) const = 0;
  virtual bool compare(const AbstractSample& other) const = 0;
  virtual bool to_message_block(ACE_Message_Block& mb) = 0;
  virtual bool from_message_block(const ACE_Message_Block& mb) = 0;
  virtual AbstractSample_rch copy(SampleMutability mutability, SampleExtent extent) const = 0;
  AbstractSample_rch copy(SampleMutability mutability) const
  {
    return copy(mutability, extent_);
  }
#ifndef OPENDDS_SAFETY_PROFILE
  virtual DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr type) = 0;
#endif
  virtual const void* native_data() = 0;
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual bool eval(FilterEvaluator& evaluator, const DDS::StringSeq& params) const = 0;
#endif

protected:
  const SampleMutability mutability_;
  const SampleExtent extent_;
};

struct OpenDDS_Dcps_Export AbstractSampleRchCmp {
  bool operator()(const AbstractSample_rch& lhs, const AbstractSample_rch& rhs) const
  {
    return lhs->compare(*rhs);
  }
};

template <typename NativeType>
class Sample : public AbstractSample {
public:
  typedef DDSTraits<NativeType> TraitsType;
  typedef MarshalTraits<NativeType> MarshalTraitsType;
  typedef RcHandle<Sample<NativeType> > Rch;
  typedef KeyOnly<const NativeType> KeyOnlyType;
  typedef KeyOnly<NativeType> MutableKeyOnlyType;
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  typedef XTypes::DynamicDataAdapter<NativeType> DynamicDataImpl;
#endif

  explicit Sample(const NativeType& data, SampleExtent extent = SampleFull)
  : AbstractSample(SampleReadOnly, extent)
  , owns_data_(false)
  , data_(&data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  , dynamic_data_(0)
#endif
  {
  }

  explicit Sample(const NativeType* data, SampleExtent extent = SampleFull)
  : AbstractSample(SampleReadOnly, extent)
  , owns_data_(true)
  , data_(data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  , dynamic_data_(0)
#endif
  {
  }

  explicit Sample(NativeType& data, SampleExtent extent = SampleFull)
  : AbstractSample(SampleMutable, extent)
  , owns_data_(false)
  , data_(&data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  , dynamic_data_(0)
#endif
  {
  }

  explicit Sample(NativeType* data, SampleExtent extent = SampleFull)
  : AbstractSample(SampleMutable, extent)
  , owns_data_(true)
  , data_(data)
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  , dynamic_data_(0)
#endif
  {
  }

  virtual ~Sample()
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

  bool deserialize(Serializer& ser) const
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

  bool compare(const AbstractSample& other) const
  {
    const Sample<NativeType>* const other_same_kind =
      dynamic_cast<const Sample<NativeType>*>(&other);
    OPENDDS_ASSERT(other_same_kind);
    return typename TraitsType::LessThanType()(*data_, *other_same_kind->data_);
  }

  bool to_message_block(ACE_Message_Block& mb)
  {
    return MarshalTraitsType::to_message_block(mb, data());
  }

  bool from_message_block(const ACE_Message_Block& mb)
  {
    return MarshalTraitsType::from_message_block(mutable_data(), mb);
  }

  AbstractSample_rch copy(SampleMutability mutability, SampleExtent extent) const
  {
    NativeType* new_data = new NativeType;
    *new_data = *data_;
    return dynamic_rchandle_cast<AbstractSample>(mutability == SampleReadOnly ?
      make_rch<Sample<NativeType> >(const_cast<const NativeType*>(new_data), extent) :
      make_rch<Sample<NativeType> >(new_data, extent));
  }

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr type)
  {
#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    if (type && !dynamic_data_) {
      dynamic_data_ = new DynamicDataImpl(type, getMetaStruct<NativeType>(), *data_);
    }
    return dynamic_data_;
#  else
    ACE_UNUSED_ARG(type);
    return 0;
#  endif
  }
#endif

  const void* native_data()
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
  DDS::DynamicData_var dynamic_data_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
