/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_QUERY_CONDITION
#include "QueryConditionImpl.h"
#include "DataReaderImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

char* QueryConditionImpl::get_query_expression()
{
  return CORBA::string_dup(query_expression_);
}

DDS::ReturnCode_t
QueryConditionImpl::get_query_parameters(DDS::StringSeq& query_parameters)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  query_parameters = query_parameters_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
QueryConditionImpl::set_query_parameters(const DDS::StringSeq& query_parameters)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  query_parameters_ = query_parameters;
  return DDS::RETCODE_OK;
}

std::vector<OPENDDS_STRING>
QueryConditionImpl::getOrderBys() const
{
  return evaluator_.getOrderBys();
}

bool
QueryConditionImpl::hasFilter() const
{
  return evaluator_.hasFilter();
}

CORBA::Boolean
QueryConditionImpl::get_trigger_value()
{
  if (hasFilter()) {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard2, parent_->sample_lock_, false);
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
    return parent_->contains_sample_filtered(sample_states_, view_states_,
      instance_states_, evaluator_, query_parameters_);
  } else {
    return ReadConditionImpl::get_trigger_value();
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_QUERY_CONDITION
