/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DCPS_UTILS_H
#define OPENDDS_DCPS_DCPS_UTILS_H

#include "dcps_export.h"

#include "Serializer.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export const char* retcode_to_string(DDS::ReturnCode_t value);

OpenDDS_Dcps_Export const char* topicstatus_to_string(OpenDDS::DCPS::TopicStatus value);

OpenDDS_Dcps_Export
bool
is_wildcard(const char *str);

/// Increments the count of occurrences of the incompatible policy
///  for the status
OpenDDS_Dcps_Export
void
increment_incompatibility_count(OpenDDS::DCPS::IncompatibleQosStatus* status,
                                DDS::QosPolicyId_t incompatible_policy);

/// Compares whether a publication and subscription are compatible
/// by comparing their constituent parts.
OpenDDS_Dcps_Export
bool compatibleQOS(OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
                   OpenDDS::DCPS::IncompatibleQosStatus* readerStatus,
                   const OpenDDS::DCPS::TransportLocatorSeq& pubTLS,
                   const OpenDDS::DCPS::TransportLocatorSeq& subTLS,
                   DDS::DataWriterQos const * const writerQos,
                   DDS::DataReaderQos const * const readerQos,
                   DDS::PublisherQos const * const pubQos,
                   DDS::SubscriberQos const * const subQos);

OpenDDS_Dcps_Export
bool
compatibleQOS(const DDS::DataWriterQos * writerQos,
              const DDS::DataReaderQos * readerQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus = 0,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus = 0);

OpenDDS_Dcps_Export
bool
compatibleQOS(const DDS::PublisherQos * pubQos,
              const DDS::SubscriberQos * subQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus = 0,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus = 0);

OpenDDS_Dcps_Export
bool
matching_partitions(const DDS::PartitionQosPolicy& pub,
                    const DDS::PartitionQosPolicy& sub);

// Should check the association of the entity QoS ?
// The changeable QoS that is supported currently and affect the association
// establishment is deadline QoS and partition QoS.
OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::DataReaderQos & qos1,
                                          const DDS::DataReaderQos & qos2);

OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::DataWriterQos & qos1,
                                          const DDS::DataWriterQos & qos2);

OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::SubscriberQos & qos1,
                                          const DDS::SubscriberQos & qos2);

OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::PublisherQos & qos1,
                                          const DDS::PublisherQos & qos2);

OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::TopicQos & qos1,
                                          const DDS::TopicQos & qos2);

OpenDDS_Dcps_Export
bool should_check_association_upon_change(const DDS::DomainParticipantQos & qos1,
                                          const DDS::DomainParticipantQos & qos2);

OpenDDS_Dcps_Export
bool repr_to_encoding_kind(DDS::DataRepresentationId_t repr, Encoding::Kind& kind);

OpenDDS_Dcps_Export
DDS::DataRepresentationIdSeq get_effective_data_rep_qos(const DDS::DataRepresentationIdSeq& qos, bool reader);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DCPS_UTILS_H */
