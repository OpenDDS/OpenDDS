/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICIMPL_H
#define OPENDDS_DCPS_MULTITOPICIMPL_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/FilterEvaluator.h"

#include <string>
#include <vector>
#include <utility>

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

  virtual ~MultiTopicImpl();

  char* get_subscription_expression();

  DDS::ReturnCode_t get_expression_parameters(DDS::StringSeq& parameters);

  DDS::ReturnCode_t set_expression_parameters(const DDS::StringSeq& parameters);

  struct SubjectFieldSpec {
    std::string incoming_name_;
    std::string resulting_name_;

    explicit SubjectFieldSpec(const std::string& inc,
                              const std::string& res = "")
      : incoming_name_(inc)
      , resulting_name_(res == "" ? inc : res)
    {}
  };

  const std::vector<SubjectFieldSpec>& get_aggregation() const
  {
    return aggregation_;
  }

  const std::vector<std::string>& get_selection() const
  {
    return selection_;
  }

  template<typename Sample>
  bool filter(const Sample& s) const
  {
    if (!filter_eval_) return true;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
    return filter_eval_->eval(s, expression_parameters_);
  }

private:
  std::string subscription_expression_;
  DDS::StringSeq expression_parameters_;
  FilterEvaluator* filter_eval_;

  std::vector<SubjectFieldSpec> aggregation_;
  std::vector<std::string> selection_;

  ///concurrent access to expression_parameters_
  mutable ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#endif
