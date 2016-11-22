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
  return parent_->contains_sample(sample_states_,
                                  view_states_, instance_states_);
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
  return DDS::DataReader::_duplicate(parent_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
