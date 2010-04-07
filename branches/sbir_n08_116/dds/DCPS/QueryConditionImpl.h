/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QUERYCONDITIONIMPL_H
#define OPENDDS_DCPS_QUERYCONDITIONIMPL_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/ReadConditionImpl.h"
#include "dds/DCPS/FilterEvaluator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <vector>
#include <string>

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

class OpenDDS_Dcps_Export QueryConditionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::QueryCondition>
  , public virtual ReadConditionImpl {
public:
  QueryConditionImpl(DataReaderImpl* dr, DDS::SampleStateMask sample_states,
                     DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states,
                     const char* query_expression, const DDS::StringSeq& query_parameters)
  : ReadConditionImpl(dr, sample_states, view_states, instance_states)
  , query_expression_(query_expression)
  , query_parameters_(query_parameters)
  , evaluator_(query_expression, true) {}

  virtual ~QueryConditionImpl() {}

  char* get_query_expression()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_query_parameters(DDS::StringSeq& query_parameters)
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t set_query_parameters(
    const DDS::StringSeq& query_parameters)
  ACE_THROW_SPEC((CORBA::SystemException));

  std::vector<std::string> getOrderBys() const;

  bool hasFilter() const;

  template <typename Sample>
  bool filter(const Sample& s) const
  {
    return evaluator_.eval(s, query_parameters_);
  }

private:
  CORBA::String_var query_expression_;
  DDS::StringSeq query_parameters_;
  FilterEvaluator evaluator_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#endif
