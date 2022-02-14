/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_WRITER_H
#define OPENDDS_DCPS_INTERNAL_DATA_WRITER_H

#include "TimeTypes.h"
#include "RcObject.h"
#include "DurabilityService.h"
#include "dds/DdsDcpsInfrastructureC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <class Sample> class InternalTopic;
template <class Sample> class InternalDataReader;

template<typename Sample>
class InternalDataWriter : public RcObject {
public:
  typedef RcHandle<InternalTopic<Sample> > InternalTopicPtr;
  typedef RcHandle<InternalDataWriter> InternalDataWriterPtr;
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  InternalDataWriter(InternalTopicPtr topic,
                     DDS::InstanceHandle_t publication_handle,
                     const DDS::DataWriterQos& qos)
    : topic_(topic)
    , publication_handle_(publication_handle)
    , qos_(qos)
  {
    switch (qos_.durability.kind) {
    case DDS::VOLATILE_DURABILITY_QOS:
      durability_service_ = make_rch<VolatileDurabilityService<Sample> >();
      break;
    case DDS::TRANSIENT_LOCAL_DURABILITY_QOS:
      durability_service_ = make_rch<TransientLocalDurabilityService<Sample> >();
      break;
    case DDS::TRANSIENT_DURABILITY_QOS:
      durability_service_ = make_rch<TransientDurabilityService<Sample> >();
      break;
    case DDS::PERSISTENT_DURABILITY_QOS:
      durability_service_ = make_rch<PersistentDurabilityService<Sample> >();
      break;
    }
    OPENDDS_ASSERT(durability_service_);
  }

  DDS::InstanceHandle_t get_publication_handle() const { return publication_handle_; }

  void add_reader(InternalDataReaderPtr reader)
  {
    durability_service_->add_reader(reader, publication_handle_);
  }

  void register_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    durability_service_->register_instance(publication_handle_, sample, source_timestamp);
    topic_->register_instance(sample, source_timestamp, get_publication_handle());
  }

  void write(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    durability_service_->store(sample, source_timestamp);
    topic_->store(sample, source_timestamp, get_publication_handle());
  }

  void unregister_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    durability_service_->unregister_instance(publication_handle_, sample, source_timestamp);
    topic_->unregister_instance(sample, source_timestamp, get_publication_handle());
  }

  void dispose_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    durability_service_->dispose_instance(publication_handle_, sample, source_timestamp);
    topic_->dispose_instance(sample, source_timestamp, get_publication_handle());
  }

private:
  InternalTopicPtr topic_;
  RcHandle<DurabilityService<Sample> > durability_service_;
  const DDS::InstanceHandle_t publication_handle_;
  const DDS::DataWriterQos qos_;
};

void assign_default_internal_data_writer_qos(DDS::DataWriterQos& qos)
{
  const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };
  const DDS::Duration_t infinity = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

  qos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
  qos.durability_service.service_cleanup_delay = zero;
  qos.durability_service.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
  qos.durability_service.history_depth = 1;
  qos.durability_service.max_samples = DDS::LENGTH_UNLIMITED;
  qos.durability_service.max_instances = DDS::LENGTH_UNLIMITED;
  qos.durability_service.max_samples_per_instance = DDS::LENGTH_UNLIMITED;
  qos.deadline.period = infinity;
  qos.latency_budget.duration = zero;
  qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
  qos.liveliness.lease_duration = infinity;
  qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  qos.reliability.max_blocking_time = { 0, 100000000 }; // 100ms
  qos.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
  qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  qos.history.depth = 1;
  qos.resource_limits.max_samples = DDS::LENGTH_UNLIMITED;
  qos.resource_limits.max_instances = DDS::LENGTH_UNLIMITED;
  qos.resource_limits.max_samples_per_instance = DDS::LENGTH_UNLIMITED;
  qos.transport_priority.value = 0;
  qos.lifespan.duration = infinity;
  //qos.user_data;
  qos.ownership.kind = DDS::SHARED_OWNERSHIP_QOS;
  qos.ownership_strength.value = 0;
  qos.writer_data_lifecycle.autodispose_unregistered_instances = true;
  //qos.representation;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_INTERNAL_DATA_WRITER_H */
