#include "QosSettings.h"

namespace OpenDDS { namespace FACE { namespace config {

QosSettings::QosSettings() :
  publisher_qos_()
, subscriber_qos_()
, data_writer_qos_()
, data_reader_qos_()
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
  target = data_writer_qos_;
}

void
QosSettings::apply_to(DDS::DataReaderQos&  target) const
{
  target = data_reader_qos_;
}

void
QosSettings::set_qos(QosLevel level, const char* name, const char* value)
{
  switch(level) {
    case publisher:
      set_qos(publisher_qos_, name, value);
      break;
    case subscriber:
      set_qos(subscriber_qos_, name, value);
      break;
    case data_writer:
      set_qos(data_writer_qos_, name, value);
      break;
    case data_reader:
      set_qos(data_reader_qos_, name, value);
      break;
  }
}

bool
set_presentation_access_scope_kind_qos(
  DDS::PresentationQosPolicy& target,
  const char* name,
  const char* value)
{
  bool matched = false;
  if (!strcmp(name, "presentation.access_scope")) {
    if (!strcmp(value, "INSTANCE")) {
      target.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
      matched = true;
    }
    if (!strcmp(value, "TOPIC")) {
      target.access_scope = DDS::TOPIC_PRESENTATION_QOS;
      matched = true;
    }
    if (!strcmp(value, "GROUP")) {
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
  if (!strcmp(value, "true")) {
    target = true;
    matched = true;
  } else if (!strcmp(value, "false")) {
    target = false;
    matched = true;
  }
  return matched;
}

bool
set_presentation_coherent_access_qos(
  DDS::PresentationQosPolicy& target,
  const char* name,
  const char* value)
{
  bool matched = false;
  if (!strcmp(name, "presentation.coherent_access")) {
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
  if (!strcmp(name, "presentation.ordered_access")) {
    matched = set_bool_qos_value(target.ordered_access, value);
  }
  return matched;
}

bool
set_partition_name_qos(
  DDS::PartitionQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!strcmp(name, "partition.name")) {
    target.name.length(target.name.length() + 1);
    target.name[target.name.length() - 1] = value;
    matched = true;
  }
  return matched;
}

bool
set_durability_kind_qos(
  DDS::DurabilityQosPolicy& target, const char* name, const char* value)
{
  bool matched = false;
  if (!strcmp(name, "durability.kind")) {
    if (!strcmp(value, "VOLATILE")) {
      target.kind = DDS::VOLATILE_DURABILITY_QOS;
      matched = true;
    } else if (!strcmp(value, "TRANSIENT_LOCAL")) {
      target.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      matched = true;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    } else if (!strcmp(value, "TRANSIENT")) {
      target.kind = DDS::TRANSIENT_DURABILITY_QOS;
      matched = true;
    } else if (!strcmp(value, "PERSISTENT")) {
      target.kind = DDS::PERSISTENT_DURABILITY_QOS;
      matched = true;
#endif
    }
  }
  return matched;
}

void
log_parser_error(const char* section, const char* name, const char* value)
{
  ACE_DEBUG((LM_ERROR, "Could not set %s QOS setting %s to value %s\n",
    section, name, value));
}

void QosSettings::set_qos(
  DDS::PublisherQos& target, const char* name, const char* value)
{
  bool matched = 
    set_presentation_access_scope_kind_qos(target.presentation, name, value) ||
    set_presentation_coherent_access_qos(target.presentation, name, value) ||
    set_presentation_ordered_access_qos(target.presentation, name, value) ||
    set_partition_name_qos(target.partition, name, value);
    // group data not settable
    // entity factory not settable

  if (!matched) {
    log_parser_error("publisher", name, value);
  }
}

void QosSettings::set_qos(
  DDS::SubscriberQos& target, const char* name, const char* value)
{
  bool matched = 
    set_presentation_access_scope_kind_qos(target.presentation, name, value) ||
    set_presentation_coherent_access_qos(target.presentation, name, value) ||
    set_presentation_ordered_access_qos(target.presentation, name, value) ||
    set_partition_name_qos(target.partition, name, value);
    // group data not settable
    // entity factory not settable
  if (!matched) {
    log_parser_error("subscriber", name, value);
  }
}

void QosSettings::set_qos(
  DDS::DataWriterQos& target, const char* name, const char* value)
{
  bool matched = 
    set_durability_kind_qos(target.durability, name, value) ||
    false;

  if (!matched) {
    log_parser_error("data writer", name, value);
  }
}

void QosSettings::set_qos(
  DDS::DataReaderQos& target, const char* name, const char* value)
{
  bool matched = 
    set_durability_kind_qos(target.durability, name, value) ||
    false;

  if (!matched) {
    log_parser_error("data reader", name, value);
  }
}

} } }

