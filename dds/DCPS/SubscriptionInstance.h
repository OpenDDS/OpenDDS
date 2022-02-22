/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H
#define OPENDDS_DCPS_SUBSCRIPTION_INSTANCE_H

#include "dcps_export.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"
#include "InstanceState.h"
#include "RcObject.h"

#include "dds/DdsDcpsInfrastructureC.h"

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
class OpenDDS_Dcps_Export SubscriptionInstance : public RcObject {
public:
  SubscriptionInstance(DataReaderImpl* reader,
                       const DDS::DataReaderQos& qos,
                       ACE_Recursive_Thread_Mutex& lock,
                       DDS::InstanceHandle_t handle,
                       bool owns_handle);

  ~SubscriptionInstance();

  bool matches(CORBA::ULong sample_states, CORBA::ULong view_states, CORBA::ULong instance_states) const;

  /// Instance state for this instance
  const InstanceState_rch instance_state_;

  /// Sequence number of the move recent data sample received
  SequenceNumber last_sequence_;

  /// Data sample(s) in this instance
  ReceivedDataElementList rcvd_samples_;

  CORBA::ULong read_sample_count_;
  CORBA::ULong not_read_sample_count_;
  CORBA::ULong sample_states_;

  /// ReceivedDataElementList strategy
  unique_ptr<ReceivedDataStrategy> rcvd_strategy_;

  /// The instance handle for the registered object
  const DDS::InstanceHandle_t instance_handle_;

  const bool owns_handle_;

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
