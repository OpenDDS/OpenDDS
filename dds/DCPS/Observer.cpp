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
#include "transport/framework/ReceivedDataSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Observer::Sample::Sample(DDS::InstanceHandle_t a_instance,
                         DDS::InstanceStateKind a_instance_state,
                         const DDS::Time_t& a_timestamp,
                         const SequenceNumber& a_sequence_number,
                         const void* a_data,
                         const ValueWriterDispatcher& a_data_dispatcher)
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
                         const ValueWriterDispatcher& a_data_dispatcher)
  : instance(a_instance)
  , instance_state(a_instance_state)
  , timestamp(a_rde.source_timestamp_)
  , sequence_number(a_rde.sequence_)
  , data(a_rde.registered_data_)
  , data_dispatcher(a_data_dispatcher)
{}

Observer::~Observer() {}

void
vwrite(ValueWriter& vw, const Observer::Sample& sample)
{
  vw.begin_struct();
  vw.begin_struct_member("instance");
  vw.write_int32(sample.instance);
  vw.end_struct_member();
  vw.begin_struct_member("instance_state");
  vw.write_uint32(sample.instance_state);
  vw.end_struct_member();
  vw.begin_struct_member("timestamp");
  vwrite(vw, sample.timestamp);
  vw.end_struct_member();
  vw.begin_struct_member("sequence_number");
  vw.write_int64(sample.sequence_number.getValue());
  vw.end_struct_member();
  vw.begin_struct_member("data");
  sample.data_dispatcher.write(vw, sample.data);
  vw.end_struct_member();
  vw.end_struct();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
