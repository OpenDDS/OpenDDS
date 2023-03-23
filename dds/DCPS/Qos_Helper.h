/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QOS_HELPER_H
#define OPENDDS_DCPS_QOS_HELPER_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsTopicC.h"
#include "Time_Helper.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class Qos_Helper
 *
 * @brief This class implements methods that verify whether a qos is
 *        valid, consistent and changeable.
 *
 * valid - the values are in acceptable ranges without respect
 *         to any other values.
 *
 * consistent - the values are consistent with each other.
 *         The spec sometimes calls this "compatible" but I
 *         this compatible should be reserved for matching
 *         QoS of subscriptions and publications.
 *         The spec is confusing in its inconsistency of the
 *         use of "compatible" and "consistent".
 *
 * The qos supported in current implementation:
 *         Liveliness  :   kind = AUTOMATIC
 *         Reliability :   kind = RELIABLE | BEST_EFFORT
 *                         max_blocking_time
 *         History     :   kind = KEEP_ALL | KEEP_LAST
 *                         depth > 1
 *         RESOURCE_LIMITS : max_samples_per_instance
 *
 * Other than these supported qos, any qos that is different from the
 * initial value is invalid.
 *
 * @note Since in the first implementation of DSS in TAO a limited
 *       number of QoS values are allowed to be modified, the
 *       consistency test on QoS settings have not been
 *       implemented to check future "valid" QoS values.
 *
 * @note None of the supported QoS in the first implementation are
 *       changeable. The changed value will be checked per the QoS
 *       table in the DDS specification.
 */
class OpenDDS_Dcps_Export Qos_Helper {
public:

  static bool consistent(
    const DDS::ResourceLimitsQosPolicy& resource_limits,
    const DDS::HistoryQosPolicy& history);

  static bool consistent(
    const DDS::DeadlineQosPolicy& deadline,
    const DDS::TimeBasedFilterQosPolicy& time_based_filter);

  static bool consistent(const DDS::DomainParticipantQos& qos);

  static bool consistent(const DDS::TopicQos& qos);

  static bool consistent(const DDS::DataWriterQos& qos);

  static bool consistent(const DDS::PublisherQos& qos);

  static bool consistent(const DDS::DataReaderQos& qos);

  static bool consistent(const DDS::SubscriberQos& qos);

  static bool consistent(const DDS::DomainParticipantFactoryQos & qos);

  static bool valid(const DDS::UserDataQosPolicy& qos);

  static bool valid(const DDS::TopicDataQosPolicy & qos);

  static bool valid(const DDS::GroupDataQosPolicy& qos);

  static bool valid(const DDS::TransportPriorityQosPolicy& qos);

  static bool valid(const DDS::LifespanQosPolicy& qos);

  static bool valid(const DDS::DurabilityQosPolicy& qos);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  static bool valid(const DDS::DurabilityServiceQosPolicy& qos);
#endif

  static bool valid(const DDS::PresentationQosPolicy& qos);

  static bool valid(const DDS::DeadlineQosPolicy& qos);

  static bool valid(const DDS::LatencyBudgetQosPolicy& qos);

  static bool valid(const DDS::OwnershipQosPolicy& qos);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  static bool valid(const DDS::OwnershipStrengthQosPolicy& qos);
#endif

  static bool valid(const DDS::LivelinessQosPolicy& qos);

  static bool valid(const DDS::TimeBasedFilterQosPolicy& qos);

  static bool valid(const DDS::PartitionQosPolicy& qos);

  static bool valid(const DDS::ReliabilityQosPolicy& qos);

  static bool valid(const DDS::DestinationOrderQosPolicy& qos);

  static bool valid(const DDS::HistoryQosPolicy& qos);

  static bool valid(const DDS::ResourceLimitsQosPolicy& qos);

  static bool valid(const DDS::EntityFactoryQosPolicy& qos);

  static bool valid(const DDS::PropertyQosPolicy& qos);

  static bool valid(const DDS::WriterDataLifecycleQosPolicy& qos);

  static bool valid(const DDS::ReaderDataLifecycleQosPolicy& qos);

  static bool valid(const DDS::DomainParticipantQos& qos);

  static bool valid(const DDS::TopicQos& qos);

  static bool valid(const DDS::DataWriterQos& qos);

  static bool valid(const DDS::PublisherQos& qos);

  static bool valid(const DDS::DataReaderQos& qos);

  static bool valid(const DDS::SubscriberQos& qos);

  static bool valid(const DDS::DomainParticipantFactoryQos& qos);

  static bool valid(const DDS::DataRepresentationQosPolicy& qos);

  static bool changeable(const DDS::UserDataQosPolicy& qos1,
                         const DDS::UserDataQosPolicy& qos2);

  static bool changeable(const DDS::TopicDataQosPolicy & qos1,
                         const DDS::TopicDataQosPolicy & qos2);

  static bool changeable(const DDS::GroupDataQosPolicy& qos1,
                         const DDS::GroupDataQosPolicy& qos2);

  static bool changeable(const DDS::TransportPriorityQosPolicy& qos1,
                         const DDS::TransportPriorityQosPolicy& qos2);

  static bool changeable(const DDS::LifespanQosPolicy& qos1,
                         const DDS::LifespanQosPolicy& qos2);

  static bool changeable(const DDS::DurabilityQosPolicy& qos1,
                         const DDS::DurabilityQosPolicy& qos2);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  static bool changeable(const DDS::DurabilityServiceQosPolicy& qos1,
                         const DDS::DurabilityServiceQosPolicy& qos2);
#endif

  static bool changeable(const DDS::PresentationQosPolicy& qos1,
                         const DDS::PresentationQosPolicy& qos2);

  static bool changeable(const DDS::DeadlineQosPolicy& qos1,
                         const DDS::DeadlineQosPolicy& qos2);

  static bool changeable(const DDS::LatencyBudgetQosPolicy& qos1,
                         const DDS::LatencyBudgetQosPolicy& qos2);

  static bool changeable(const DDS::OwnershipQosPolicy& qos1,
                         const DDS::OwnershipQosPolicy& qos2);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  static bool changeable(const DDS::OwnershipStrengthQosPolicy& qos1,
                         const DDS::OwnershipStrengthQosPolicy& qos2);
#endif

  static bool changeable(const DDS::LivelinessQosPolicy& qos1,
                         const DDS::LivelinessQosPolicy& qos2);

  static bool changeable(const DDS::TimeBasedFilterQosPolicy& qos1,
                         const DDS::TimeBasedFilterQosPolicy& qos2);

  static bool changeable(const DDS::PartitionQosPolicy& qos1,
                         const DDS::PartitionQosPolicy& qos2);

  static bool changeable(const DDS::ReliabilityQosPolicy& qos1,
                         const DDS::ReliabilityQosPolicy& qos2);

  static bool changeable(const DDS::DestinationOrderQosPolicy& qos1,
                         const DDS::DestinationOrderQosPolicy& qos2);

  static bool changeable(const DDS::HistoryQosPolicy& qos1,
                         const DDS::HistoryQosPolicy& qos2);

  static bool changeable(const DDS::ResourceLimitsQosPolicy& qos1,
                         const DDS::ResourceLimitsQosPolicy& qos2);

  static bool changeable(const DDS::EntityFactoryQosPolicy& qos1,
                         const DDS::EntityFactoryQosPolicy& qos2) ;

  static bool changeable(const DDS::WriterDataLifecycleQosPolicy& qos1,
                         const DDS::WriterDataLifecycleQosPolicy& qos2);

  static bool changeable(const DDS::ReaderDataLifecycleQosPolicy& qos1,
                         const DDS::ReaderDataLifecycleQosPolicy& qos2);

  static bool changeable(const DDS::DomainParticipantQos& qos1,
                         const DDS::DomainParticipantQos& qos2);

  static bool changeable(const DDS::TopicQos& qos1,
                         const DDS::TopicQos& qos2);

  static bool changeable(const DDS::DataWriterQos& qos1,
                         const DDS::DataWriterQos& qos2);

  static bool changeable(const DDS::PublisherQos& qos1,
                         const DDS::PublisherQos& qos2);

  static bool changeable(const DDS::DataReaderQos& qos1,
                         const DDS::DataReaderQos& qos2);

  static bool changeable(const DDS::SubscriberQos& qos1,
                         const DDS::SubscriberQos& qos2);

  static bool changeable(const DDS::DomainParticipantFactoryQos& qos1,
                         const DDS::DomainParticipantFactoryQos& qos2);

  static bool changeable(
    const DDS::DataRepresentationQosPolicy& qos1,
    const DDS::DataRepresentationQosPolicy& qos2);

  static bool copy_from_topic_qos(DDS::DataReaderQos& a_datareader_qos,
                                  const DDS::TopicQos& a_topic_qos);

  static bool copy_from_topic_qos(DDS::DataWriterQos& a_datareader_qos,
                                  const DDS::TopicQos& a_topic_qos);
};

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::UserDataQosPolicy& qos1,
                const DDS::UserDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::TopicDataQosPolicy& qos1,
                const DDS::TopicDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::GroupDataQosPolicy& qos1,
                const DDS::GroupDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::TransportPriorityQosPolicy& qos1,
                const DDS::TransportPriorityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::LifespanQosPolicy& qos1,
                const DDS::LifespanQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DurabilityQosPolicy& qos1,
                const DDS::DurabilityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DurabilityServiceQosPolicy& qos1,
                const DDS::DurabilityServiceQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::PresentationQosPolicy& qos1,
                const DDS::PresentationQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DeadlineQosPolicy& qos1,
                const DDS::DeadlineQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::LatencyBudgetQosPolicy& qos1,
                const DDS::LatencyBudgetQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::OwnershipQosPolicy& qos1,
                const DDS::OwnershipQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::OwnershipStrengthQosPolicy& qos1,
                const DDS::OwnershipStrengthQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::LivelinessQosPolicy& qos1,
                const DDS::LivelinessQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::TimeBasedFilterQosPolicy& qos1,
                const DDS::TimeBasedFilterQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::PartitionQosPolicy& qos1,
                const DDS::PartitionQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::ReliabilityQosPolicy& qos1,
                const DDS::ReliabilityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DestinationOrderQosPolicy& qos1,
                const DDS::DestinationOrderQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::HistoryQosPolicy& qos1,
                const DDS::HistoryQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::ResourceLimitsQosPolicy& qos1,
                const DDS::ResourceLimitsQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::EntityFactoryQosPolicy& qos1,
                const DDS::EntityFactoryQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::WriterDataLifecycleQosPolicy& qos1,
                const DDS::WriterDataLifecycleQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::ReaderDataLifecycleQosPolicy& qos1,
                const DDS::ReaderDataLifecycleQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DomainParticipantQos& qos1,
                const DDS::DomainParticipantQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::TopicQos& qos1,
                const DDS::TopicQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DataWriterQos& qos1,
                const DDS::DataWriterQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::PublisherQos& qos1,
                const DDS::PublisherQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DataReaderQos& qos1,
                const DDS::DataReaderQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::SubscriberQos& qos1,
                const DDS::SubscriberQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DomainParticipantFactoryQos& qos1,
                const DDS::DomainParticipantFactoryQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::DataRepresentationQosPolicy& qos1,
                const DDS::DataRepresentationQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::TypeConsistencyEnforcementQosPolicy& qos1,
                const DDS::TypeConsistencyEnforcementQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::UserDataQosPolicy& qos1,
                const DDS::UserDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::TopicDataQosPolicy& qos1,
                const DDS::TopicDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::GroupDataQosPolicy& qos1,
                const DDS::GroupDataQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::TransportPriorityQosPolicy& qos1,
                const DDS::TransportPriorityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::LifespanQosPolicy& qos1,
                const DDS::LifespanQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DurabilityQosPolicy& qos1,
                const DDS::DurabilityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DurabilityServiceQosPolicy& qos1,
                const DDS::DurabilityServiceQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::PresentationQosPolicy& qos1,
                const DDS::PresentationQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DeadlineQosPolicy& qos1,
                const DDS::DeadlineQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::LatencyBudgetQosPolicy& qos1,
                const DDS::LatencyBudgetQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::OwnershipQosPolicy& qos1,
                const DDS::OwnershipQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::OwnershipStrengthQosPolicy& qos1,
                const DDS::OwnershipStrengthQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::LivelinessQosPolicy& qos1,
                const DDS::LivelinessQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::TimeBasedFilterQosPolicy& qos1,
                const DDS::TimeBasedFilterQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::PartitionQosPolicy& qos1,
                const DDS::PartitionQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::ReliabilityQosPolicy& qos1,
                const DDS::ReliabilityQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DestinationOrderQosPolicy& qos1,
                const DDS::DestinationOrderQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::HistoryQosPolicy& qos1,
                const DDS::HistoryQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::ResourceLimitsQosPolicy& qos1,
                const DDS::ResourceLimitsQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::EntityFactoryQosPolicy& qos1,
                const DDS::EntityFactoryQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::WriterDataLifecycleQosPolicy& qos1,
                const DDS::WriterDataLifecycleQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::ReaderDataLifecycleQosPolicy& qos1,
                const DDS::ReaderDataLifecycleQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::PropertyQosPolicy& qos1,
                const DDS::PropertyQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DomainParticipantQos& qos1,
                const DDS::DomainParticipantQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::TopicQos& qos1,
                const DDS::TopicQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DataWriterQos& qos1,
                const DDS::DataWriterQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::PublisherQos& qos1,
                const DDS::PublisherQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DataReaderQos& qos1,
                const DDS::DataReaderQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::SubscriberQos& qos1,
                const DDS::SubscriberQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DomainParticipantFactoryQos& qos1,
                const DDS::DomainParticipantFactoryQos& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::DataRepresentationQosPolicy& qos1,
                const DDS::DataRepresentationQosPolicy& qos2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::TypeConsistencyEnforcementQosPolicy& qos1,
                const DDS::TypeConsistencyEnforcementQosPolicy& qos2);
#endif

class TransportPriorityQosPolicyBuilder {
public:
  TransportPriorityQosPolicyBuilder()
  {
    qos_.value = 0;
  }

  explicit TransportPriorityQosPolicyBuilder(const DDS::TransportPriorityQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::TransportPriorityQosPolicy& qos() const { return qos_; }
  DDS::TransportPriorityQosPolicy& qos() { return qos_; }
  operator const DDS::TransportPriorityQosPolicy&() const { return qos_; }
  operator DDS::TransportPriorityQosPolicy&() { return qos_; }

  TransportPriorityQosPolicyBuilder& value(int value)
  {
    qos_.value = value;
    return *this;
  }

private:
  DDS::TransportPriorityQosPolicy qos_;
};

class LifespanQosPolicyBuilder {
public:
  LifespanQosPolicyBuilder()
  {
    qos_.duration.sec = DDS::DURATION_INFINITE_SEC;
    qos_.duration.nanosec = DDS::DURATION_INFINITE_NSEC;
  }

  explicit LifespanQosPolicyBuilder(const DDS::LifespanQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::LifespanQosPolicy& qos() const { return qos_; }
  DDS::LifespanQosPolicy& qos() { return qos_; }
  operator const DDS::LifespanQosPolicy&() const { return qos_; }
  operator DDS::LifespanQosPolicy&() { return qos_; }

  LifespanQosPolicyBuilder& duration(const DDS::Duration_t& duration)
  {
    qos_.duration = duration;
    return *this;
  }

private:
  DDS::LifespanQosPolicy qos_;
};

class DurabilityQosPolicyBuilder {
public:
  DurabilityQosPolicyBuilder()
  {
    qos_.kind = DDS::VOLATILE_DURABILITY_QOS;
  }

  explicit DurabilityQosPolicyBuilder(const DDS::DurabilityQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::DurabilityQosPolicy& qos() const { return qos_; }
  DDS::DurabilityQosPolicy& qos() { return qos_; }
  operator const DDS::DurabilityQosPolicy&() const { return qos_; }
  operator DDS::DurabilityQosPolicy&() { return qos_; }

  DurabilityQosPolicyBuilder& kind(DDS::DurabilityQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  DurabilityQosPolicyBuilder& _volatile()
  {
    qos_.kind = DDS::VOLATILE_DURABILITY_QOS;
    return *this;
  }

  DurabilityQosPolicyBuilder& transient_local()
  {
    qos_.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    return *this;
  }

  DurabilityQosPolicyBuilder& transient()
  {
    qos_.kind = DDS::TRANSIENT_DURABILITY_QOS;
    return *this;
  }

  DurabilityQosPolicyBuilder& persistent()
  {
    qos_.kind = DDS::PERSISTENT_DURABILITY_QOS;
    return *this;
  }

private:
  DDS::DurabilityQosPolicy qos_;
};

class DurabilityServiceQosPolicyBuilder {
public:
  DurabilityServiceQosPolicyBuilder()
  {
    qos_.service_cleanup_delay.sec = DDS::DURATION_ZERO_SEC;
    qos_.service_cleanup_delay.nanosec = DDS::DURATION_ZERO_NSEC;
    qos_.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.history_depth = 1;
    qos_.max_samples = DDS::LENGTH_UNLIMITED;
    qos_.max_instances = DDS::LENGTH_UNLIMITED;
    qos_.max_samples_per_instance = DDS::LENGTH_UNLIMITED;
  }

  explicit DurabilityServiceQosPolicyBuilder(const DDS::DurabilityServiceQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::DurabilityServiceQosPolicy& qos() const { return qos_; }
  DDS::DurabilityServiceQosPolicy& qos() { return qos_; }
  operator const DDS::DurabilityServiceQosPolicy&() const { return qos_; }
  operator DDS::DurabilityServiceQosPolicy&() { return qos_; }

  DurabilityServiceQosPolicyBuilder& service_cleanup_delay(const DDS::Duration_t& delay)
  {
    qos_.service_cleanup_delay = delay;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& history_kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.history_kind = kind;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& keep_last(int depth)
  {
    qos_.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.history_depth = depth;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& keep_all()
  {
    qos_.history_kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& history_depth(int depth)
  {
    qos_.history_depth = depth;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& max_samples(int value)
  {
    qos_.max_samples = value;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& max_instances(int value)
  {
    qos_.max_instances = value;
    return *this;
  }

  DurabilityServiceQosPolicyBuilder& max_samples_per_instance(int value)
  {
    qos_.max_samples_per_instance = value;
    return *this;
  }

private:
  DDS::DurabilityServiceQosPolicy qos_;
};

class DeadlineQosPolicyBuilder {
public:
  DeadlineQosPolicyBuilder()
  {
    qos_.period.sec = DDS::DURATION_INFINITE_SEC;
    qos_.period.nanosec = DDS::DURATION_INFINITE_NSEC;
  }

  explicit DeadlineQosPolicyBuilder(const DDS::DeadlineQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::DeadlineQosPolicy& qos() const { return qos_; }
  DDS::DeadlineQosPolicy& qos() { return qos_; }
  operator const DDS::DeadlineQosPolicy&() const { return qos_; }
  operator DDS::DeadlineQosPolicy&() { return qos_; }

  DeadlineQosPolicyBuilder& period(const DDS::Duration_t& period)
  {
    qos_.period = period;
    return *this;
  }

private:
  DDS::DeadlineQosPolicy qos_;
};

class LatencyBudgetQosPolicyBuilder {
public:
  LatencyBudgetQosPolicyBuilder()
  {
    qos_.duration.sec = DDS::DURATION_ZERO_SEC;
    qos_.duration.nanosec = DDS::DURATION_ZERO_NSEC;
  }

  explicit LatencyBudgetQosPolicyBuilder(const DDS::LatencyBudgetQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::LatencyBudgetQosPolicy& qos() const { return qos_; }
  DDS::LatencyBudgetQosPolicy& qos() { return qos_; }
  operator const DDS::LatencyBudgetQosPolicy&() const { return qos_; }
  operator DDS::LatencyBudgetQosPolicy&() { return qos_; }

  LatencyBudgetQosPolicyBuilder& duration(const DDS::Duration_t& duration)
  {
    qos_.duration = duration;
    return *this;
  }

private:
  DDS::LatencyBudgetQosPolicy qos_;
};

class OwnershipQosPolicyBuilder {
public:
  OwnershipQosPolicyBuilder()
  {
    qos_.kind = DDS::SHARED_OWNERSHIP_QOS;
  }

  explicit OwnershipQosPolicyBuilder(const DDS::OwnershipQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::OwnershipQosPolicy& qos() const { return qos_; }
  DDS::OwnershipQosPolicy& qos() { return qos_; }
  operator const DDS::OwnershipQosPolicy&() const { return qos_; }
  operator DDS::OwnershipQosPolicy&() { return qos_; }

  OwnershipQosPolicyBuilder& kind(DDS::OwnershipQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  OwnershipQosPolicyBuilder& shared()
  {
    qos_.kind = DDS::SHARED_OWNERSHIP_QOS;
    return *this;
  }

  OwnershipQosPolicyBuilder& exclusive()
  {
    qos_.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
    return *this;
  }

private:
  DDS::OwnershipQosPolicy qos_;
};

class OwnershipStrengthQosPolicyBuilder {
public:
  OwnershipStrengthQosPolicyBuilder()
  {
    qos_.value = 0;
  }

  explicit OwnershipStrengthQosPolicyBuilder(const DDS::OwnershipStrengthQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::OwnershipStrengthQosPolicy& qos() const { return qos_; }
  DDS::OwnershipStrengthQosPolicy& qos() { return qos_; }
  operator const DDS::OwnershipStrengthQosPolicy&() const { return qos_; }
  operator DDS::OwnershipStrengthQosPolicy&() { return qos_; }

  OwnershipStrengthQosPolicyBuilder& value(int value)
  {
    qos_.value = value;
    return *this;
  }

private:
  DDS::OwnershipStrengthQosPolicy qos_;
};

class LivelinessQosPolicyBuilder {
public:
  LivelinessQosPolicyBuilder()
  {
    qos_.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    qos_.lease_duration.sec = DDS::DURATION_INFINITE_SEC;
    qos_.lease_duration.nanosec = DDS::DURATION_INFINITE_NSEC;
  }

  explicit LivelinessQosPolicyBuilder(const DDS::LivelinessQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::LivelinessQosPolicy& qos() const { return qos_; }
  DDS::LivelinessQosPolicy& qos() { return qos_; }
  operator const DDS::LivelinessQosPolicy&() const { return qos_; }
  operator DDS::LivelinessQosPolicy&() { return qos_; }

  LivelinessQosPolicyBuilder& kind(DDS::LivelinessQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  LivelinessQosPolicyBuilder& automatic()
  {
    qos_.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    return *this;
  }

  LivelinessQosPolicyBuilder& manual_by_participant()
  {
    qos_.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    return *this;
  }

  LivelinessQosPolicyBuilder& manual_by_topic()
  {
    qos_.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    return *this;
  }

  LivelinessQosPolicyBuilder& lease_duration(const DDS::Duration_t& duration)
  {
    qos_.lease_duration = duration;
    return *this;
  }

private:
  DDS::LivelinessQosPolicy qos_;
};

class ReliabilityQosPolicyBuilder {
public:
  ReliabilityQosPolicyBuilder()
  {
    qos_.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    // TODO: According to the spec, this should be:
    // qos_.max_blocking_time.sec = 0;
    // qos_.max_blocking_time.nanosec = 100000000;
    // Change this at the next major release.
    qos_.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;
    qos_.max_blocking_time.nanosec = DDS::DURATION_INFINITE_NSEC;
  }

  explicit ReliabilityQosPolicyBuilder(const DDS::ReliabilityQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::ReliabilityQosPolicy& qos() const { return qos_; }
  DDS::ReliabilityQosPolicy& qos() { return qos_; }
  operator const DDS::ReliabilityQosPolicy&() const { return qos_; }
  operator DDS::ReliabilityQosPolicy&() { return qos_; }

  ReliabilityQosPolicyBuilder& kind(DDS::ReliabilityQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  ReliabilityQosPolicyBuilder& best_effort()
  {
    qos_.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    return *this;
  }

  ReliabilityQosPolicyBuilder& reliable()
  {
    qos_.kind = DDS::RELIABLE_RELIABILITY_QOS;
    return *this;
  }

  ReliabilityQosPolicyBuilder& max_blocking_time(const DDS::Duration_t& duration)
  {
    qos_.max_blocking_time = duration;
    return *this;
  }

private:
  DDS::ReliabilityQosPolicy qos_;
};

class DestinationOrderQosPolicyBuilder {
public:
  DestinationOrderQosPolicyBuilder()
  {
    qos_.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
  }

  explicit DestinationOrderQosPolicyBuilder(const DDS::DestinationOrderQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::DestinationOrderQosPolicy& qos() const { return qos_; }
  DDS::DestinationOrderQosPolicy& qos() { return qos_; }
  operator const DDS::DestinationOrderQosPolicy&() const { return qos_; }
  operator DDS::DestinationOrderQosPolicy&() { return qos_; }

  DestinationOrderQosPolicyBuilder& kind(DDS::DestinationOrderQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  DestinationOrderQosPolicyBuilder& by_reception_timestamp()
  {
    qos_.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

  DestinationOrderQosPolicyBuilder& by_source_timestamp()
  {
    qos_.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

private:
  DDS::DestinationOrderQosPolicy qos_;
};

class HistoryQosPolicyBuilder {
public:
  HistoryQosPolicyBuilder()
  {
    qos_.kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.depth = 1;
  }

  explicit HistoryQosPolicyBuilder(const DDS::HistoryQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::HistoryQosPolicy& qos() const { return qos_; }
  DDS::HistoryQosPolicy& qos() { return qos_; }
  operator const DDS::HistoryQosPolicy&() const { return qos_; }
  operator DDS::HistoryQosPolicy&() { return qos_; }

  HistoryQosPolicyBuilder& kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.kind = kind;
    return *this;
  }

  HistoryQosPolicyBuilder& keep_last(int depth)
  {
    qos_.kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.depth = depth;
    return *this;
  }

  HistoryQosPolicyBuilder& keep_all()
  {
    qos_.kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  HistoryQosPolicyBuilder& depth(int depth)
  {
    qos_.depth = depth;
    return *this;
  }

private:
  DDS::HistoryQosPolicy qos_;
};

class ResourceLimitsQosPolicyBuilder {
public:
  ResourceLimitsQosPolicyBuilder()
  {
    qos_.max_samples = DDS::LENGTH_UNLIMITED;
    qos_.max_instances = DDS::LENGTH_UNLIMITED;
    qos_.max_samples_per_instance = DDS::LENGTH_UNLIMITED;
  }

  explicit ResourceLimitsQosPolicyBuilder(const DDS::ResourceLimitsQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::ResourceLimitsQosPolicy& qos() const { return qos_; }
  DDS::ResourceLimitsQosPolicy& qos() { return qos_; }
  operator const DDS::ResourceLimitsQosPolicy&() const { return qos_; }
  operator DDS::ResourceLimitsQosPolicy&() { return qos_; }

  ResourceLimitsQosPolicyBuilder& max_samples(int value)
  {
    qos_.max_samples = value;
    return *this;
  }

  ResourceLimitsQosPolicyBuilder& max_instances(int value)
  {
    qos_.max_instances = value;
    return *this;
  }

  ResourceLimitsQosPolicyBuilder& max_samples_per_instance(int value)
  {
    qos_.max_samples_per_instance = value;
    return *this;
  }

private:
  DDS::ResourceLimitsQosPolicy qos_;
};

class WriterDataLifecycleQosPolicyBuilder {
public:
  WriterDataLifecycleQosPolicyBuilder()
  {
    qos_.autodispose_unregistered_instances = true;
  }

  explicit WriterDataLifecycleQosPolicyBuilder(const DDS::WriterDataLifecycleQosPolicy& qos)
    : qos_(qos)
  {}

  const DDS::WriterDataLifecycleQosPolicy& qos() const { return qos_; }
  DDS::WriterDataLifecycleQosPolicy& qos() { return qos_; }
  operator const DDS::WriterDataLifecycleQosPolicy&() const { return qos_; }
  operator DDS::WriterDataLifecycleQosPolicy&() { return qos_; }

  WriterDataLifecycleQosPolicyBuilder& autodispose_unregistered_instances(bool value)
  {
    qos_.autodispose_unregistered_instances = value;
    return *this;
  }

private:
  DDS::WriterDataLifecycleQosPolicy qos_;
};

class TopicQosBuilder {
public:
  TopicQosBuilder()
  {
    // topic_data
    qos_.durability = DurabilityQosPolicyBuilder();
    qos_.durability_service = DurabilityServiceQosPolicyBuilder();
    qos_.deadline = DeadlineQosPolicyBuilder();
    qos_.latency_budget = LatencyBudgetQosPolicyBuilder();
    qos_.liveliness = LivelinessQosPolicyBuilder();
    qos_.reliability = ReliabilityQosPolicyBuilder();
    qos_.destination_order = DestinationOrderQosPolicyBuilder();
    qos_.history = HistoryQosPolicyBuilder();
    qos_.resource_limits = ResourceLimitsQosPolicyBuilder();
    qos_.transport_priority = TransportPriorityQosPolicyBuilder();
    qos_.lifespan = LifespanQosPolicyBuilder();
    qos_.ownership = OwnershipQosPolicyBuilder();
    // representation
  }

  explicit TopicQosBuilder(const DDS::TopicQos& qos)
    : qos_(qos)
  {}

  const DDS::TopicQos& qos() const { return qos_; }
  DDS::TopicQos& qos() { return qos_; }
  operator const DDS::TopicQos&() const { return qos_; }
  operator DDS::TopicQos&() { return qos_; }

  TopicQosBuilder& topic_data_value(const DDS::OctetSeq& value)
  {
    qos_.topic_data.value = value;
    return *this;
  }

  TopicQosBuilder& durability_kind(DDS::DurabilityQosPolicyKind kind)
  {
    qos_.durability.kind = kind;
    return *this;
  }

  TopicQosBuilder& durability_volatile()
  {
    qos_.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& durability_transient_local()
  {
    qos_.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& durability_transient()
  {
    qos_.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& durability_persistent()
  {
    qos_.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& durability_service_service_cleanup_delay(const DDS::Duration_t& delay)
  {
    qos_.durability_service.service_cleanup_delay = delay;
    return *this;
  }

  TopicQosBuilder& durability_service_history_kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.durability_service.history_kind = kind;
    return *this;
  }

  TopicQosBuilder& durability_service_keep_last(int depth)
  {
    qos_.durability_service.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.durability_service.history_depth = depth;
    return *this;
  }

  TopicQosBuilder& durability_service_keep_all()
  {
    qos_.durability_service.history_kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  TopicQosBuilder& durability_service_history_depth(int depth)
  {
    qos_.durability_service.history_depth = depth;
    return *this;
  }

  TopicQosBuilder& durability_service_max_samples(int value)
  {
    qos_.durability_service.max_samples = value;
    return *this;
  }

  TopicQosBuilder& durability_service_max_instances(int value)
  {
    qos_.durability_service.max_instances = value;
    return *this;
  }

  TopicQosBuilder& durability_service_max_samples_per_instance(int value)
  {
    qos_.durability_service.max_samples_per_instance = value;
    return *this;
  }

  TopicQosBuilder& deadline_period(const DDS::Duration_t& duration)
  {
    qos_.deadline.period = duration;
    return *this;
  }

  TopicQosBuilder& latency_budget_duration(const DDS::Duration_t& duration)
  {
    qos_.latency_budget.duration = duration;
    return *this;
  }

  TopicQosBuilder& liveliness_kind(DDS::LivelinessQosPolicyKind kind)
  {
    qos_.liveliness.kind = kind;
    return *this;
  }

  TopicQosBuilder& liveliness_automatic()
  {
    qos_.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    return *this;
  }

  TopicQosBuilder& liveliness_manual_by_participant()
  {
    qos_.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    return *this;
  }

  TopicQosBuilder& liveliness_manual_by_topic()
  {
    qos_.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    return *this;
  }

  TopicQosBuilder& liveliness_lease_duration(const DDS::Duration_t& duration)
  {
    qos_.liveliness.lease_duration = duration;
    return *this;
  }

  TopicQosBuilder& reliability_kind(DDS::ReliabilityQosPolicyKind kind)
  {
    qos_.reliability.kind = kind;
    return *this;
  }

  TopicQosBuilder& reliability_best_effort()
  {
    qos_.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& reliability_reliable()
  {
    qos_.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    return *this;
  }

  TopicQosBuilder& reliability_max_blocking_time(const DDS::Duration_t& duration)
  {
    qos_.reliability.max_blocking_time = duration;
    return *this;
  }

  TopicQosBuilder& destination_order_kind(DDS::DestinationOrderQosPolicyKind kind)
  {
    qos_.destination_order.kind = kind;
    return *this;
  }

  TopicQosBuilder& destination_order_by_reception_timestamp()
  {
    qos_.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

  TopicQosBuilder& destination_order_by_source_timestamp()
  {
    qos_.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

  TopicQosBuilder& history_kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.history.kind = kind;
    return *this;
  }

  TopicQosBuilder& history_keep_last(int depth)
  {
    qos_.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.history.depth = depth;
    return *this;
  }

  TopicQosBuilder& history_keep_all()
  {
    qos_.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  TopicQosBuilder& history_depth(int depth)
  {
    qos_.history.depth = depth;
    return *this;
  }

  TopicQosBuilder& resource_limits_max_samples(int value)
  {
    qos_.resource_limits.max_samples = value;
    return *this;
  }

  TopicQosBuilder& resource_limits_max_instances(int value)
  {
    qos_.resource_limits.max_instances = value;
    return *this;
  }

  TopicQosBuilder& resource_limits_max_samples_per_instance(int value)
  {
    qos_.resource_limits.max_samples_per_instance = value;
    return *this;
  }

  TopicQosBuilder& transport_priority_value(int value)
  {
    qos_.transport_priority.value = value;
    return *this;
  }

  TopicQosBuilder& lifespan_duration(const DDS::Duration_t& duration)
  {
    qos_.lifespan.duration = duration;
    return *this;
  }

  TopicQosBuilder& ownership_kind(DDS::OwnershipQosPolicyKind kind)
  {
    qos_.ownership.kind = kind;
    return *this;
  }

  TopicQosBuilder& ownership_shared()
  {
    qos_.ownership.kind = DDS::SHARED_OWNERSHIP_QOS;
    return *this;
  }

  TopicQosBuilder& ownership_exclusive()
  {
    qos_.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
    return *this;
  }

private:
  DDS::TopicQos qos_;
};

class OpenDDS_Dcps_Export DataWriterQosBuilder {
public:
  DataWriterQosBuilder()
  {
    qos_.durability = DurabilityQosPolicyBuilder();
    qos_.durability_service = DurabilityServiceQosPolicyBuilder();
    qos_.deadline = DeadlineQosPolicyBuilder();
    qos_.latency_budget = LatencyBudgetQosPolicyBuilder();
    qos_.liveliness = LivelinessQosPolicyBuilder();
    qos_.reliability = ReliabilityQosPolicyBuilder().reliable().max_blocking_time(make_duration(0, 100000000));
    qos_.destination_order = DestinationOrderQosPolicyBuilder();
    qos_.history = HistoryQosPolicyBuilder();
    qos_.resource_limits = ResourceLimitsQosPolicyBuilder();
    qos_.transport_priority = TransportPriorityQosPolicyBuilder();
    qos_.lifespan = LifespanQosPolicyBuilder();
    // userdata
    qos_.ownership = OwnershipQosPolicyBuilder();
    qos_.ownership_strength = OwnershipStrengthQosPolicyBuilder();
    qos_.writer_data_lifecycle = WriterDataLifecycleQosPolicyBuilder();
    // representation
  }

  explicit DataWriterQosBuilder(const DDS::DataWriterQos& qos)
    : qos_(qos)
  {}

  explicit DataWriterQosBuilder(DDS::Publisher_var publisher);

  DataWriterQosBuilder(DDS::Topic_var topic,
                       DDS::Publisher_var publisher);

  const DDS::DataWriterQos& qos() const { return qos_; }
  DDS::DataWriterQos& qos() { return qos_; }
  operator const DDS::DataWriterQos&() const { return qos_; }
  operator DDS::DataWriterQos&() { return qos_; }

  bool operator==(const DataWriterQosBuilder& other) const
  {
    return qos_ == other.qos_;
  }

  bool operator!=(const DataWriterQosBuilder& other) const
  {
    return !(*this == other);
  }

  DataWriterQosBuilder& durability_kind(DDS::DurabilityQosPolicyKind kind)
  {
    qos_.durability.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& durability_volatile()
  {
    qos_.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& durability_transient_local()
  {
    qos_.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& durability_transient()
  {
    qos_.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& durability_persistent()
  {
    qos_.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& durability_service_service_cleanup_delay(const DDS::Duration_t& delay)
  {
    qos_.durability_service.service_cleanup_delay = delay;
    return *this;
  }

  DataWriterQosBuilder& durability_service_history_kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.durability_service.history_kind = kind;
    return *this;
  }

  DataWriterQosBuilder& durability_service_keep_last(int depth)
  {
    qos_.durability_service.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.durability_service.history_depth = depth;
    return *this;
  }

  DataWriterQosBuilder& durability_service_keep_all()
  {
    qos_.durability_service.history_kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  DataWriterQosBuilder& durability_service_history_depth(int depth)
  {
    qos_.durability_service.history_depth = depth;
    return *this;
  }

  DataWriterQosBuilder& durability_service_max_samples(int value)
  {
    qos_.durability_service.max_samples = value;
    return *this;
  }

  DataWriterQosBuilder& durability_service_max_instances(int value)
  {
    qos_.durability_service.max_instances = value;
    return *this;
  }

  DataWriterQosBuilder& durability_service_max_samples_per_instance(int value)
  {
    qos_.durability_service.max_samples_per_instance = value;
    return *this;
  }

  DataWriterQosBuilder& deadline_period(const DDS::Duration_t& duration)
  {
    qos_.deadline.period = duration;
    return *this;
  }

  DataWriterQosBuilder& latency_budget_duration(const DDS::Duration_t& duration)
  {
    qos_.latency_budget.duration = duration;
    return *this;
  }

  DataWriterQosBuilder& liveliness_kind(DDS::LivelinessQosPolicyKind kind)
  {
    qos_.liveliness.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& liveliness_automatic()
  {
    qos_.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    return *this;
  }

  DataWriterQosBuilder& liveliness_manual_by_participant()
  {
    qos_.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    return *this;
  }

  DataWriterQosBuilder& liveliness_manual_by_topic()
  {
    qos_.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    return *this;
  }

  DataWriterQosBuilder& liveliness_lease_duration(const DDS::Duration_t& duration)
  {
    qos_.liveliness.lease_duration = duration;
    return *this;
  }

  DataWriterQosBuilder& reliability_kind(DDS::ReliabilityQosPolicyKind kind)
  {
    qos_.reliability.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& reliability_best_effort()
  {
    qos_.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& reliability_reliable()
  {
    qos_.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    return *this;
  }

  DataWriterQosBuilder& reliability_max_blocking_time(const DDS::Duration_t& duration)
  {
    qos_.reliability.max_blocking_time = duration;
    return *this;
  }

  DataWriterQosBuilder& destination_order_kind(DDS::DestinationOrderQosPolicyKind kind)
  {
    qos_.destination_order.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& destination_order_by_reception_timestamp()
  {
    qos_.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

  DataWriterQosBuilder& destination_order_by_source_timestamp()
  {
    qos_.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    return *this;
  }

  DataWriterQosBuilder& history_kind(DDS::HistoryQosPolicyKind kind)
  {
    qos_.history.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& history_keep_last(int depth)
  {
    qos_.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.history.depth = depth;
    return *this;
  }

  DataWriterQosBuilder& history_keep_all()
  {
    qos_.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    return *this;
  }

  DataWriterQosBuilder& history_depth(int depth)
  {
    qos_.history.depth = depth;
    return *this;
  }

  DataWriterQosBuilder& resource_limits_max_samples(int value)
  {
    qos_.resource_limits.max_samples = value;
    return *this;
  }

  DataWriterQosBuilder& resource_limits_max_instances(int value)
  {
    qos_.resource_limits.max_instances = value;
    return *this;
  }

  DataWriterQosBuilder& resource_limits_max_samples_per_instance(int value)
  {
    qos_.resource_limits.max_samples_per_instance = value;
    return *this;
  }

  DataWriterQosBuilder& transport_priority_value(int value)
  {
    qos_.transport_priority.value = value;
    return *this;
  }

  DataWriterQosBuilder& lifespan_duration(const DDS::Duration_t& duration)
  {
    qos_.lifespan.duration = duration;
    return *this;
  }

  DataWriterQosBuilder& user_data_value(const DDS::OctetSeq& value)
  {
    qos_.user_data.value = value;
    return *this;
  }

  DataWriterQosBuilder& ownership_kind(DDS::OwnershipQosPolicyKind kind)
  {
    qos_.ownership.kind = kind;
    return *this;
  }

  DataWriterQosBuilder& ownership_shared()
  {
    qos_.ownership.kind = DDS::SHARED_OWNERSHIP_QOS;
    return *this;
  }

  DataWriterQosBuilder& ownership_exclusive()
  {
    qos_.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
    return *this;
  }

  DataWriterQosBuilder& ownership_strength_value(int value)
  {
    qos_.ownership_strength.value = value;
    return *this;
  }

  DataWriterQosBuilder& writer_data_lifecycle_autodispose_unregistered_instances(bool value)
  {
    qos_.writer_data_lifecycle.autodispose_unregistered_instances = value;
    return *this;
  }

private:
  DDS::DataWriterQos qos_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "Qos_Helper.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_QOS_HELPER_H */
