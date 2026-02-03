/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReadConditionImpl.h"
#include "DataReaderImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

CORBA::Boolean ReadConditionImpl::get_trigger_value()
{
  RcHandle<DataReaderImpl> parent = parent_.lock();
  return parent ? parent->contains_sample(sample_states_,
                                  view_states_, instance_states_) : false;
}

DDS::SampleStateMask ReadConditionImpl::get_sample_state_mask()
{
  return sample_states_;
}

DDS::ViewStateMask ReadConditionImpl::get_view_state_mask()
{
  return view_states_;
}

DDS::InstanceStateMask ReadConditionImpl::get_instance_state_mask()
{
  return instance_states_;
}

DDS::DataReader_ptr ReadConditionImpl::get_datareader()
{
  RcHandle<DataReaderImpl> parent = parent_.lock();
  return parent ? DDS::DataReader::_duplicate(parent.get()) : 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
