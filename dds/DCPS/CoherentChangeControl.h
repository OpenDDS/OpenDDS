/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_COHERENT_CHANGE_CONTROL_H
#define OPENDDS_DCPS_COHERENT_CHANGE_CONTROL_H

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

#include "Definitions.h"
#include "GuidUtils.h"
#include "Serializer.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "PoolAllocator.h"
#include <iosfwd>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct WriterCoherentSample {
  WriterCoherentSample (ACE_UINT32 num_sample = 0,
                        SequenceNumber last_sample = SequenceNumber());

  void reset ();
  ACE_UINT32 num_samples_;
  SequenceNumber last_sample_;
};

typedef OPENDDS_MAP_CMP(PublicationId, WriterCoherentSample, GUID_tKeyLessThan) GroupCoherentSamples;

/// End Coherent Change message.
struct OpenDDS_Dcps_Export CoherentChangeControl {

  WriterCoherentSample  coherent_samples_;
  bool                  group_coherent_;
  RepoId                publisher_id_;
  GroupCoherentSamples  group_coherent_samples_;

  CoherentChangeControl() ;

  /// Similar to IDL compiler generated methods.
  size_t max_marshaled_size() ;

  void reset ();
};


/// Marshal/Insertion into a buffer.
OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator<< (OpenDDS::DCPS::Serializer& serializer, OpenDDS::DCPS::CoherentChangeControl& value);

OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator>> (OpenDDS::DCPS::Serializer& serializer, OpenDDS::DCPS::CoherentChangeControl& value);

/// Message header insertion onto an ostream.
OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const OpenDDS::DCPS::CoherentChangeControl& value);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "CoherentChangeControl.inl"
#endif /* __ACE_INLINE__ */

#endif

#endif  /* OPENDDS_DCPS_COHERENT_CHANGE_CONTROL_H */
