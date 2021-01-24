/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONTENTFILTEREDTOPICIMPL_H
#define OPENDDS_DCPS_CONTENTFILTEREDTOPICIMPL_H

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC

#include "TopicDescriptionImpl.h"
#include "FilterEvaluator.h"
#include "PoolAllocator.h"

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
    const char* filter_expression, DomainParticipantImpl* participant);

  virtual ~ContentFilteredTopicImpl() {}

  char* get_filter_expression();

  DDS::ReturnCode_t get_expression_parameters(DDS::StringSeq& parameters);

  DDS::ReturnCode_t set_expression_parameters(const DDS::StringSeq& parameters);

  DDS::Topic_ptr get_related_topic();

  /**
   * Returns true if the sample matches the filter.
   */
  template<typename Sample>
  bool filter(const Sample& s, bool sample_only_has_key_fields) const
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
    const MetaStruct& meta = getMetaStruct<Sample>();
    /*
     * Omit the sample from results if the filter references non-key fields
     * and the sample only has key fields.
     */
    if (sample_only_has_key_fields && filter_eval_.has_non_key_fields(meta)) {
      return false;
    }
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
  typedef OPENDDS_VECTOR(WeakRcHandle<DataReaderImpl>) Readers;
  Readers readers_;

  /// Concurrent access to expression_parameters_ and readers_
  mutable ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_FILTERED_TOPIC

#endif
