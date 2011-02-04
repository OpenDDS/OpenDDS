/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "CoherentChangeControl.h"
#include "Serializer.h"
#include "RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>


#if !defined (__ACE_INLINE__)
#include "CoherentChangeControl.inl"
#endif /* __ACE_INLINE__ */


namespace OpenDDS {
namespace DCPS {

ACE_CDR::Boolean
operator<< (Serializer& serializer, CoherentChangeControl& value)
{
  serializer << value.coherent_samples_.num_samples_ ;
  serializer << value.coherent_samples_.last_sample_.getValue();
  serializer << ACE_OutputCDR::from_boolean (value.group_coherent_);

  if (value.group_coherent_) {
    serializer << value.publisher_id_;
    ACE_UINT32 sz = value.group_coherent_samples_.size ();
    serializer << sz;
    GroupCoherentSamples::iterator itEnd = value.group_coherent_samples_.end ();
    for (GroupCoherentSamples::iterator
         it = value.group_coherent_samples_.begin (); it != itEnd; ++it) {
        serializer << it->first;
        serializer << it->second.num_samples_;
        serializer << it->second.last_sample_.getValue();
    }
  }

  return serializer.good_bit() ;
}

ACE_CDR::Boolean
operator>> (Serializer& serializer, CoherentChangeControl& value)
{
  serializer >> value.coherent_samples_.num_samples_;
  if (serializer.good_bit() != true) return false;

  SequenceNumber::Value seqNum;
  serializer >> seqNum;
  value.coherent_samples_.last_sample_.setValue(seqNum);
  if (serializer.good_bit() != true) return false;

  serializer >> ACE_InputCDR::to_boolean(value.group_coherent_);
  if (serializer.good_bit() != true) return false;

  if (value.group_coherent_) {
    serializer >> value.publisher_id_;
    if (serializer.good_bit() != true) return false;

    ACE_UINT32 sz = 0;
    serializer >> sz;
    if (serializer.good_bit() != true) return false;

    for (ACE_UINT32 i = 0; i < sz; ++i) {
      PublicationId writer(GUID_UNKNOWN);
      ACE_UINT32     num_sample = 0;
      ACE_INT16     last_sample = 0;

      serializer >> writer;
      if (serializer.good_bit() != true) return false;
      serializer >> num_sample;
      if (serializer.good_bit() != true) return false;
      serializer >> last_sample;
      if (serializer.good_bit() != true) return false;

      std::pair<GroupCoherentSamples::iterator, bool> pair =
        value.group_coherent_samples_.insert (GroupCoherentSamples::value_type (
          writer, WriterCoherentSample(num_sample, last_sample)));
      if (!pair.second) {
        return false;
      }
    }
  }

  return true;
}

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const CoherentChangeControl& value)
{
  str << "num_samples: " << std::dec << value.coherent_samples_.num_samples_ << ", "
      << "last_sample: " << value.coherent_samples_.last_sample_.getValue() << ", ";
  if (value.group_coherent_) {
    RepoIdConverter converter(value.publisher_id_);
    str << "publisher: " << std::dec << std::string(converter).c_str() << ", ";
    str << "group size: " << std::dec << value.group_coherent_samples_.size () << ", ";
    GroupCoherentSamples::const_iterator itEnd
      = value.group_coherent_samples_.end ();
    for (GroupCoherentSamples::const_iterator it = value.group_coherent_samples_.begin ();
         it != itEnd; ++it) {
      RepoIdConverter converter(it->first);
      str << "writer: " << std::string(converter).c_str() << ", "
          << "num_samples: " << it->second.num_samples_ << ", "
          << "last_sample: " << it->second.last_sample_.getValue()  << std::endl;
    }
  }
  return str;
}

} // namespace DCPS
} // namespace OpenDDS
