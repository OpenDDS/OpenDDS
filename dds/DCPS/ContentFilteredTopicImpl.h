/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONTENTFILTEREDTOPICIMPL_H
#define OPENDDS_DCPS_CONTENTFILTEREDTOPICIMPL_H

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC

#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/FilterEvaluator.h"
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

class OpenDDS_Dcps_Export ContentFilteredTopicImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::ContentFilteredTopic>
  , public virtual TopicDescriptionImpl {
public:
  ContentFilteredTopicImpl(const char* name, DDS::Topic_ptr related_topic,
    const char* filter_expression, const DDS::StringSeq& expression_parameters,
    DomainParticipantImpl* participant);

  virtual ~ContentFilteredTopicImpl() {}

  char* get_filter_expression();

  DDS::ReturnCode_t get_expression_parameters(DDS::StringSeq& parameters);

  DDS::ReturnCode_t set_expression_parameters(const DDS::StringSeq& parameters);

  DDS::Topic_ptr get_related_topic();

  template<typename Sample>
  bool filter(const Sample& s) const
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
    return filter_eval_.eval(s, expression_parameters_);
  }

  void add_reader(DataReaderImpl& reader);
  void remove_reader(DataReaderImpl& reader);

  const char* get_filter_class_name () const
  {
    return filter_eval_.usesExtendedGrammar () ? "OPENDDSSQL" : "DDSSQL";
  }

private:
  OPENDDS_STRING filter_expression_;
  FilterEvaluator filter_eval_;
  DDS::StringSeq expression_parameters_;
  DDS::Topic_var related_topic_;
  OPENDDS_VECTOR(DataReaderImpl*) readers_;

  /// Concurrent access to expression_parameters_ and readers_
  mutable ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_FILTERED_TOPIC

#endif
