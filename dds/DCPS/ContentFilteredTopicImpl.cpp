/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
#include "ContentFilteredTopicImpl.h"
#include "DataReaderImpl.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ContentFilteredTopicImpl::ContentFilteredTopicImpl(const char* name,
  DDS::Topic_ptr related_topic, const char* filter_expression,
  const DDS::StringSeq& expression_parameters,
  DomainParticipantImpl* participant)
  : TopicDescriptionImpl(name,
      CORBA::String_var(related_topic->get_type_name()),
      dynamic_cast<TopicDescriptionImpl*>(related_topic)->get_type_support(),
      participant)
  , filter_expression_(filter_expression)
  , filter_eval_(filter_expression, false /*allowOrderBy*/)
  , expression_parameters_(expression_parameters)
  , related_topic_(DDS::Topic::_duplicate(related_topic))
{}

char* ContentFilteredTopicImpl::get_filter_expression()
{
  return CORBA::string_dup(filter_expression_.c_str());
}

DDS::ReturnCode_t
ContentFilteredTopicImpl::get_expression_parameters(DDS::StringSeq& params)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_,
    DDS::RETCODE_OUT_OF_RESOURCES);
  params = expression_parameters_;
  return DDS::RETCODE_OK;
}

namespace {
  bool string_equal(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
  }
}

DDS::ReturnCode_t
ContentFilteredTopicImpl::set_expression_parameters(const DDS::StringSeq& p)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_,
    DDS::RETCODE_OUT_OF_RESOURCES);

  const CORBA::ULong len = p.length();
  if (len == expression_parameters_.length()) {
    const char* const* p_buf = p.get_buffer();
    char* const* e_buf = expression_parameters_.get_buffer();
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
    if (std::equal(&p_buf[0], &p_buf[len], &e_buf[0], string_equal)) {
      // no change, bail out now to avoid remote InfoRepo calls
      return DDS::RETCODE_OK;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  }

  expression_parameters_ = p;

  for (std::vector<DataReaderImpl*>::iterator iter = readers_.begin(),
       end = readers_.end(); iter != end; ++iter) {
    (*iter)->update_subscription_params(p);
  }

  return DDS::RETCODE_OK;
}

DDS::Topic_ptr
ContentFilteredTopicImpl::get_related_topic()
{
  return DDS::Topic::_duplicate(related_topic_);
}

void
ContentFilteredTopicImpl::add_reader(DataReaderImpl& reader)
{
  // readers_ does not own or reference-count the reader because
  // the readers reference this CFT and this CFT can't be removed
  // until all readers are gone (DomainParticipant::delete_contentfilteredtopic)
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, lock_);
  readers_.push_back(&reader);
}

void
ContentFilteredTopicImpl::remove_reader(DataReaderImpl& reader)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, lock_);
  std::vector<DataReaderImpl*>::iterator end = readers_.end();
  readers_.erase(std::remove(readers_.begin(), end, &reader), end);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_FILTERED_TOPIC

