/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QUERYCONDITIONIMPL_H
#define OPENDDS_DCPS_QUERYCONDITIONIMPL_H

#ifndef OPENDDS_NO_QUERY_CONDITION

#include "dds/DdsDcpsSubscriptionC.h"
#include "ReadConditionImpl.h"
#include "FilterEvaluator.h"
#include "PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

class OpenDDS_Dcps_Export QueryConditionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::QueryCondition>
  , public virtual ReadConditionImpl {
public:
  QueryConditionImpl(DataReaderImpl* dr, DDS::SampleStateMask sample_states,
                     DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states,
                     const char* query_expression);

  virtual ~QueryConditionImpl() {}

  char* get_query_expression();

  DDS::ReturnCode_t get_query_parameters(DDS::StringSeq& query_parameters);

  DDS::ReturnCode_t set_query_parameters(
    const DDS::StringSeq& query_parameters);

  CORBA::Boolean get_trigger_value();

  std::vector<OPENDDS_STRING> getOrderBys() const;

  bool hasFilter() const;

  /**
   * Returns true if the sample matches the query.
   */
  template<typename Sample>
  bool filter(const Sample& s, bool sample_only_has_key_fields) const
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, false);
    const MetaStruct& meta = getMetaStruct<Sample>();
    /*
     * Omit the sample from results if the query references non-key fields
     * and the sample only has key fields.
     */
    if (sample_only_has_key_fields && evaluator_.has_non_key_fields(meta)) {
      if (DCPS_debug_level > 8) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) QueryConditionImpl::filter: ")
          ACE_TEXT("Sample has been filtered because the query ")
          ACE_TEXT("references fields that are not readable\n")
        ));
      }
      return false;
    }
    return evaluator_.eval(s, query_parameters_);
  }

private:
  CORBA::String_var query_expression_;
  DDS::StringSeq query_parameters_;
  FilterEvaluator evaluator_;
  /// Concurrent access to query_parameters_
  mutable ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_QUERY_CONDITION

#endif
