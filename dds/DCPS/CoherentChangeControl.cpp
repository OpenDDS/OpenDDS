/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

#include "CoherentChangeControl.h"
#include "Serializer.h"
#include "GuidConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/PoolAllocator.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>


#if !defined (__ACE_INLINE__)
#include "CoherentChangeControl.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_CDR::Boolean
operator<<(Serializer& serializer, CoherentChangeControl& value)
{
  if (!(serializer << value.coherent_samples_.num_samples_) ||
      !(serializer << value.coherent_samples_.last_sample_) ||
      !(serializer << ACE_OutputCDR::from_boolean(value.group_coherent_))) {
    return false;
  }

  if (value.group_coherent_) {
    if (!(serializer << value.publisher_id_) ||
        !(serializer << static_cast<ACE_UINT32>(value.group_coherent_samples_.size()))) {
      return false;
    }
    GroupCoherentSamples::iterator itEnd = value.group_coherent_samples_.end();
    for (GroupCoherentSamples::iterator it =
           value.group_coherent_samples_.begin(); it != itEnd; ++it) {
      if (!(serializer << it->first) ||
          !(serializer << it->second.num_samples_) ||
          !(serializer << it->second.last_sample_)) {
        return false;
      }
    }
  }

  return serializer.good_bit();
}

ACE_CDR::Boolean
operator>>(Serializer& serializer, CoherentChangeControl& value)
{
  if (!(serializer >> value.coherent_samples_.num_samples_) ||
      !(serializer >> value.coherent_samples_.last_sample_) ||
      !(serializer >> ACE_InputCDR::to_boolean(value.group_coherent_))) {
    return false;
  }

  if (value.group_coherent_) {
    ACE_UINT32 sz = 0;
    if (!(serializer >> value.publisher_id_) ||
        !(serializer >> sz)) {
      return false;
    }

    for (ACE_UINT32 i = 0; i < sz; ++i) {
      PublicationId writer(GUID_UNKNOWN);
      ACE_UINT32 num_sample = 0;
      ACE_INT16 last_sample = 0;

      if (!(serializer >> writer) ||
          !(serializer >> num_sample) ||
          !(serializer >> last_sample)) {
        return false;
      }

      std::pair<GroupCoherentSamples::iterator, bool> pair =
        value.group_coherent_samples_.insert(GroupCoherentSamples::value_type(
          writer, WriterCoherentSample(num_sample, last_sample)));
      if (!pair.second) {
        return false;
      }
    }
  }

  return serializer.good_bit();
}

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const CoherentChangeControl& value)
{
  str << "num_samples: " << std::dec << value.coherent_samples_.num_samples_
      << ", last_sample: " << value.coherent_samples_.last_sample_.getValue()
      << ", ";
  if (value.group_coherent_) {
    GuidConverter converter(value.publisher_id_);
    str << "publisher: " << std::dec << OPENDDS_STRING(converter).c_str() << ", ";
    str << "group size: " << std::dec << value.group_coherent_samples_.size()
        << ", ";
    GroupCoherentSamples::const_iterator itEnd =
      value.group_coherent_samples_.end();
    for (GroupCoherentSamples::const_iterator it =
           value.group_coherent_samples_.begin(); it != itEnd; ++it) {
      GuidConverter converter(it->first);
      str << "writer: " << OPENDDS_STRING(converter).c_str() << ", "
          << "num_samples: " << it->second.num_samples_ << ", "
          << "last_sample: " << it->second.last_sample_.getValue()  << std::endl;
    }
  }
  return str;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE
