/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_READCONDITIONIMPL_H
#define OPENDDS_DCPS_READCONDITIONIMPL_H

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/ConditionImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

class ReadConditionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::ReadCondition>
  , public virtual ConditionImpl {
public:
  ReadConditionImpl(DataReaderImpl* dr, DDS::SampleStateMask sample_states,
                    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
  : parent_(dr)
  , sample_states_(sample_states)
  , view_states_(view_states)
  , instance_states_(instance_states) {}

  virtual ~ReadConditionImpl() {}

  CORBA::Boolean get_trigger_value();

  DDS::SampleStateMask get_sample_state_mask();

  DDS::ViewStateMask get_view_state_mask();

  DDS::InstanceStateMask get_instance_state_mask();

  DDS::DataReader_ptr get_datareader();

protected:
  DataReaderImpl* parent_;
  DDS::SampleStateMask sample_states_;
  DDS::ViewStateMask view_states_;
  DDS::InstanceStateMask instance_states_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
