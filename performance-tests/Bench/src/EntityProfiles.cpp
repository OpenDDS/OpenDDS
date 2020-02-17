// -*- C++ -*-
//

#include "EntityProfiles.h"

namespace Test {

void
PublicationProfile::copyToWriterQos( ::DDS::DataWriterQos& qos)
{
  if( this->writerQosMask & SetDurabilityQos) {
    qos.durability.kind = this->writerQos.durability.kind;
  }
  if( this->writerQosMask & SetDurabilityServiceDurationQos) {
    qos.durability_service.service_cleanup_delay.sec
      = this->writerQos.durability_service.service_cleanup_delay.sec;
    qos.durability_service.service_cleanup_delay.nanosec
      = this->writerQos.durability_service.service_cleanup_delay.nanosec;
  }
  if( this->writerQosMask & SetDurabilityServiceHistoryKindQos) {
    qos.durability_service.history_kind
      = this->writerQos.durability_service.history_kind;
  }
  if( this->writerQosMask & SetDurabilityServiceHistoryDepthQos) {
    qos.durability_service.history_depth
      = this->writerQos.durability_service.history_depth;
  }
  if( this->writerQosMask & SetDurabilityServiceSamplesQos) {
    qos.durability_service.max_samples
      = this->writerQos.durability_service.max_samples;
  }
  if( this->writerQosMask & SetDurabilityServiceInstancesQos) {
    qos.durability_service.max_instances
      = this->writerQos.durability_service.max_instances;
  }
  if( this->writerQosMask & SetDurabilityServiceSamplesPerInstanceQos) {
    qos.durability_service.max_samples_per_instance
      = this->writerQos.durability_service.max_samples_per_instance;
  }
  if( this->writerQosMask & SetDeadlineQos) {
    qos.deadline.period.sec     = this->writerQos.deadline.period.sec;
    qos.deadline.period.nanosec = this->writerQos.deadline.period.nanosec;
  }
  if( this->writerQosMask & SetLatencyBudgetQos) {
    qos.latency_budget.duration.sec
      = this->writerQos.latency_budget.duration.sec;
    qos.latency_budget.duration.nanosec
      = this->writerQos.latency_budget.duration.nanosec;
  }
  if( this->writerQosMask & SetLivelinessKindQos) {
    qos.liveliness.kind = this->writerQos.liveliness.kind;
  }
  if( this->writerQosMask & SetLivelinessDurationQos) {
    qos.liveliness.lease_duration.sec
      = this->writerQos.liveliness.lease_duration.sec;
    qos.liveliness.lease_duration.nanosec
      = this->writerQos.liveliness.lease_duration.nanosec;
  }
  if( this->writerQosMask & SetReliabilityKindQos) {
    qos.reliability.kind = this->writerQos.reliability.kind;
  }
  if( this->writerQosMask & SetReliabilityMaxBlockingQos) {
    qos.reliability.max_blocking_time.sec
      = this->writerQos.reliability.max_blocking_time.sec;
    qos.reliability.max_blocking_time.nanosec
      = this->writerQos.reliability.max_blocking_time.nanosec;
  }
  if( this->writerQosMask & SetDestinationOrderQos) {
    qos.destination_order.kind = this->writerQos.destination_order.kind;
  }
  if( this->writerQosMask & SetHistoryKindQos) {
    qos.history.kind = this->writerQos.history.kind;
  }
  if( this->writerQosMask & SetHistoryDepthQos) {
    qos.history.depth = this->writerQos.history.depth;
  }
  if( this->writerQosMask & SetResourceMaxSamplesQos) {
    qos.resource_limits.max_samples = this->writerQos.resource_limits.max_samples;
  }
  if( this->writerQosMask & SetResourceMaxInstancesQos) {
    qos.resource_limits.max_instances = this->writerQos.resource_limits.max_instances;
  }
  if( this->writerQosMask & SetResourceMaxSamplesPerInstanceQos) {
    qos.resource_limits.max_samples_per_instance
      = this->writerQos.resource_limits.max_samples_per_instance;
  }
  if( this->writerQosMask & SetTransportPriorityQos) {
    qos.transport_priority.value = this->writerQos.transport_priority.value;
  }
  if( this->writerQosMask & SetLifespanQos) {
    qos.lifespan.duration.sec     = this->writerQos.lifespan.duration.sec;
    qos.lifespan.duration.nanosec = this->writerQos.lifespan.duration.nanosec;
  }
  if( this->writerQosMask & SetUserDataQos) {
    qos.user_data.value = this->writerQos.user_data.value;
//  qos.user_data.value.length( this->writerQos.user_data.value.length());
//  qos.user_data.value.replace(
//    this->writerQos.user_data.value.length(),
//    this->writerQos.user_data.value.length(),
//    &this->writerQos.user_data.value[ 0]
//  );
  }
#ifdef VERSION_1_2
  if( this->writerQosMask & SetOwnershipKindQos) {
    qos.ownership.kind = this->writerQos.ownership.kind;
  }
#endif /* VERSION_1_2 */
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if( this->writerQosMask & SetOwnershipStrengthQos) {
    qos.ownership_strength.value = this->writerQos.ownership_strength.value;
  }
#endif
  if( this->writerQosMask & SetWriterDataLifecycleQos) {
    qos.writer_data_lifecycle.autodispose_unregistered_instances
      = this->writerQos.writer_data_lifecycle.autodispose_unregistered_instances;
  }
}

void
SubscriptionProfile::copyToReaderQos( ::DDS::DataReaderQos& qos)
{
  if( this->readerQosMask & SetDurabilityQos) {
    qos.durability.kind = this->readerQos.durability.kind;
  }
  if( this->readerQosMask & SetDeadlineQos) {
    qos.deadline.period.sec     = this->readerQos.deadline.period.sec;
    qos.deadline.period.nanosec = this->readerQos.deadline.period.nanosec;
  }
  if( this->readerQosMask & SetLatencyBudgetQos) {
    qos.latency_budget.duration.sec
      = this->readerQos.latency_budget.duration.sec;
    qos.latency_budget.duration.nanosec
      = this->readerQos.latency_budget.duration.nanosec;
  }
  if( this->readerQosMask & SetLivelinessKindQos) {
    qos.liveliness.kind = this->readerQos.liveliness.kind;
  }
  if( this->readerQosMask & SetLivelinessDurationQos) {
    qos.liveliness.lease_duration.sec
      = this->readerQos.liveliness.lease_duration.sec;
    qos.liveliness.lease_duration.nanosec
      = this->readerQos.liveliness.lease_duration.nanosec;
  }
  if( this->readerQosMask & SetReliabilityKindQos) {
    qos.reliability.kind = this->readerQos.reliability.kind;
  }
  if( this->readerQosMask & SetReliabilityMaxBlockingQos) {
    qos.reliability.max_blocking_time.sec
      = this->readerQos.reliability.max_blocking_time.sec;
    qos.reliability.max_blocking_time.nanosec
      = this->readerQos.reliability.max_blocking_time.nanosec;
  }
  if( this->readerQosMask & SetDestinationOrderQos) {
    qos.destination_order.kind = this->readerQos.destination_order.kind;
  }
  if( this->readerQosMask & SetHistoryKindQos) {
    qos.history.kind = this->readerQos.history.kind;
  }
  if( this->readerQosMask & SetHistoryDepthQos) {
    qos.history.depth = this->readerQos.history.depth;
  }
  if( this->readerQosMask & SetResourceMaxSamplesQos) {
    qos.resource_limits.max_samples = this->readerQos.resource_limits.max_samples;
  }
  if( this->readerQosMask & SetResourceMaxInstancesQos) {
    qos.resource_limits.max_instances = this->readerQos.resource_limits.max_instances;
  }
  if( this->readerQosMask & SetResourceMaxSamplesPerInstanceQos) {
    qos.resource_limits.max_samples_per_instance
      = this->readerQos.resource_limits.max_samples_per_instance;
  }
  if( this->readerQosMask & SetUserDataQos) {
    qos.user_data.value = this->readerQos.user_data.value;
//  qos.user_data.value.length( this->readerQos.user_data.value.length());
//  qos.user_data.value.replace(
//    this->readerQos.user_data.value.length(),
//    this->readerQos.user_data.value.length(),
//    &this->readerQos.user_data.value[ 0]
//  );
  }
  if( this->readerQosMask & SetTimeBasedFilterQos) {
    qos.time_based_filter.minimum_separation.sec
      = this->readerQos.time_based_filter.minimum_separation.sec;
    qos.time_based_filter.minimum_separation.nanosec
      = this->readerQos.time_based_filter.minimum_separation.nanosec;
  }
  if( this->readerQosMask & SetReaderDataLifecycleQos) {
    qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec
      = this->readerQos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec;
    qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec
      = this->readerQos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec;
    qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec
      = this->readerQos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec;
    qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec
      = this->readerQos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec;
  }
}

} // End of namespace Test

