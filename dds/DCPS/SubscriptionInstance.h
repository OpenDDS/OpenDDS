/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H
#define OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H

#include "ace/OS_Memory.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include "dcps_export.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"
#include "InstanceState.h"
#include "PoolAllocationBase.h"
#include "RcObject.h"
#include "ace/Synch_Traits.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

/**
  * @class SubscriptionInstance
  *
  * @brief Struct that has information about an instance and the instance
  *        sample list.
  */
class SubscriptionInstance : public RcObject {
public:
  SubscriptionInstance(DataReaderImpl *reader,
                       const DDS::DataReaderQos& qos,
                       ACE_Recursive_Thread_Mutex& lock,
                       DDS::InstanceHandle_t handle)
    : instance_state_(make_rch<InstanceState>(reader, ref(lock), handle)),
      last_sequence_(),
      rcvd_samples_(instance_state_),
      instance_handle_(handle),
      deadline_timer_id_(-1)
  {
    switch (qos.destination_order.kind) {
    case DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS:
      this->rcvd_strategy_.reset(new ReceptionDataStrategy(this->rcvd_samples_));
      break;

    case DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS:
      this->rcvd_strategy_.reset(new SourceDataStrategy(this->rcvd_samples_));
      break;
    }

    if (!this->rcvd_strategy_) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: SubscriptionInstance: ")
                 ACE_TEXT(" unable to allocate ReceiveDataStrategy!\n")));
    }
  }

  /// Instance state for this instance
  InstanceState_rch instance_state_;

  /// Sequence number of the move recent data sample received
  SequenceNumber last_sequence_ ;

  /// Data sample(s) in this instance
  ReceivedDataElementList rcvd_samples_ ;

  /// ReceivedDataElementList strategy
  unique_ptr<ReceivedDataStrategy> rcvd_strategy_;

  /// The instance handle for the registered object
  DDS::InstanceHandle_t instance_handle_;

  MonotonicTimePoint last_sample_tv_;

  MonotonicTimePoint cur_sample_tv_;

  long deadline_timer_id_;

  MonotonicTimePoint last_accepted_;
};

typedef RcHandle<SubscriptionInstance> SubscriptionInstance_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H */
