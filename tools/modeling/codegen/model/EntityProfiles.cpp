// -*- C++ -*-
//

#include "EntityProfiles.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model {

void
WriterProfile::copyToWriterQos(::DDS::DataWriterQos& dds_qos)
{
  if( this->mask & SetDurabilityQos) {
    dds_qos.durability.kind = this->qos.durability.kind;
  }
  if( this->mask & SetDurabilityServiceDurationQos) {
    dds_qos.durability_service.service_cleanup_delay.sec
      = this->qos.durability_service.service_cleanup_delay.sec;
    dds_qos.durability_service.service_cleanup_delay.nanosec
      = this->qos.durability_service.service_cleanup_delay.nanosec;
  }
  if( this->mask & SetDurabilityServiceHistoryKindQos) {
    dds_qos.durability_service.history_kind
      = this->qos.durability_service.history_kind;
  }
  if( this->mask & SetDurabilityServiceHistoryDepthQos) {
    dds_qos.durability_service.history_depth
      = this->qos.durability_service.history_depth;
  }
  if( this->mask & SetDurabilityServiceSamplesQos) {
    dds_qos.durability_service.max_samples
      = this->qos.durability_service.max_samples;
  }
  if( this->mask & SetDurabilityServiceInstancesQos) {
    dds_qos.durability_service.max_instances
      = this->qos.durability_service.max_instances;
  }
  if( this->mask & SetDurabilityServiceSamplesPerInstanceQos) {
    dds_qos.durability_service.max_samples_per_instance
      = this->qos.durability_service.max_samples_per_instance;
  }
  if( this->mask & SetDeadlineQos) {
    dds_qos.deadline.period.sec     = this->qos.deadline.period.sec;
    dds_qos.deadline.period.nanosec = this->qos.deadline.period.nanosec;
  }
  if( this->mask & SetLatencyBudgetQos) {
    dds_qos.latency_budget.duration.sec
      = this->qos.latency_budget.duration.sec;
    dds_qos.latency_budget.duration.nanosec
      = this->qos.latency_budget.duration.nanosec;
  }
  if( this->mask & SetLivelinessKindQos) {
    dds_qos.liveliness.kind = this->qos.liveliness.kind;
  }
  if( this->mask & SetLivelinessDurationQos) {
    dds_qos.liveliness.lease_duration.sec
      = this->qos.liveliness.lease_duration.sec;
    dds_qos.liveliness.lease_duration.nanosec
      = this->qos.liveliness.lease_duration.nanosec;
  }
  if( this->mask & SetReliabilityKindQos) {
    dds_qos.reliability.kind = this->qos.reliability.kind;
  }
  if( this->mask & SetReliabilityMaxBlockingQos) {
    dds_qos.reliability.max_blocking_time.sec
      = this->qos.reliability.max_blocking_time.sec;
    dds_qos.reliability.max_blocking_time.nanosec
      = this->qos.reliability.max_blocking_time.nanosec;
  }
  if( this->mask & SetDestinationOrderQos) {
    dds_qos.destination_order.kind = this->qos.destination_order.kind;
  }
  if( this->mask & SetHistoryKindQos) {
    dds_qos.history.kind = this->qos.history.kind;
  }
  if( this->mask & SetHistoryDepthQos) {
    dds_qos.history.depth = this->qos.history.depth;
  }
  if( this->mask & SetResourceMaxSamplesQos) {
    dds_qos.resource_limits.max_samples = this->qos.resource_limits.max_samples;
  }
  if( this->mask & SetResourceMaxInstancesQos) {
    dds_qos.resource_limits.max_instances = this->qos.resource_limits.max_instances;
  }
  if( this->mask & SetResourceMaxSamplesPerInstanceQos) {
    dds_qos.resource_limits.max_samples_per_instance
      = this->qos.resource_limits.max_samples_per_instance;
  }
  if( this->mask & SetTransportPriorityQos) {
    dds_qos.transport_priority.value = this->qos.transport_priority.value;
  }
  if( this->mask & SetLifespanQos) {
    dds_qos.lifespan.duration.sec     = this->qos.lifespan.duration.sec;
    dds_qos.lifespan.duration.nanosec = this->qos.lifespan.duration.nanosec;
  }
  if( this->mask & SetUserDataQos) {
    dds_qos.user_data.value = this->qos.user_data.value;
//  dds_qos.user_data.value.length( this->qos.user_data.value.length());
//  dds_qos.user_data.value.replace(
//    this->qos.user_data.value.length(),
//    this->qos.user_data.value.length(),
//    &this->qos.user_data.value[ 0]
//  );
  }
  if( this->mask & SetOwnershipKindQos) {
    dds_qos.ownership.kind = this->qos.ownership.kind;
  }
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if( this->mask & SetOwnershipStrengthQos) {
    dds_qos.ownership_strength.value = this->qos.ownership_strength.value;
  }
#endif
  if( this->mask & SetWriterDataLifecycleQos) {
    dds_qos.writer_data_lifecycle.autodispose_unregistered_instances
      = this->qos.writer_data_lifecycle.autodispose_unregistered_instances;
  }
}

void
ReaderProfile::copyToReaderQos(::DDS::DataReaderQos& dds_qos)
{
  if( this->mask & SetDurabilityQos) {
    dds_qos.durability.kind = this->qos.durability.kind;
  }
  if( this->mask & SetDeadlineQos) {
    dds_qos.deadline.period.sec     = this->qos.deadline.period.sec;
    dds_qos.deadline.period.nanosec = this->qos.deadline.period.nanosec;
  }
  if( this->mask & SetLatencyBudgetQos) {
    dds_qos.latency_budget.duration.sec
      = this->qos.latency_budget.duration.sec;
    dds_qos.latency_budget.duration.nanosec
      = this->qos.latency_budget.duration.nanosec;
  }
  if( this->mask & SetLivelinessKindQos) {
    dds_qos.liveliness.kind = this->qos.liveliness.kind;
  }
  if( this->mask & SetLivelinessDurationQos) {
    dds_qos.liveliness.lease_duration.sec
      = this->qos.liveliness.lease_duration.sec;
    dds_qos.liveliness.lease_duration.nanosec
      = this->qos.liveliness.lease_duration.nanosec;
  }
  if( this->mask & SetReliabilityKindQos) {
    dds_qos.reliability.kind = this->qos.reliability.kind;
  }
  if( this->mask & SetReliabilityMaxBlockingQos) {
    dds_qos.reliability.max_blocking_time.sec
      = this->qos.reliability.max_blocking_time.sec;
    dds_qos.reliability.max_blocking_time.nanosec
      = this->qos.reliability.max_blocking_time.nanosec;
  }
  if( this->mask & SetDestinationOrderQos) {
    dds_qos.destination_order.kind = this->qos.destination_order.kind;
  }
  if( this->mask & SetHistoryKindQos) {
    dds_qos.history.kind = this->qos.history.kind;
  }
  if( this->mask & SetHistoryDepthQos) {
    dds_qos.history.depth = this->qos.history.depth;
  }
  if( this->mask & SetResourceMaxSamplesQos) {
    dds_qos.resource_limits.max_samples = this->qos.resource_limits.max_samples;
  }
  if( this->mask & SetResourceMaxInstancesQos) {
    dds_qos.resource_limits.max_instances = this->qos.resource_limits.max_instances;
  }
  if( this->mask & SetResourceMaxSamplesPerInstanceQos) {
    dds_qos.resource_limits.max_samples_per_instance
      = this->qos.resource_limits.max_samples_per_instance;
  }
  if( this->mask & SetUserDataQos) {
    dds_qos.user_data.value = this->qos.user_data.value;
//  dds_qos.user_data.value.length( this->qos.user_data.value.length());
//  dds_qos.user_data.value.replace(
//    this->qos.user_data.value.length(),
//    this->qos.user_data.value.length(),
//    &this->qos.user_data.value[ 0]
//  );
  }
  if( this->mask & SetTimeBasedFilterQos) {
    dds_qos.time_based_filter.minimum_separation.sec
      = this->qos.time_based_filter.minimum_separation.sec;
    dds_qos.time_based_filter.minimum_separation.nanosec
      = this->qos.time_based_filter.minimum_separation.nanosec;
  }
  if( this->mask & SetReaderDataLifecycleQos) {
    dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec
      = this->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec;
    dds_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec
      = this->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec;
    dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec
      = this->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec;
    dds_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec
      = this->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec;
  }
}

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL
