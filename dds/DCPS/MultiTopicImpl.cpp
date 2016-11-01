/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_MULTI_TOPIC
#include "MultiTopicImpl.h"
#include "Registered_Data_Types.h"
#include "DomainParticipantImpl.h"
#include "TopicExpressionGrammar.h"
#include "FilterEvaluator.h"
#include "AstNodeWrapper.h"

#include <stdexcept>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  TypeSupport_ptr findTypeSupport(DomainParticipantImpl* participant,
    const char* type_name)
  {
    TypeSupport_var p = Registered_Data_Types->lookup(participant, type_name);
    if (!p) {
      throw std::runtime_error(OPENDDS_STRING("Data type: ") + type_name +
        " is not registered.");
    }
    return p;
  }
}

MultiTopicImpl::MultiTopicImpl(const char* name,
  const char* type_name, const char* subscription_expression,
  const DDS::StringSeq& expression_parameters,
  DomainParticipantImpl* participant)
  : TopicDescriptionImpl(name, type_name,
      findTypeSupport(participant, type_name),
      participant)
  , subscription_expression_(subscription_expression)
  , expression_parameters_(expression_parameters)
  , filter_eval_(NULL)
{
  const char* out = subscription_expression
    + std::strlen(subscription_expression);
  yard::SimpleTextParser parser(subscription_expression, out);
  if (!parser.Parse<TopicExpressionGrammar::TopicCompleteInput>()) {
    reportErrors(parser, subscription_expression);
  }

  for (AstNode* iter = parser.GetAstRoot()->GetFirstChild(); iter;
      iter = iter->GetSibling()) {
    if (iter->TypeMatches<TopicExpressionGrammar::SubjectFieldSpec>()) {
      AstNode* fieldName = iter->GetFirstChild();
      aggregation_.push_back(SubjectFieldSpec(toString(fieldName),
        toString(fieldName->GetSibling())));
    } else if (iter->TypeMatches<TopicExpressionGrammar::TopicName>()) {
      selection_.push_back(toString(iter));
    } else {
      filter_eval_ = new FilterEvaluator(iter);
    }
  }
}

MultiTopicImpl::~MultiTopicImpl()
{
  delete filter_eval_;
}

char* MultiTopicImpl::get_subscription_expression()
{
  return CORBA::string_dup(subscription_expression_.c_str());
}

DDS::ReturnCode_t
MultiTopicImpl::get_expression_parameters(DDS::StringSeq& params)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_,
    DDS::RETCODE_OUT_OF_RESOURCES);
  params = expression_parameters_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
MultiTopicImpl::set_expression_parameters(const DDS::StringSeq& p)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_,
    DDS::RETCODE_OUT_OF_RESOURCES);
  expression_parameters_ = p;
  return DDS::RETCODE_OK;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_MULTI_TOPIC
