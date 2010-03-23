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
#include "QueryConditionImpl.h"

namespace OpenDDS {
namespace DCPS {

char* QueryConditionImpl::get_query_expression()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return CORBA::string_dup(query_expression_);
}

DDS::ReturnCode_t
QueryConditionImpl::get_query_parameters(DDS::StringSeq& query_parameters)
ACE_THROW_SPEC((CORBA::SystemException))
{
  query_parameters = query_parameters_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
QueryConditionImpl::set_query_parameters(const DDS::StringSeq& query_parameters)
ACE_THROW_SPEC((CORBA::SystemException))
{
  query_parameters_ = query_parameters;
  return DDS::RETCODE_OK;
}

} // namespace DCPS
} // namespace OpenDDS

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
