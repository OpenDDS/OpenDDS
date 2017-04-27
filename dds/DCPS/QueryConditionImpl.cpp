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

QueryConditionImpl::QueryConditionImpl(
  DataReaderImpl* dr, DDS::SampleStateMask sample_states,
  DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states,
  const char* query_expression)
  : ReadConditionImpl(dr, sample_states, view_states, instance_states)
  , query_expression_(query_expression)
  , evaluator_(query_expression, true)
{
  if (DCPS_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) QueryConditionImpl::QueryConditionImpl() - ")
      ACE_TEXT("Creating qc with query <%C> which requires <%d> parameters\n"),
      query_expression, evaluator_.number_parameters()));
  }
}

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

  // Check sequence of strings that give values to the ‘parameters’ (i.e., "%n" tokens)
  // in the query_expression matches the size of the parameter sequence.
  // The tokens start with 0 which means that when the maximum number used is 1 we need
  // two parameters, (zero and one)
  if (query_parameters.length() != evaluator_.number_parameters()) {
    if (DCPS_debug_level > 1) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) QueryConditionImpl::set_expression_parameters() - ")
        ACE_TEXT("passed incorrect set of query parameters, expected %d received %d\n"),
        evaluator_.number_parameters (), query_parameters.length()));
    }
    return DDS::RETCODE_ERROR;
  }

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
