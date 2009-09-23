/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UTILS_H
#define DCPS_UTILS_H

#include  "inforepo_export.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/// Increments the count of occurances of the incompatible policy
///  for the status
void
increment_incompatibility_count(OpenDDS::DCPS::IncompatibleQosStatus* status,
                                DDS::QosPolicyId_t incompatible_policy);

/// Compares whether a publication and subscription are compatible
bool compatibleQOS(DCPS_IR_Publication *  publication,
                   DCPS_IR_Subscription * subscription);

bool
compatibleQOS(const DDS::DataWriterQos * writerQos,
              const DDS::DataReaderQos * readerQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus);

bool
compatibleQOS(const DDS::PublisherQos * pubQos,
              const DDS::SubscriberQos * subQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus);

// Should check the compatibility of the entity QoS ?
// The changeable QoS that is supported currently and should be compatible is
// deadline QoS.
bool should_check_compatibility_upon_change(const DDS::DataReaderQos & qos1,
                                            const DDS::DataReaderQos & qos2);

bool should_check_compatibility_upon_change(const DDS::DataWriterQos & qos1,
                                            const DDS::DataWriterQos & qos2);

bool should_check_compatibility_upon_change(const DDS::SubscriberQos & qos1,
                                            const DDS::SubscriberQos & qos2);

bool should_check_compatibility_upon_change(const DDS::PublisherQos & qos1,
                                            const DDS::PublisherQos & qos2);

bool should_check_compatibility_upon_change(const DDS::TopicQos & qos1,
                                            const DDS::TopicQos & qos2);

bool should_check_compatibility_upon_change(const DDS::DomainParticipantQos & qos1,
                                            const DDS::DomainParticipantQos & qos2);

// Should check the association of the entity QoS ?
// The changeable QoS that is supported currently and affect the association
// establishment is deadline QoS and partition QoS.
bool should_check_association_upon_change(const DDS::DataReaderQos & qos1,
                                          const DDS::DataReaderQos & qos2);

bool should_check_association_upon_change(const DDS::DataWriterQos & qos1,
                                          const DDS::DataWriterQos & qos2);

bool should_check_association_upon_change(const DDS::SubscriberQos & qos1,
                                          const DDS::SubscriberQos & qos2);

bool should_check_association_upon_change(const DDS::PublisherQos & qos1,
                                          const DDS::PublisherQos & qos2);

bool should_check_association_upon_change(const DDS::TopicQos & qos1,
                                          const DDS::TopicQos & qos2);

bool should_check_association_upon_change(const DDS::DomainParticipantQos & qos1,
                                          const DDS::DomainParticipantQos & qos2);

#endif /* DCPS_UTILS_H */
