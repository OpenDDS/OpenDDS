/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "Observer.h"
#include "DataSampleElement.h"
#include "ReceivedDataElementList.h"
#include <dds/DCPS/transport/framework/ReceivedDataSample.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

CORBA::ULong Observer::Sample::to_instance_state(char message_id)
{
  switch (message_id) {
  case UNREGISTER_INSTANCE:
    return DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
  case DISPOSE_INSTANCE:
  case DISPOSE_UNREGISTER_INSTANCE:
    return DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
  default:
    return DDS::ALIVE_INSTANCE_STATE;
  }
}

Observer::Sample::Sample(DDS::InstanceHandle_t i, const DataSampleElement& e, const DDS::Time_t& t)
  : instance_(i)
  , instance_state_(to_instance_state(e.get_header().message_id_))
  , timestamp_(t)
  , seq_n_(e.get_header().sequence_)
  , data_(e.get_sample())
{}

Observer::Sample::Sample(const ReceivedDataSample& s, DDS::InstanceHandle_t i)
  : instance_(i)
  , instance_state_(to_instance_state(s.header_.message_id_))
  , timestamp_{s.header_.source_timestamp_sec_, s.header_.source_timestamp_nanosec_}
  , seq_n_(s.header_.sequence_)
  , data_(s.sample_.get())
{}

Observer::Sample::Sample(const ReceivedDataElement& s, DDS::InstanceHandle_t i, CORBA::ULong instance_state)
  : instance_(i)
  , instance_state_(to_instance_state(instance_state))
  , timestamp_(s.source_timestamp_)
  , seq_n_(s.sequence_)
  , data_(s.registered_data_)
{}

Observer::Sample::Sample(
  DDS::InstanceHandle_t i,
  CORBA::ULong instance_state,
  const DDS::Time_t& t,
  const SequenceNumber& sn,
  const void* data)
  : instance_(i)
  , instance_state_(instance_state)
  , timestamp_(t)
  , seq_n_(sn)
  , data_(data)
{}

void Observer::on_enabled(DDS::DataWriter_ptr) {}
void Observer::on_enabled(DDS::DataReader_ptr) {}
void Observer::on_deleted(DDS::DataWriter_ptr) {}
void Observer::on_deleted(DDS::DataReader_ptr) {}
void Observer::on_qos_changed(DDS::DataWriter_ptr) {}
void Observer::on_qos_changed(DDS::DataReader_ptr) {}

void Observer::on_associated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
void Observer::on_associated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}
void Observer::on_disassociated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
void Observer::on_disassociated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}

void Observer::on_sample_sent(const DDS::DataWriter_ptr, const Sample&) {}
void Observer::on_sample_received(const DDS::DataReader_ptr, const Sample&) {}
void Observer::on_sample_read(const DDS::DataReader_ptr, const Sample&) {}
void Observer::on_sample_taken(const DDS::DataReader_ptr, const Sample&) {}

Observer::~Observer() {}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
