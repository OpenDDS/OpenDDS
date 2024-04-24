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
#include "XTypes/MemberDescriptorImpl.h"
#include "transport/framework/ReceivedDataSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Observer::Sample::Sample(DDS::InstanceHandle_t a_instance,
                         DDS::InstanceStateKind a_instance_state,
                         const DDS::Time_t& a_timestamp,
                         const SequenceNumber& a_sequence_number,
                         const void* a_data,
                         const ValueDispatcher& a_data_dispatcher)
  : instance(a_instance)
  , instance_state(a_instance_state)
  , timestamp(a_timestamp)
  , sequence_number(a_sequence_number)
  , data(a_data)
  , data_dispatcher(a_data_dispatcher)
{}

Observer::Sample::Sample(DDS::InstanceHandle_t a_instance,
                         DDS::InstanceStateKind a_instance_state,
                         const ReceivedDataElement& a_rde,
                         const ValueDispatcher& a_data_dispatcher)
  : instance(a_instance)
  , instance_state(a_instance_state)
  , timestamp(a_rde.source_timestamp_)
  , sequence_number(a_rde.sequence_)
  , data(a_rde.registered_data_)
  , data_dispatcher(a_data_dispatcher)
{}

Observer::~Observer() {}

bool
vwrite(ValueWriter& vw, const Observer::Sample& sample)
{
  if (!vw.begin_struct(FINAL)) { return false; }
  if (!vw.begin_struct_member(MemberParam("instance"))) { return false; }
  if (!vw.write_int32(sample.instance)) { return false; }
  if (!vw.end_struct_member()) { return false; }
  if (!vw.begin_struct_member(MemberParam("instance_state"))) { return false; }
  if (!vw.write_uint32(sample.instance_state)) { return false; }
  if (!vw.end_struct_member()) { return false; }
  if (!vw.begin_struct_member(MemberParam("timestamp"))) { return false; }
  if (!vwrite(vw, sample.timestamp)) { return false; }
  if (!vw.end_struct_member()) { return false; }
  if (!vw.begin_struct_member(MemberParam("sequence_number"))) { return false; }
  if (!vw.write_int64(sample.sequence_number.getValue())) { return false; }
  if (!vw.end_struct_member()) { return false; }
  if (!vw.begin_struct_member(MemberParam("data"))) { return false; }
  if (!sample.data_dispatcher.write(vw, sample.data)) { return false; }
  if (!vw.end_struct_member()) { return false; }
  if (!vw.end_struct()) { return false; }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
