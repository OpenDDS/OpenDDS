/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_SAMPLE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_SAMPLE_H

#include <dds/DCPS/Definitions.h>

#if !OPENDDS_CONFIG_SAFETY_PROFILE

#include <dds/DCPS/Sample.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicSample : public DCPS::Sample {
public:
  DynamicSample();
  DynamicSample(const DynamicSample&);
  DynamicSample& operator=(const DynamicSample&);

  DynamicSample(DDS::DynamicData_ptr data,
                Mutability mutability, Extent extent)
    : Sample(mutability, extent)
    , data_(DDS::DynamicData::_duplicate(data))
  {
  }

  DynamicSample(DDS::DynamicData_ptr data, Extent extent)
    : Sample(Sample::ReadOnly, extent)
    , data_(DDS::DynamicData::_duplicate(data))
  {
  }

  DynamicSample(DDS::DynamicData_ptr data)
    : Sample(Sample::ReadOnly, Sample::Full)
    , data_(DDS::DynamicData::_duplicate(data))
  {
  }

  void set_key_only(bool k) { extent_ = k ? Sample::KeyOnly : Sample::Full; }

  bool serialize(DCPS::Serializer& ser) const;
  bool deserialize(DCPS::Serializer& ser);
  size_t serialized_size(const DCPS::Encoding& enc) const;
  bool compare(const DCPS::Sample& other) const;

  bool to_message_block(ACE_Message_Block&) const
  {
    // Not needed
    OPENDDS_ASSERT(false);
    return false;
  }

  bool from_message_block(const ACE_Message_Block&)
  {
    // Not needed
    OPENDDS_ASSERT(false);
    return false;
  }

  DCPS::Sample_rch copy(DCPS::Sample::Mutability mutability, DCPS::Sample::Extent extent) const
  {
    DDS::DynamicData_var dd = data_->clone();
    return DCPS::make_rch<DynamicSample>(dd, mutability, extent);
  }

  DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr) const
  {
    return data_;
  }

  DDS::DynamicData_var dynamic_data() const
  {
    return data_;
  }

  const void* native_data() const
  {
    return 0;
  }

#if OPENDDS_CONFIG_CONTENT_SUBSCRIPTION
  bool eval(DCPS::FilterEvaluator& evaluator, const DDS::StringSeq& params) const
  {
    return evaluator.eval(*this, params);
  }
#endif

  struct KeyLessThan {
    bool operator()(const DynamicSample& lhs, const DynamicSample& rhs) const
    {
      return lhs.compare(rhs);
    }
  };

protected:
  DDS::DynamicData_var data_;
};

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
