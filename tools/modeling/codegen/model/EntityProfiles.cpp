// -*- C++ -*-
//
// $Id$

#include "EntityProfiles.h"

namespace OpenDDS { namespace Model {

void
WriterProfile::copyToWriterQos( ::DDS::DataWriterQos& qos)
{
  if( this->mask & SetDurabilityQos) {
    qos.durability.kind = this->qos.durability.kind;
  }
  if( this->mask & SetDurabilityServiceDurationQos) {
    qos.durability_service.service_cleanup_delay.sec
      = this->qos.durability_service.service_cleanup_delay.sec;
    qos.durability_service.service_cleanup_delay.nanosec
      = this->qos.durability_service.service_cleanup_delay.nanosec;
  }
  if( this->mask & SetDurabilityServiceHistoryKindQos) {
    qos.durability_service.history_kind
      = this->qos.durability_service.history_kind;
  }
  if( this->mask & SetDurabilityServiceHistoryDepthQos) {
    qos.durability_service.history_depth
      = this->qos.durability_service.history_depth;
  }
  if( this->mask & SetDurabilityServiceSamplesQos) {
    qos.durability_service.max_samples
      = this->qos.durability_service.max_samples;
  }
  if( this->mask & SetDurabilityServiceInstancesQos) {
    qos.durability_service.max_instances
      = this->qos.durability_service.max_instances;
  }
  if( this->mask & SetDurabilityServiceSamplesPerInstanceQos) {
    qos.durability_service.max_samples_per_instance
      = this->qos.durability_service.max_samples_per_instance;
  }
  if( this->mask & SetDeadlineQos) {
    qos.deadline.period.sec     = this->qos.deadline.period.sec;
    qos.deadline.period.nanosec = this->qos.deadline.period.nanosec;
  }
  if( this->mask & SetLatencyBudgetQos) {
    qos.latency_budget.duration.sec
      = this->qos.latency_budget.duration.sec;
    qos.latency_budget.duration.nanosec
      = this->qos.latency_budget.duration.nanosec;
  }
  if( this->mask & SetLivelinessKindQos) {
    qos.liveliness.kind = this->qos.liveliness.kind;
  }
  if( this->mask & SetLivelinessDurationQos) {
    qos.liveliness.lease_duration.sec
      = this->qos.liveliness.lease_duration.sec;
    qos.liveliness.lease_duration.nanosec
      = this->qos.liveliness.lease_duration.nanosec;
  }
  if( this->mask & SetReliabilityKindQos) {
    qos.reliability.kind = this->qos.reliability.kind;
  }
  if( this->mask & SetReliabilityMaxBlockingQos) {
    qos.reliability.max_blocking_time.sec
      = this->qos.reliability.max_blocking_time.sec;
    qos.reliability.max_blocking_time.nanosec
      = this->qos.reliability.max_blocking_time.nanosec;
  }
  if( this->mask & SetDestinationOrderQos) {
    qos.destination_order.kind = this->qos.destination_order.kind;
  }
  if( this->mask & SetHistoryKindQos) {
    qos.history.kind = this->qos.history.kind;
  }
  if( this->mask & SetHistoryDepthQos) {
    qos.history.depth = this->qos.history.depth;
  }
  if( this->mask & SetResourceMaxSamplesQos) {
    qos.resource_limits.max_samples = this->qos.resource_limits.max_samples;
  }
  if( this->mask & SetResourceMaxInstancesQos) {
    qos.resource_limits.max_instances = this->qos.resource_limits.max_instances;
  }
  if( this->mask & SetResourceMaxSamplesPerInstanceQos) {
    qos.resource_limits.max_samples_per_instance
      = this->qos.resource_limits.max_samples_per_instance;
  }
  if( this->mask & SetTransportPriorityQos) {
    qos.transport_priority.value = this->qos.transport_priority.value;
  }
  if( this->mask & SetLifespanQos) {
    qos.lifespan.duration.sec     = this->qos.lifespan.duration.sec;
    qos.lifespan.duration.nanosec = this->qos.lifespan.duration.nanosec;
  }
  if( this->mask & SetUserDataQos) {
    qos.user_data.value = this->qos.user_data.value;
//  qos.user_data.value.length( this->qos.user_data.value.length());
//  qos.user_data.value.replace(
//    this->qos.user_data.value.length(),
//    this->qos.user_data.value.length(),
//    &this->qos.user_data.value[ 0]
//  );
  }
  if( this->mask & SetOwnershipKindQos) {
    qos.ownership.kind = this->qos.ownership.kind;
  }
  if( this->mask & SetOwnershipStrengthQos) {
    qos.ownership_strength.value = this->qos.ownership_strength.value;
  }
  if( this->mask & SetWriterDataLifecycleQos) {
    qos.writer_data_lifecycle.autodispose_unregistered_instances
      = this->qos.writer_data_lifecycle.autodispose_unregistered_instances;
  }
}

void
ReaderProfile::copyToReaderQos( ::DDS::DataReaderQos& qos)
{
  if( this->mask & SetDurabilityQos) {
    qos.durability.kind = this->qos.durability.kind;
  }
  if( this->mask & SetDeadlineQos) {
    qos.deadline.period.sec     = this->qos.deadline.period.sec;
    qos.deadline.period.nanosec = this->qos.deadline.period.nanosec;
  }
  if( this->mask & SetLatencyBudgetQos) {
    qos.latency_budget.duration.sec
      = this->qos.latency_budget.duration.sec;
    qos.latency_budget.duration.nanosec
      = this->qos.latency_budget.duration.nanosec;
  }
  if( this->mask & SetLivelinessKindQos) {
    qos.liveliness.kind = this->qos.liveliness.kind;
  }
  if( this->mask & SetLivelinessDurationQos) {
    qos.liveliness.lease_duration.sec
      = this->qos.liveliness.lease_duration.sec;
    qos.liveliness.lease_duration.nanosec
      = this->qos.liveliness.lease_duration.nanosec;
  }
  if( this->mask & SetReliabilityKindQos) {
    qos.reliability.kind = this->qos.reliability.kind;
  }
  if( this->mask & SetReliabilityMaxBlockingQos) {
    qos.reliability.max_blocking_time.sec
      = this->qos.reliability.max_blocking_time.sec;
    qos.reliability.max_blocking_time.nanosec
      = this->qos.reliability.max_blocking_time.nanosec;
  }
  if( this->mask & SetDestinationOrderQos) {
    qos.destination_order.kind = this->qos.destination_order.kind;
  }
  if( this->mask & SetHistoryKindQos) {
    qos.history.kind = this->qos.history.kind;
  }
  if( this->mask & SetHistoryDepthQos) {
    qos.history.depth = this->qos.history.depth;
  }
  if( this->mask & SetResourceMaxSamplesQos) {
    qos.resource_limits.max_samples = this->qos.resource_limits.max_samples;
  }
  if( this->mask & SetResourceMaxInstancesQos) {
    qos.resource_limits.max_instances = this->qos.resource_limits.max_instances;
  }
  if( this->mask & SetResourceMaxSamplesPerInstanceQos) {
    qos.resource_limits.max_samples_per_instance
      = this->qos.resource_limits.max_samples_per_instance;
  }
  if( this->mask & SetUserDataQos) {
    qos.user_data.value = this->qos.user_data.value;
//  qos.user_data.value.length( this->qos.user_data.value.length());
//  qos.user_data.value.replace(
//    this->qos.user_data.value.length(),
//    this->qos.user_data.value.length(),
//    &this->qos.user_data.value[ 0]
//  );
  }
  if( this->mask & SetTimeBasedFilterQos) {
    qos.time_based_filter.minimum_separation.sec
      = this->qos.time_based_filter.minimum_separation.sec;
    qos.time_based_filter.minimum_separation.nanosec
      = this->qos.time_based_filter.minimum_separation.nanosec;
  }
  if( this->mask & SetReaderDataLifecycleQos) {
    qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec
      = this->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec;
    qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec
      = this->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec;
    qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec
      = this->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec;
    qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec
      = this->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec;
  }
}

} } // End of namespace OpenDDS::Model

