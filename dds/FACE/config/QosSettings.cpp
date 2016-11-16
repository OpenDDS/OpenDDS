#include "QosSettings.h"
#include "dds/DCPS/Service_Participant.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

QosSettings::QosSettings() :
  publisher_qos_(TheServiceParticipant->initial_PublisherQos())
, subscriber_qos_(TheServiceParticipant->initial_SubscriberQos())
, datawriter_qos_(TheServiceParticipant->initial_DataWriterQos())
, datareader_qos_(TheServiceParticipant->initial_DataReaderQos())
{

}

void
QosSettings::apply_to(DDS::PublisherQos&  target) const
{
  target = publisher_qos_;
}

void
QosSettings::apply_to(DDS::SubscriberQos&  target) const
{
  target = subscriber_qos_;
}

void
QosSettings::apply_to(DDS::DataWriterQos&  target) const
{
  target = datawriter_qos_;
}

void
QosSettings::apply_to(DDS::DataReaderQos&  target) const
{
  target = datareader_qos_;
}

int
QosSettings::set_qos(QosLevel level, const char* name, const char* value)
{
  int status = 0;
  switch (level) {
    case publisher:
      status = set_qos(publisher_qos_, name, value);
      break;
    case subscriber:
      status = set_qos(subscriber_qos_, name, value);
      break;
    case datawriter:
      status = set_qos(datawriter_qos_, name, value);
      break;
    case datareader:
      status = set_qos(datareader_qos_, name, value);
      break;
  }
  return status;
}

bool
set_presentation_access_scope_qos(
  DDS::PresentationQosPolicy& target,
  const char* name,
  const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "presentation.access_scope")) {
    if (!std::strcmp(value, "INSTANCE")) {
      target.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
      matched = true;
    }
    if (!std::strcmp(value, "TOPIC")) {
      target.access_scope = DDS::TOPIC_PRESENTATION_QOS;
      matched = true;
    }
    if (!std::strcmp(value, "GROUP")) {
      target.access_scope = DDS::GROUP_PRESENTATION_QOS;
      matched = true;
    }
  }
  return matched;
}

// Set boolean value on the target and return true if it was a valid value
bool
set_bool_qos_value(bool& target, const char* value)
{
  bool matched = false;
  if (!std::strcmp(value, "true")) {
    target = true;
    matched = true;
  } else if (!std::strcmp(value, "false")) {
    target = false;
    matched = true;
  }
  return matched;
}

// Set duration on the target and return true if it was a valid value
bool
set_duration_qos_value(DDS::Duration_t& target,
                       const char* prefix_match, // prefix to match
                       const char* name,         // config name provided
                       const char* value)        // config value provided
{
  char buffer[64];
  std::strncpy(buffer, prefix_match, 64 - 4);
  std::strcat(buffer, ".sec");
  if (!std::strcmp(name, buffer)) {
    if (!std::strcmp(value, "DURATION_INFINITE_SEC")) {
      target.sec=DDS::DURATION_INFINITE_SEC;
      return true;
    }
    target.sec = atoi(value);
    return true;
  }
  std::strncpy(buffer, prefix_match, 64 - 7);
  std::strcat(buffer, ".nanosec");
  if (!std::strcmp(name, buffer)) {
    if (!std::strcmp(value, "DURATION_INFINITE_NSEC")) {
      target.nanosec=DDS::DURATION_INFINITE_NSEC;
      return true;
    }
    target.nanosec = atoi(value);
    return true;
  }
  return false;
}

bool
set_presentation_coherent_access_qos(
  DDS::PresentationQosPolicy& target,
  const char* name,
  const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "presentation.coherent_access")) {
    matched = set_bool_qos_value(target.coherent_access, value);
  }
  return matched;
}

bool
set_presentation_ordered_access_qos(
  DDS::PresentationQosPolicy& target,
  const char* name,
  const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "presentation.ordered_access")) {
    matched = set_bool_qos_value(target.ordered_access, value);
  }
  return matched;
}

bool
set_partition_name_qos(
  DDS::PartitionQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "partition.name")) {
    // Value can be a comma-separated list
    const char* start = value;
    char buffer[128];
    std::memset(buffer, 0, sizeof(buffer));
    while (const char* next_comma = std::strchr(start, ',')) {
      // Copy into temp buffer, won't have null
      std::strncpy(buffer, start, next_comma - start);
      // Append null
      buffer[next_comma - start] = '\0';
      // Add to QOS
      target.name.length(target.name.length() + 1);
      target.name[target.name.length() - 1] = static_cast<const char*>(buffer);
      // Advance pointer
      start = next_comma + 1;
    }
    // Append everything after last comma
    target.name.length(target.name.length() + 1);
    target.name[target.name.length() - 1] = start;

    matched = true;
  }
  return matched;
}

bool
set_durability_kind_qos(
  DDS::DurabilityQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "durability.kind")) {
    if (!std::strcmp(value, "VOLATILE")) {
      target.kind = DDS::VOLATILE_DURABILITY_QOS;
      matched = true;
    } else if (!std::strcmp(value, "TRANSIENT_LOCAL")) {
      target.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      matched = true;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    } else if (!std::strcmp(value, "TRANSIENT")) {
      target.kind = DDS::TRANSIENT_DURABILITY_QOS;
      matched = true;
    } else if (!std::strcmp(value, "PERSISTENT")) {
      target.kind = DDS::PERSISTENT_DURABILITY_QOS;
      matched = true;
#endif
    }
  }
  return matched;
}

bool
set_deadline_period_qos(
  DDS::DeadlineQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(target.period, "deadline.period", name, value);
}

bool
set_latency_budget_duration_qos(
  DDS::LatencyBudgetQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.duration, "latency_budget.duration", name, value);
}

bool set_liveliness_kind_qos(
   DDS::LivelinessQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "liveliness.kind")) {
    if (!std::strcmp(value, "AUTOMATIC")) {
      target.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
      matched = true;
    } else if (!std::strcmp(value, "MANUAL_BY_TOPIC")) {
      target.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      matched = true;
    } else if (!std::strcmp(value, "MANUAL_BY_PARTICIPANT")) {
      target.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      matched = true;
    }
  }
  return matched;
}

bool set_liveliness_lease_duration_qos(
   DDS::LivelinessQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.lease_duration, "liveliness.lease_duration", name, value);
}

bool set_reliability_kind_qos(
  DDS::ReliabilityQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "reliability.kind")) {
    if (!std::strcmp(value, "BEST_EFFORT")) {
      target.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
      matched = true;
    } else if (!std::strcmp(value, "RELIABLE")) {
      target.kind = DDS::RELIABLE_RELIABILITY_QOS;
      matched = true;
    }
  }
  return matched;
}

bool set_reliability_max_blocking_time_qos(
  DDS::ReliabilityQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.max_blocking_time, "reliability.max_blocking_time", name, value);
}

bool set_destination_order_kind_qos(
  DDS::DestinationOrderQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "destination_order.kind")) {
    if (!std::strcmp(value, "BY_RECEPTION_TIMESTAMP")) {
      target.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      matched = true;
    } else if (!std::strcmp(value, "BY_SOURCE_TIMESTAMP")) {
      target.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
      matched = true;
    }
  }
  return matched;
}

bool set_history_kind_qos(
  DDS::HistoryQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "history.kind")) {
    if (!std::strcmp(value, "KEEP_ALL")) {
      target.kind = DDS::KEEP_ALL_HISTORY_QOS;
      matched = true;
    } else if (!std::strcmp(value, "KEEP_LAST")) {
      target.kind = DDS::KEEP_LAST_HISTORY_QOS;
      matched = true;
    }
  }
  return matched;
}

bool set_history_depth_qos(
  DDS::HistoryQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "history.depth")) {
    target.depth = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_resource_limits_max_samples_qos(
  DDS::ResourceLimitsQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "resource_limits.max_samples")) {
    target.max_samples = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_resource_limits_max_instances_qos(
  DDS::ResourceLimitsQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "resource_limits.max_instances")) {
    target.max_instances = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_resource_limits_max_samples_per_instance_qos(
  DDS::ResourceLimitsQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "resource_limits.max_samples_per_instance")) {
    target.max_samples_per_instance = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_transport_priority_qos(
  DDS::TransportPriorityQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "transport_priority.value")) {
    target.value = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_lifespan_duration_qos(
  DDS::LifespanQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.duration, "lifespan.duration", name, value);
}

bool set_ownership_kind_qos(
  DDS::OwnershipQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "ownership.kind")) {
    if (!std::strcmp(value, "SHARED")) {
      target.kind = DDS::SHARED_OWNERSHIP_QOS;
      matched = true;
    } else if (!std::strcmp(value, "EXCLUSIVE")) {
      target.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
      matched = true;
    }
  }
  return matched;
}

bool set_ownership_strength_value_qos(
  DDS::OwnershipStrengthQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!std::strcmp(name, "ownership_strength.value")) {
    target.value = atoi(value);
    matched = true;
  }
  return matched;
}

bool set_time_based_filter_minimum_separation(
  DDS::TimeBasedFilterQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.minimum_separation, "time_based_filter.minimum_separation", name, value);
}

bool set_reader_data_lifecycle_autopurge_nowriter_samples_delay(
  DDS::ReaderDataLifecycleQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.autopurge_nowriter_samples_delay,
    "reader_data_lifecycle.autopurge_nowriter_samples_delay", name, value);
}

bool set_reader_data_lifecycle_autopurge_disposed_samples_delay(
  DDS::ReaderDataLifecycleQosPolicy& target, const char* name, const char* value)
{
  return set_duration_qos_value(
    target.autopurge_disposed_samples_delay,
    "reader_data_lifecycle.autopurge_disposed_samples_delay", name, value);
}

void
log_parser_error(const char* section, const char* name, const char* value)
{
  ACE_DEBUG((LM_ERROR, "Could not set %s QOS setting %s to value %s\n",
    section, name, value));
}

int QosSettings::set_qos(
  DDS::PublisherQos& target, const char* name, const char* value)
{
  bool matched =
    set_presentation_access_scope_qos(target.presentation, name, value) ||
    set_presentation_coherent_access_qos(target.presentation, name, value) ||
    set_presentation_ordered_access_qos(target.presentation, name, value) ||
    set_partition_name_qos(target.partition, name, value);
    // group data not settable
    // entity factory not settable

  if (!matched) {
    log_parser_error("publisher", name, value);
  }
  return matched ? 0 : 1;
}

int QosSettings::set_qos(
  DDS::SubscriberQos& target, const char* name, const char* value)
{
  bool matched =
    set_presentation_access_scope_qos(target.presentation, name, value) ||
    set_presentation_coherent_access_qos(target.presentation, name, value) ||
    set_presentation_ordered_access_qos(target.presentation, name, value) ||
    set_partition_name_qos(target.partition, name, value);
    // group data not settable
    // entity factory not settable
  if (!matched) {
    log_parser_error("subscriber", name, value);
  }
  return matched ? 0 : 1;
}

int QosSettings::set_qos(
  DDS::DataWriterQos& target, const char* name, const char* value)
{
  bool matched =
    set_durability_kind_qos(target.durability, name, value) ||
    // durability service not settable - not supporting those durabilities
    set_deadline_period_qos(target.deadline, name, value) ||
    set_latency_budget_duration_qos(target.latency_budget, name, value) ||
    set_liveliness_kind_qos(target.liveliness, name, value) ||
    set_liveliness_lease_duration_qos(target.liveliness, name, value) ||
    set_reliability_kind_qos(target.reliability, name, value) ||
    set_reliability_max_blocking_time_qos(target.reliability, name, value) ||
    set_destination_order_kind_qos(target.destination_order, name, value) ||
    set_history_kind_qos(target.history, name, value) ||
    set_history_depth_qos(target.history, name, value) ||
    set_resource_limits_max_samples_qos(target.resource_limits, name, value) ||
    set_resource_limits_max_instances_qos(target.resource_limits, name, value) ||
    set_resource_limits_max_samples_per_instance_qos(target.resource_limits, name, value) ||
    set_transport_priority_qos(target.transport_priority, name, value) ||
    set_lifespan_duration_qos(target.lifespan, name, value) ||
    // user_data not settable - can't be retrieved
    set_ownership_kind_qos(target.ownership, name, value) ||
    set_ownership_strength_value_qos(target.ownership_strength, name, value);
    // writer_data_lifecycle not settable - no interface to dispose

  if (!matched) {
    log_parser_error("data writer", name, value);
  }
  return matched ? 0 : 1;
}

int QosSettings::set_qos(
  DDS::DataReaderQos& target, const char* name, const char* value)
{
  bool matched =
    set_durability_kind_qos(target.durability, name, value) ||
    // durability service not settable
    set_deadline_period_qos(target.deadline, name, value) ||
    set_latency_budget_duration_qos(target.latency_budget, name, value) ||
    set_liveliness_kind_qos(target.liveliness, name, value) ||
    set_liveliness_lease_duration_qos(target.liveliness, name, value) ||
    set_reliability_kind_qos(target.reliability, name, value) ||
    set_reliability_max_blocking_time_qos(target.reliability, name, value) ||
    set_destination_order_kind_qos(target.destination_order, name, value) ||
    set_history_kind_qos(target.history, name, value) ||
    set_history_depth_qos(target.history, name, value) ||
    set_resource_limits_max_samples_qos(target.resource_limits, name, value) ||
    set_resource_limits_max_instances_qos(
        target.resource_limits, name, value) ||
    set_resource_limits_max_samples_per_instance_qos(
        target.resource_limits, name, value) ||
    set_ownership_kind_qos(target.ownership, name, value) ||
    set_time_based_filter_minimum_separation(
        target.time_based_filter, name, value) ||
    set_reader_data_lifecycle_autopurge_nowriter_samples_delay(
        target.reader_data_lifecycle, name, value) ||
    set_reader_data_lifecycle_autopurge_disposed_samples_delay(
        target.reader_data_lifecycle, name, value);

  if (!matched) {
    log_parser_error("data reader", name, value);
  }
  return matched ? 0 : 1;
}

} } }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
