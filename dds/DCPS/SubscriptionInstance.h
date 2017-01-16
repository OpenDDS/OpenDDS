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
#include "RcObject_T.h"
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
class SubscriptionInstance : public RcObject<ACE_SYNCH_MUTEX> {
public:
  SubscriptionInstance(DataReaderImpl *reader,
                       const DDS::DataReaderQos& qos,
                       ACE_Recursive_Thread_Mutex& lock,
                       DDS::InstanceHandle_t handle)
    : instance_state_(reader, lock, handle),
      last_sequence_(),
      rcvd_samples_(&instance_state_),
      rcvd_strategy_(0),
      instance_handle_(handle),
      deadline_timer_id_(-1)
  {
    switch (qos.destination_order.kind) {
    case DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS:
      ACE_NEW_NORETURN(this->rcvd_strategy_,
                       ReceptionDataStrategy(this->rcvd_samples_));
      break;

    case DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS:
      ACE_NEW_NORETURN(this->rcvd_strategy_,
                       SourceDataStrategy(this->rcvd_samples_));
      break;
    }

    if (this->rcvd_strategy_ == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: SubscriptionInstance: ")
                 ACE_TEXT(" unable to allocate ReceiveDataStrategy!\n")));
    }
  }

  ~SubscriptionInstance()
  {
    delete this->rcvd_strategy_;
  }

  /// Instance state for this instance
  InstanceState instance_state_ ;

  /// sequence number of the move recect data sample received
  SequenceNumber last_sequence_ ;

  /// Data sample(s) in this instance
  ReceivedDataElementList rcvd_samples_ ;

  /// ReceivedDataElementList strategy
  ReceivedDataStrategy* rcvd_strategy_;

  /// The instance handle for the registered object
  DDS::InstanceHandle_t instance_handle_;

  ACE_Time_Value   last_sample_tv_;

  ACE_Time_Value   cur_sample_tv_;

  long             deadline_timer_id_;

  ACE_Time_Value   last_accepted_;
};

typedef RcHandle<SubscriptionInstance> SubscriptionInstance_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H */
