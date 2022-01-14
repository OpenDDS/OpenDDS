/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_NS_string.h"
#include "ace/Truncate.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE
bool operator==(const DDS::UserDataQosPolicy& qos1,
                const DDS::UserDataQosPolicy& qos2)
{
  return qos1.value == qos2.value;
}

ACE_INLINE
bool operator==(const DDS::TopicDataQosPolicy& qos1,
                const DDS::TopicDataQosPolicy& qos2)
{
  return qos1.value == qos2.value;
}

ACE_INLINE
bool operator==(const DDS::GroupDataQosPolicy& qos1,
                const DDS::GroupDataQosPolicy& qos2)
{
  return qos1.value == qos2.value;
}

ACE_INLINE
bool operator==(const DDS::TransportPriorityQosPolicy& qos1,
                const DDS::TransportPriorityQosPolicy& qos2)
{
  return qos1.value == qos2.value;
}

ACE_INLINE
bool operator==(const DDS::LifespanQosPolicy& qos1,
                const DDS::LifespanQosPolicy& qos2)
{
  return qos1.duration == qos2.duration;
}

ACE_INLINE
bool
operator==(const DDS::DurabilityQosPolicy& qos1,
           const DDS::DurabilityQosPolicy& qos2)
{
  return qos1.kind == qos2.kind;
}

ACE_INLINE
bool
operator==(const DDS::DurabilityServiceQosPolicy& qos1,
           const DDS::DurabilityServiceQosPolicy& qos2)
{
  return
    qos1.service_cleanup_delay == qos2.service_cleanup_delay
    && qos1.history_kind == qos2.history_kind
    && qos1.history_depth == qos2.history_depth
    && qos1.max_samples == qos2.max_samples
    && qos1.max_instances == qos2.max_instances
    && qos1.max_samples_per_instance == qos2.max_samples_per_instance;
}

ACE_INLINE
bool operator==(const DDS::PresentationQosPolicy& qos1,
                const DDS::PresentationQosPolicy& qos2)
{
  return
    qos1.access_scope == qos2.access_scope
    && qos1.coherent_access == qos2.coherent_access
    && qos1.ordered_access == qos2.ordered_access;
}

ACE_INLINE
bool operator==(const DDS::DeadlineQosPolicy& qos1,
                const DDS::DeadlineQosPolicy& qos2)
{
  return qos1.period == qos2.period;
}

ACE_INLINE
bool operator==(const DDS::LatencyBudgetQosPolicy& qos1,
                const DDS::LatencyBudgetQosPolicy& qos2)
{
  return qos1.duration == qos2.duration;
}

ACE_INLINE
bool operator==(const DDS::OwnershipQosPolicy& qos1,
                const DDS::OwnershipQosPolicy& qos2)
{
  return qos1.kind == qos2.kind;
}

ACE_INLINE
bool operator==(const DDS::OwnershipStrengthQosPolicy& qos1,
                const DDS::OwnershipStrengthQosPolicy& qos2)
{
  return qos1.value == qos2.value;
}

ACE_INLINE
bool operator==(const DDS::LivelinessQosPolicy& qos1,
                const DDS::LivelinessQosPolicy& qos2)
{
  return
    qos1.kind == qos2.kind
    && qos1.lease_duration == qos2.lease_duration;
}

ACE_INLINE
bool operator==(const DDS::TimeBasedFilterQosPolicy& qos1,
                const DDS::TimeBasedFilterQosPolicy& qos2)
{
  return qos1.minimum_separation == qos2.minimum_separation;
}

ACE_INLINE
bool operator==(const DDS::PartitionQosPolicy& qos1,
                const DDS::PartitionQosPolicy& qos2)
{
  const CORBA::ULong len = qos1.name.length();

  if (len == qos2.name.length()) {
    for (CORBA::ULong i = 0; i < len; ++i) {
      if (0 != ACE_OS::strcmp(qos1.name[i], qos2.name[i])) {
        return false;
      }
    }

    return true;
  }

  return false;
}

ACE_INLINE
bool operator==(const DDS::ReliabilityQosPolicy& qos1,
                const DDS::ReliabilityQosPolicy& qos2)
{
  return
    qos1.kind == qos2.kind
    && qos1.max_blocking_time == qos2.max_blocking_time;
}

ACE_INLINE
bool operator==(const DDS::DestinationOrderQosPolicy& qos1,
                const DDS::DestinationOrderQosPolicy& qos2)
{
  return qos1.kind == qos2.kind;
}

ACE_INLINE
bool operator==(const DDS::HistoryQosPolicy& qos1,
                const DDS::HistoryQosPolicy& qos2)
{
  return
    qos1.kind == qos2.kind
    && qos1.depth == qos2.depth;
}

ACE_INLINE
bool operator==(const DDS::ResourceLimitsQosPolicy& qos1,
                const DDS::ResourceLimitsQosPolicy& qos2)
{
  return
    qos1.max_samples == qos2.max_samples
    && qos1.max_instances == qos2.max_instances
    && qos1.max_samples_per_instance == qos2.max_samples_per_instance;
}

ACE_INLINE
bool operator==(const DDS::EntityFactoryQosPolicy& qos1,
                const DDS::EntityFactoryQosPolicy& qos2)
{
  // Marked_Default_Qos for DomainParticipant uses a value that's not 0 or 1
  return std::memcmp(&qos1.autoenable_created_entities,
                     &qos2.autoenable_created_entities,
                     sizeof qos2.autoenable_created_entities) == 0;
}

ACE_INLINE
bool operator==(const DDS::WriterDataLifecycleQosPolicy& qos1,
                const DDS::WriterDataLifecycleQosPolicy& qos2)
{
  return
    qos1.autodispose_unregistered_instances == qos2.autodispose_unregistered_instances;
}

ACE_INLINE
bool operator==(const DDS::ReaderDataLifecycleQosPolicy& qos1,
                const DDS::ReaderDataLifecycleQosPolicy& qos2)
{
  return
    (qos1.autopurge_nowriter_samples_delay == qos2.autopurge_nowriter_samples_delay)
    && (qos1.autopurge_disposed_samples_delay == qos2.autopurge_disposed_samples_delay);
}

template <typename T>
ACE_INLINE
bool operator==(const TAO::unbounded_value_sequence<T>& seq1, const TAO::unbounded_value_sequence<T>& seq2)
{
  if (seq1.length() != seq2.length()) {
    return false;
  }

  for (unsigned int i = 0; i < seq1.length(); ++i) {
    if (seq1[i].name != seq2[i].name || seq1[i].value != seq2[i].value || seq1[i].propagate != seq2[i].propagate) {
      return false;
    }
  }
  return true;
}

ACE_INLINE
bool operator==(const DDS::PropertyQosPolicy& qos1,
                const DDS::PropertyQosPolicy& qos2)
{
  return qos1.value == qos2.value && qos1.binary_value == qos2.binary_value;
}

ACE_INLINE
bool operator==(const DDS::DomainParticipantQos& qos1,
                const DDS::DomainParticipantQos& qos2)
{
  return
    qos1.user_data == qos2.user_data
    && qos1.entity_factory == qos2.entity_factory
    && qos1.property == qos2.property;
}

ACE_INLINE
bool operator==(const DDS::TopicQos& qos1,
                const DDS::TopicQos& qos2)
{
  return
    qos1.topic_data == qos2.topic_data
    && qos1.durability == qos2.durability
    && qos1.durability_service == qos2.durability_service
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.transport_priority == qos2.transport_priority
    && qos1.lifespan == qos2.lifespan
    && qos1.ownership == qos2.ownership
    && qos1.representation == qos2.representation;
}

ACE_INLINE
bool operator==(const DDS::DataWriterQos& qos1,
                const DDS::DataWriterQos& qos2)
{
  return
    qos1.durability == qos2.durability
    && qos1.durability_service == qos2.durability_service
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.transport_priority == qos2.transport_priority
    && qos1.lifespan == qos2.lifespan
    && qos1.user_data == qos2.user_data
    && qos1.ownership == qos2.ownership
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    && qos1.ownership_strength == qos2.ownership_strength
#endif
    && qos1.writer_data_lifecycle == qos2.writer_data_lifecycle
    && qos1.representation == qos2.representation;
}

ACE_INLINE
bool operator==(const DDS::PublisherQos& qos1,
                const DDS::PublisherQos& qos2)
{
  return
    qos1.presentation == qos2.presentation
    && qos1.partition == qos2.partition
    && qos1.group_data == qos2.group_data
    && qos1.entity_factory == qos2.entity_factory;
}

ACE_INLINE
bool operator==(const DDS::DataReaderQos& qos1,
                const DDS::DataReaderQos& qos2)
{
  return
    qos1.durability == qos2.durability
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.user_data == qos2.user_data
    && qos1.time_based_filter == qos2.time_based_filter
    && qos1.reader_data_lifecycle == qos2.reader_data_lifecycle
    && qos1.representation == qos2.representation;
}

ACE_INLINE
bool operator==(const DDS::SubscriberQos& qos1,
                const DDS::SubscriberQos& qos2)
{
  return
    qos1.presentation == qos2.presentation
    && qos1.partition == qos2.partition
    && qos1.group_data == qos2.group_data
    && qos1.entity_factory == qos2.entity_factory;
}

ACE_INLINE
bool operator==(const DDS::DomainParticipantFactoryQos& qos1,
                const DDS::DomainParticipantFactoryQos& qos2)
{
  return qos1.entity_factory == qos2.entity_factory;
}

ACE_INLINE
bool operator==(const DDS::DataRepresentationQosPolicy& qos1,
                const DDS::DataRepresentationQosPolicy& qos2)
{
  const CORBA::ULong count = qos1.value.length();
  const CORBA::ULong count2 = qos2.value.length();
  if (count != count2) {
    return false;
  }
  for (CORBA::ULong i = 0; i < count; ++i) {
    if (qos1.value[i] != qos2.value[i]) {
      return false;
    }
  }
  return true;
}

ACE_INLINE
bool operator==(const DDS::TypeConsistencyEnforcementQosPolicy& qos1,
                const DDS::TypeConsistencyEnforcementQosPolicy& qos2)
{
  return qos1.kind == qos2.kind
    && qos1.ignore_sequence_bounds == qos2.ignore_sequence_bounds
    && qos1.ignore_string_bounds == qos2.ignore_string_bounds
    && qos1.ignore_member_names == qos2.ignore_member_names
    && qos1.prevent_type_widening == qos2.prevent_type_widening
    && qos1.force_type_validation == qos2.force_type_validation;
}

ACE_INLINE
bool operator!=(const DDS::UserDataQosPolicy& qos1,
                const DDS::UserDataQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::TopicDataQosPolicy& qos1,
                const DDS::TopicDataQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::GroupDataQosPolicy& qos1,
                const DDS::GroupDataQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::TransportPriorityQosPolicy& qos1,
                const DDS::TransportPriorityQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::LifespanQosPolicy& qos1,
                const DDS::LifespanQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool
operator!=(const DDS::DurabilityQosPolicy& qos1,
           const DDS::DurabilityQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool
operator!=(const DDS::DurabilityServiceQosPolicy& qos1,
           const DDS::DurabilityServiceQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::PresentationQosPolicy& qos1,
                const DDS::PresentationQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DeadlineQosPolicy& qos1,
                const DDS::DeadlineQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::LatencyBudgetQosPolicy& qos1,
                const DDS::LatencyBudgetQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::OwnershipQosPolicy& qos1,
                const DDS::OwnershipQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::OwnershipStrengthQosPolicy& qos1,
                const DDS::OwnershipStrengthQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::LivelinessQosPolicy& qos1,
                const DDS::LivelinessQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::TimeBasedFilterQosPolicy& qos1,
                const DDS::TimeBasedFilterQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::PartitionQosPolicy& qos1,
                const DDS::PartitionQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::ReliabilityQosPolicy& qos1,
                const DDS::ReliabilityQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DestinationOrderQosPolicy& qos1,
                const DDS::DestinationOrderQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::HistoryQosPolicy& qos1,
                const DDS::HistoryQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::ResourceLimitsQosPolicy& qos1,
                const DDS::ResourceLimitsQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::EntityFactoryQosPolicy& qos1,
                const DDS::EntityFactoryQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::WriterDataLifecycleQosPolicy& qos1,
                const DDS::WriterDataLifecycleQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::ReaderDataLifecycleQosPolicy& qos1,
                const DDS::ReaderDataLifecycleQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::PropertyQosPolicy& qos1,
                const DDS::PropertyQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DomainParticipantQos& qos1,
                const DDS::DomainParticipantQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::TopicQos& qos1,
                const DDS::TopicQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DataWriterQos& qos1,
                const DDS::DataWriterQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::PublisherQos& qos1,
                const DDS::PublisherQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DataReaderQos& qos1,
                const DDS::DataReaderQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::SubscriberQos& qos1,
                const DDS::SubscriberQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DomainParticipantFactoryQos& qos1,
                const DDS::DomainParticipantFactoryQos& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::DataRepresentationQosPolicy& qos1,
                const DDS::DataRepresentationQosPolicy& qos2)
{
  return !(qos1 == qos2);
}

ACE_INLINE
bool operator!=(const DDS::TypeConsistencyEnforcementQosPolicy& qos1,
                const DDS::TypeConsistencyEnforcementQosPolicy& qos2)
{
  return !(qos1 == qos2);
}
#endif

ACE_INLINE
bool
Qos_Helper::consistent(
  const DDS::ResourceLimitsQosPolicy& resource_limits,
  const DDS::HistoryQosPolicy& history)
{
  CORBA::Long const max_samples_per_instance =
    resource_limits.max_samples_per_instance;
  CORBA::Long const max_samples = resource_limits.max_samples;

  return
    (max_samples_per_instance == DDS::LENGTH_UNLIMITED
     || (max_samples_per_instance >= history.depth
         && (max_samples == DDS::LENGTH_UNLIMITED
             || max_samples >= max_samples_per_instance)));
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::DeadlineQosPolicy& deadline,
                       const DDS::TimeBasedFilterQosPolicy& time_based_filter)
{
  return time_based_filter.minimum_separation <= deadline.period;
}

ACE_INLINE
bool Qos_Helper::consistent(const DDS::DomainParticipantQos& /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::consistent(const DDS::TopicQos& qos)
{
  // Leverage existing validation functions for related QoS
  // policies.

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  DDS::HistoryQosPolicy const ds_history = {
    qos.durability_service.history_kind,
    qos.durability_service.history_depth
  };

  DDS::ResourceLimitsQosPolicy const ds_resource_limits = {
    qos.durability_service.max_samples,
    qos.durability_service.max_instances,
    qos.durability_service.max_samples_per_instance
  };

  return
    consistent(qos.resource_limits, qos.history)
    && consistent(ds_resource_limits, ds_history);
#else
  ACE_UNUSED_ARG(qos);
  return true;
#endif
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::DataWriterQos& qos)
{
  // Leverage existing validation functions for related QoS
  // policies.

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  DDS::HistoryQosPolicy const ds_history = {
    qos.durability_service.history_kind,
    qos.durability_service.history_depth
  };

  DDS::ResourceLimitsQosPolicy const ds_resource_limits = {
    qos.durability_service.max_samples,
    qos.durability_service.max_instances,
    qos.durability_service.max_samples_per_instance
  };

  return
    consistent(qos.resource_limits, qos.history)
    && consistent(ds_resource_limits, ds_history);
#else
  ACE_UNUSED_ARG(qos);
  return true;
#endif
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::PublisherQos& /* qos */)
{
  return true;
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::DataReaderQos& qos)
{
  return
    consistent(qos.deadline, qos.time_based_filter) &&
    consistent(qos.resource_limits, qos.history);
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::SubscriberQos& /* qos */)
{
  return true;
}

ACE_INLINE
bool
Qos_Helper::consistent(const DDS::DomainParticipantFactoryQos& /* qos */)
{
  return true;
}

// Note: Since in the first implmenation of DSS in TAO
//       a limited number of QoS values are allowed to be
//       modified, the validity tests are simplified to mostly
//       being a check that they are the defaults defined in
//       the specification.
//       SO INVALID ALSO INCLUDES UNSUPPORTED QoS.
// TBD - when QoS become support the valid checks should check
//       the ranges of the values of the QoS.

// The spec does not have specification about the content of
// UserDataQosPolicy,TopicDataQosPolicy and GroupDataQosPolicy
// so they are valid with any value.
ACE_INLINE
bool Qos_Helper::valid(const DDS::UserDataQosPolicy& /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::TopicDataQosPolicy & /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::GroupDataQosPolicy& /* qos */)
{
  return true;
}

// All values of TRANSPORT_PRIORITY.value are accepted.
ACE_INLINE
bool Qos_Helper::valid(const DDS::TransportPriorityQosPolicy& /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::LifespanQosPolicy& qos)
{
  return valid_duration(qos.duration);
}

ACE_INLINE
bool
Qos_Helper::valid(const DDS::DurabilityQosPolicy& qos)
{
  return
    (qos.kind == DDS::VOLATILE_DURABILITY_QOS
     || qos.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS
     || qos.kind == DDS::TRANSIENT_DURABILITY_QOS
     || qos.kind == DDS::PERSISTENT_DURABILITY_QOS);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::PresentationQosPolicy& qos)
{
  return
    (qos.access_scope == DDS::INSTANCE_PRESENTATION_QOS
     || qos.access_scope == DDS::TOPIC_PRESENTATION_QOS
     || qos.access_scope == DDS::GROUP_PRESENTATION_QOS);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DeadlineQosPolicy& qos)
{
  return valid_duration(qos.period);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::LatencyBudgetQosPolicy& /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::OwnershipQosPolicy& qos)
{
  return
    qos.kind == DDS::SHARED_OWNERSHIP_QOS
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    || qos.kind == DDS::EXCLUSIVE_OWNERSHIP_QOS
#endif
    ;
}

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
ACE_INLINE
bool Qos_Helper::valid(const DDS::OwnershipStrengthQosPolicy& /*qos*/)
{
  return true;
}
#endif

ACE_INLINE
bool Qos_Helper::valid(const DDS::LivelinessQosPolicy& qos)
{
  return
    (qos.kind == DDS::AUTOMATIC_LIVELINESS_QOS
     || qos.kind == DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS
     || qos.kind == DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS)

    && valid_duration(qos.lease_duration);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::TimeBasedFilterQosPolicy& /*qos*/)
{
  return true;
}

ACE_INLINE
bool
Qos_Helper::valid(const DDS::PartitionQosPolicy& /* qos */)
{
  // All strings are valid, although we may not accept all
  // wildcard patterns.  For now we treat unsupported wildcard
  // patterns literally instead of as wildcards.
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::ReliabilityQosPolicy& qos)
{
  return
    qos.kind == DDS::BEST_EFFORT_RELIABILITY_QOS
    || qos.kind == DDS::RELIABLE_RELIABILITY_QOS;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DestinationOrderQosPolicy& qos)
{
  return qos.kind == DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS
         || qos.kind == DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
}

ACE_INLINE
bool
Qos_Helper::valid(const DDS::HistoryQosPolicy& qos)
{
  return
    (qos.kind == DDS::KEEP_LAST_HISTORY_QOS
     || qos.kind == DDS::KEEP_ALL_HISTORY_QOS)
    && (qos.depth == DDS::LENGTH_UNLIMITED
        || qos.depth > 0);
}

ACE_INLINE
bool
Qos_Helper::valid(const DDS::ResourceLimitsQosPolicy& qos)
{
  return
    (qos.max_samples == DDS::LENGTH_UNLIMITED
     || qos.max_samples > 0)
    && (qos.max_instances == DDS::LENGTH_UNLIMITED
        || qos.max_instances > 0)
    && (qos.max_samples_per_instance == DDS::LENGTH_UNLIMITED
        || qos.max_samples_per_instance > 0);
}

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
ACE_INLINE
bool
Qos_Helper::valid(const DDS::DurabilityServiceQosPolicy& qos)
{
  // Leverage existing validation functions for related QoS
  // policies.
  DDS::HistoryQosPolicy const history = {
    qos.history_kind,
    qos.history_depth
  };

  DDS::ResourceLimitsQosPolicy const resource_limits = {
    qos.max_samples,
    qos.max_instances,
    qos.max_samples_per_instance
  };

  return
    non_negative_duration(qos.service_cleanup_delay)
    && valid(history)
    && valid(resource_limits);
}
#endif

ACE_INLINE
bool Qos_Helper::valid(const DDS::EntityFactoryQosPolicy& qos)
{
  // see Marked_Default_Qos::marked_default_DomainParticipantQos()
  const void* const mem = &qos.autoenable_created_entities;
  return *static_cast<const char*>(mem) != 3;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::PropertyQosPolicy& /* qos */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::WriterDataLifecycleQosPolicy&)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::ReaderDataLifecycleQosPolicy&)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DomainParticipantQos& qos)
{
  return valid(qos.user_data) && valid(qos.entity_factory) && valid(qos.property);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::TopicQos& qos)
{
  return
    valid(qos.topic_data)
    && valid(qos.durability)
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    && valid(qos.durability_service)
#endif
    && valid(qos.deadline)
    && valid(qos.latency_budget)
    && valid(qos.liveliness)
    && valid(qos.destination_order)
    && valid(qos.history)
    && valid(qos.resource_limits)
    && valid(qos.transport_priority)
    && valid(qos.lifespan)
    && valid(qos.ownership)
    && valid(qos.representation);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DataWriterQos& qos)
{
  if (!valid(qos.durability))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid durability qos.\n")), false);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  if (!valid(qos.durability_service))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid durability_service qos.\n")), false);
#endif
  if (!valid(qos.deadline))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid deadline qos.\n")), false);

  if (!valid(qos.latency_budget))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid latency_budget qos.\n")), false);

  if (!valid(qos.liveliness))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid liveliness qos.\n")), false);

  if (!valid(qos.destination_order))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid destination_order qos.\n")), false);

  if (!valid(qos.history))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid history qos.\n")), false);

  if (!valid(qos.resource_limits))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid resource_limits qos.\n")), false);

  if (!valid(qos.transport_priority))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid transport_priority qos.\n")), false);

  if (!valid(qos.lifespan))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid lifespan qos.\n")), false);

  if (!valid(qos.user_data))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid user_data qos.\n")), false);

  if (!valid(qos.ownership))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid ownership qos.\n")), false);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (!valid(qos.ownership_strength))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid ownership_strength qos.\n")), false);
#endif
  if (!valid(qos.writer_data_lifecycle))
    ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
        ACE_TEXT("invalid writer_data_lifecycle qos.\n")), false);

  if (!valid(qos.representation)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Qos_Helper::valid::DataWriterQos, ")
      ACE_TEXT("invalid data representation qos.\n")));
    return false;
  }

  return true;
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::PublisherQos& qos)
{
  return
    valid(qos.presentation)
    && valid(qos.partition)
    && valid(qos.group_data)
    && valid(qos.entity_factory);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DataReaderQos& qos)
{
  return
    valid(qos.durability)
    && valid(qos.deadline)
    && valid(qos.latency_budget)
    && valid(qos.liveliness)
    && valid(qos.reliability)
    && valid(qos.destination_order)
    && valid(qos.history)
    && valid(qos.resource_limits)
    && valid(qos.user_data)
    && valid(qos.time_based_filter)
    && valid(qos.reader_data_lifecycle)
    && valid(qos.ownership)
    && valid(qos.representation);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::SubscriberQos& qos)
{
  return
    valid(qos.presentation)
    && valid(qos.partition)
    && valid(qos.group_data)
    && valid(qos.entity_factory);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DomainParticipantFactoryQos& qos)
{
  return valid(qos.entity_factory);
}

ACE_INLINE
bool Qos_Helper::valid(const DDS::DataRepresentationQosPolicy& qos)
{
  const CORBA::ULong count = qos.value.length();
  for (CORBA::ULong i = 0; i < count; ++i) {
    const CORBA::Short value = qos.value[i];
    switch (value) {
    case DDS::XCDR_DATA_REPRESENTATION:
    case DDS::XML_DATA_REPRESENTATION:
    case DDS::XCDR2_DATA_REPRESENTATION:
    case OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION:
      break;

    default:
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Qos_Helper::valid(const DataRepresentationQosPolicy&): ")
        ACE_TEXT("Unknown DataRepresentationId_t: %d\n"), value));
      return false;
    };
  }
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::UserDataQosPolicy& /* qos1 */,
                            const DDS::UserDataQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::TopicDataQosPolicy & /* qos1 */,
                            const DDS::TopicDataQosPolicy & /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::GroupDataQosPolicy& /* qos1 */,
                            const DDS::GroupDataQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::TransportPriorityQosPolicy& qos1,
  const DDS::TransportPriorityQosPolicy& qos2)
{
  // formal/07-01-01 specifies that this is changeable.  OpenDDS as
  // of Version 1.3 does not support dynamic modification of the
  // priority of a single DataWriter TRANSPORT_PRIORITY.value.
  //
  // return true;
  return qos1 == qos2;
}

ACE_INLINE
bool
Qos_Helper::changeable(const DDS::LifespanQosPolicy& /* qos1 */,
                       const DDS::LifespanQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DurabilityQosPolicy& qos1,
                            const DDS::DurabilityQosPolicy& qos2)
{
  return qos1 == qos2;
}

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
ACE_INLINE
bool
Qos_Helper::changeable(const DDS::DurabilityServiceQosPolicy& qos1,
                       const DDS::DurabilityServiceQosPolicy& qos2)
{
  return qos1 == qos2;
}
#endif

ACE_INLINE
bool Qos_Helper::changeable(const DDS::PresentationQosPolicy& qos1,
                            const DDS::PresentationQosPolicy& qos2)
{
  return qos1 == qos2;
}

// ---------------------------------------------------------------
/**
 * TBD: These QoS are not supported currently, they are
 *      changeable, but need a compatibility check between the
 *      publisher and subscriber ends when changing the QoS.
 */
// ---------------------------------------------------------------
ACE_INLINE
bool Qos_Helper::changeable(const DDS::DeadlineQosPolicy& /* qos1 */,
                            const DDS::DeadlineQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::LatencyBudgetQosPolicy& /* qos1 */,
  const DDS::LatencyBudgetQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::OwnershipQosPolicy& qos1,
                            const DDS::OwnershipQosPolicy& qos2)
{
  return qos1 == qos2;
}
// ---------------------------------------------------------------

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::OwnershipStrengthQosPolicy& /* qos1 */,
  const DDS::OwnershipStrengthQosPolicy& /* qos2 */)
{
  return true;
}
#endif

ACE_INLINE
bool Qos_Helper::changeable(const DDS::LivelinessQosPolicy& qos1,
                            const DDS::LivelinessQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::TimeBasedFilterQosPolicy& /* qos1 */,
  const DDS::TimeBasedFilterQosPolicy& /* qos2*/)
{
  return true;
}

ACE_INLINE
bool
Qos_Helper::changeable(const DDS::PartitionQosPolicy& /* qos1 */,
                       const DDS::PartitionQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::ReliabilityQosPolicy& qos1,
                            const DDS::ReliabilityQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DestinationOrderQosPolicy& qos1,
                            const DDS::DestinationOrderQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::HistoryQosPolicy& qos1,
                            const DDS::HistoryQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::ResourceLimitsQosPolicy& qos1,
                            const DDS::ResourceLimitsQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::EntityFactoryQosPolicy& /* qos1 */,
  const DDS::EntityFactoryQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::WriterDataLifecycleQosPolicy& /* qos1 */,
  const DDS::WriterDataLifecycleQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::ReaderDataLifecycleQosPolicy& /* qos1 */,
  const DDS::ReaderDataLifecycleQosPolicy& /* qos2 */)
{
  return true;
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DomainParticipantQos& qos1,
                            const DDS::DomainParticipantQos& qos2)
{
  return
    changeable(qos1.user_data, qos2.user_data)
    && changeable(qos1.entity_factory, qos2.entity_factory);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::TopicQos& qos1,
                            const DDS::TopicQos& qos2)
{
  return
    changeable(qos1.topic_data, qos2.topic_data)
    && changeable(qos1.durability, qos2.durability)
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    && changeable(qos1.durability_service, qos2.durability_service)
#endif
    && changeable(qos1.deadline, qos2.deadline)
    && changeable(qos1.latency_budget, qos2.latency_budget)
    && changeable(qos1.liveliness, qos2.liveliness)
    && changeable(qos1.reliability, qos2.reliability)
    && changeable(qos1.destination_order, qos2.destination_order)
    && changeable(qos1.history, qos2.history)
    && changeable(qos1.resource_limits, qos2.resource_limits)
    && changeable(qos1.transport_priority, qos2.transport_priority)
    && changeable(qos1.lifespan, qos2.lifespan)
    && changeable(qos1.ownership, qos2.ownership)
    && changeable(qos1.representation, qos2.representation);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DataWriterQos& qos1,
                            const DDS::DataWriterQos& qos2)
{
  return
    changeable(qos1.durability, qos2.durability)
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    && changeable(qos1.durability_service, qos2.durability_service)
#endif
    && changeable(qos1.deadline, qos2.deadline)
    && changeable(qos1.latency_budget, qos2.latency_budget)
    && changeable(qos1.liveliness, qos2.liveliness)
    && changeable(qos1.reliability, qos2.reliability)
    && changeable(qos1.destination_order, qos2.destination_order)
    && changeable(qos1.history, qos2.history)
    && changeable(qos1.resource_limits, qos2.resource_limits)
    && changeable(qos1.transport_priority, qos2.transport_priority)
    && changeable(qos1.lifespan, qos2.lifespan)
    && changeable(qos1.user_data, qos2.user_data)
    && changeable(qos1.ownership, qos2.ownership)
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    && changeable(qos1.ownership_strength, qos2.ownership_strength)
#endif
    && changeable(qos1.writer_data_lifecycle, qos2.writer_data_lifecycle)
    && changeable(qos1.representation, qos2.representation);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::PublisherQos& qos1,
                            const DDS::PublisherQos& qos2)
{
  return
    changeable(qos1.presentation, qos2.presentation)
    && changeable(qos1.partition, qos2.partition)
    && changeable(qos1.group_data, qos2.group_data)
    && changeable(qos1.entity_factory, qos2.entity_factory);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DataReaderQos& qos1,
                            const DDS::DataReaderQos& qos2)
{
  return
    changeable(qos1.durability, qos2.durability)
    && changeable(qos1.deadline, qos2.deadline)
    && changeable(qos1.latency_budget, qos2.latency_budget)
    && changeable(qos1.liveliness, qos2.liveliness)
    && changeable(qos1.reliability, qos2.reliability)
    && changeable(qos1.destination_order, qos2.destination_order)
    && changeable(qos1.history, qos2.history)
    && changeable(qos1.resource_limits, qos2.resource_limits)
    && changeable(qos1.user_data, qos2.user_data)
    && changeable(qos1.time_based_filter, qos2.time_based_filter)
    && changeable(qos1.reader_data_lifecycle, qos2.reader_data_lifecycle)
    && changeable(qos1.ownership, qos2.ownership)
    && changeable(qos1.representation, qos2.representation);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::SubscriberQos& qos1,
                            const DDS::SubscriberQos& qos2)
{
  return
    changeable(qos1.presentation, qos2.presentation)
    && changeable(qos1.partition, qos2.partition)
    && changeable(qos1.group_data, qos2.group_data)
    && changeable(qos1.entity_factory, qos2.entity_factory);
}

ACE_INLINE
bool Qos_Helper::changeable(const DDS::DomainParticipantFactoryQos& qos1,
                            const DDS::DomainParticipantFactoryQos& qos2)
{
  return changeable(qos1.entity_factory, qos2.entity_factory);
}

ACE_INLINE
bool Qos_Helper::changeable(
  const DDS::DataRepresentationQosPolicy& qos1,
  const DDS::DataRepresentationQosPolicy& qos2)
{
  return qos1 == qos2;
}

ACE_INLINE
bool Qos_Helper::copy_from_topic_qos(DDS::DataReaderQos& a_datareader_qos,
                                     const DDS::TopicQos& a_topic_qos)
{
  if (!Qos_Helper::valid(a_topic_qos) || !Qos_Helper::consistent(a_topic_qos)) {
    return false;
  }
    // the caller can get the default before calling this
    // method if it wants to.
  a_datareader_qos.durability = a_topic_qos.durability;
  a_datareader_qos.deadline = a_topic_qos.deadline;
  a_datareader_qos.latency_budget = a_topic_qos.latency_budget;
  a_datareader_qos.liveliness = a_topic_qos.liveliness;
  a_datareader_qos.reliability = a_topic_qos.reliability;
  a_datareader_qos.destination_order = a_topic_qos.destination_order;
  a_datareader_qos.history = a_topic_qos.history;
  a_datareader_qos.resource_limits = a_topic_qos.resource_limits;
  a_datareader_qos.ownership = a_topic_qos.ownership;
  a_datareader_qos.representation = a_topic_qos.representation;
  return true;
}

ACE_INLINE
bool Qos_Helper::copy_from_topic_qos(DDS::DataWriterQos& a_datawriter_qos,
                                     const DDS::TopicQos& a_topic_qos)
{
  if (!Qos_Helper::valid(a_topic_qos) || !Qos_Helper::consistent(a_topic_qos)) {
    return false;
  }
  // Some members in the DataWriterQos are not contained
  // in the TopicQos. The caller needs initialize them.
  a_datawriter_qos.durability = a_topic_qos.durability;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  a_datawriter_qos.durability_service = a_topic_qos.durability_service;
#endif
  a_datawriter_qos.deadline = a_topic_qos.deadline;
  a_datawriter_qos.latency_budget = a_topic_qos.latency_budget;
  a_datawriter_qos.liveliness = a_topic_qos.liveliness;
  a_datawriter_qos.reliability = a_topic_qos.reliability;
  a_datawriter_qos.destination_order = a_topic_qos.destination_order;
  a_datawriter_qos.history = a_topic_qos.history;
  a_datawriter_qos.resource_limits = a_topic_qos.resource_limits;
  a_datawriter_qos.transport_priority = a_topic_qos.transport_priority;
  a_datawriter_qos.lifespan = a_topic_qos.lifespan;
  a_datawriter_qos.ownership = a_topic_qos.ownership;
  a_datawriter_qos.representation = a_topic_qos.representation;
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
