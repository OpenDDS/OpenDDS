/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "SubscriptionInstance.h"

#include "DataReaderImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

SubscriptionInstance::SubscriptionInstance(DataReaderImpl* reader,
                                           const DDS::DataReaderQos& qos,
                                           ACE_Recursive_Thread_Mutex& lock,
                                           DDS::InstanceHandle_t handle,
                                           bool owns_handle)
  : instance_state_(make_rch<InstanceState>(reader, ref(lock), handle))
  , rcvd_samples_(reader, instance_state_)
  , read_sample_count_(0)
  , not_read_sample_count_(0)
  , sample_states_(0)
  , instance_handle_(handle)
  , owns_handle_(owns_handle)
  , deadline_timer_id_(-1)
{
  switch (qos.destination_order.kind) {
  case DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS:
    rcvd_strategy_.reset(new ReceptionDataStrategy(rcvd_samples_));
    break;

  case DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS:
    rcvd_strategy_.reset(new SourceDataStrategy(rcvd_samples_));
    break;
  }

  if (!rcvd_strategy_) {
    ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: SubscriptionInstance: ")
                ACE_TEXT("unable to allocate ReceiveDataStrategy!\n")));
  }
}

SubscriptionInstance::~SubscriptionInstance()
{
  if (owns_handle_) {
    const RcHandle<DataReaderImpl> reader = instance_state_->data_reader().lock();
    if (reader) {
      reader->return_handle(instance_handle_);
    }
  }
}

bool SubscriptionInstance::matches(CORBA::ULong sample_states, CORBA::ULong view_states, CORBA::ULong instance_states) const
{
  return instance_state_->match(view_states, instance_states) && rcvd_samples_.matches(sample_states);
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
