/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#include "ContentFilteredTopicImpl.h"

namespace OpenDDS {
namespace DCPS {

ContentFilteredTopicImpl::ContentFilteredTopicImpl(const char* name,
  DDS::Topic_ptr related_topic, const char* filter_expression,
  const DDS::StringSeq& expression_parameters,
  DomainParticipantImpl* participant)
  : TopicDescriptionImpl(name, related_topic->get_type_name(),
      dynamic_cast<TopicDescriptionImpl*>(related_topic)->get_type_support(),
      participant)
  , filter_eval_(filter_expression, false /*allowOrderBy*/)
  , expression_parameters_(expression_parameters)
  , related_topic_(DDS::Topic::_duplicate(related_topic))
{}

char* ContentFilteredTopicImpl::get_filter_expression()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return CORBA::string_dup(filter_eval_.getFilterString());
}

DDS::ReturnCode_t
ContentFilteredTopicImpl::get_expression_parameters(DDS::StringSeq& params)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  params = expression_parameters_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
ContentFilteredTopicImpl::set_expression_parameters(const DDS::StringSeq& p)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
  expression_parameters_ = p;
  return DDS::RETCODE_OK;
}

DDS::Topic_ptr
ContentFilteredTopicImpl::get_related_topic()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::Topic::_duplicate(related_topic_);
}

} // namespace DCPS
} // namespace OpenDDS

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
