/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UTILS_H
#define DCPS_UTILS_H

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Increments the count of occurances of the incompatible policy
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
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus);

OpenDDS_Dcps_Export
bool
compatibleQOS(const DDS::PublisherQos * pubQos,
              const DDS::SubscriberQos * subQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus);

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

}}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_UTILS_H */
