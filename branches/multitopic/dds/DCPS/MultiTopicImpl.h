/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICIMPL_H
#define OPENDDS_DCPS_MULTITOPICIMPL_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DCPS/TopicDescriptionImpl.h"
#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MultiTopicImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::MultiTopic>
  , public virtual TopicDescriptionImpl {
public:
  MultiTopicImpl(const char* name, const char* type_name,
    const char* subscription_expression,
    const DDS::StringSeq& expression_parameters,
    DomainParticipantImpl* participant);

  virtual ~MultiTopicImpl() {}

  char* get_subscription_expression()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_expression_parameters(DDS::StringSeq& parameters)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t set_expression_parameters(const DDS::StringSeq& parameters)
    ACE_THROW_SPEC((CORBA::SystemException));

private:
  std::string subscription_expression_;
  DDS::StringSeq expression_parameters_;

  ///concurrent access to expression_parameters_
  mutable ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#endif
