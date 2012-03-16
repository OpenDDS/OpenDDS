/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RAKERESULTS_H
#define RAKERESULTS_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsSubscriptionC.h"
#include "RakeData.h"
#include "Comparator_T.h"

#include <vector>
#include <set>

namespace OpenDDS {
namespace DCPS {

enum Operation_t { DDS_OPERATION_READ, DDS_OPERATION_TAKE };

/// Rake is an abbreviation for "read or take".  This class manages the
/// results from a read() or take() operation, which are the received_data
/// and the info_seq sequences passed in by-reference from the user.
template <class SampleSeq>
class RakeResults {
public:
  RakeResults(DataReaderImpl* reader,
              SampleSeq& received_data,
              DDS::SampleInfoSeq& info_seq,
              CORBA::Long max_samples,
              DDS::PresentationQosPolicy presentation,
              DDS::QueryCondition_ptr cond,
              Operation_t oper);

  /// Returns false if the sample will definitely not be part of the
  /// resulting dataset, however if this returns true it still may be
  /// excluded (due to sorting and max_samples).
  bool insert_sample(ReceivedDataElement* sample, SubscriptionInstance* i,
                     size_t index_in_instance);

  bool copy_to_user();

private:
  template <class FwdIter>
  bool copy_into(FwdIter begin, FwdIter end,
                 typename SampleSeq::PrivateMemberAccess& received_data_p);

  RakeResults(const RakeResults&); // no copy construction
  RakeResults& operator=(const RakeResults&); // no assignment

  DataReaderImpl* reader_;
  SampleSeq& received_data_;
  DDS::SampleInfoSeq& info_seq_;
  CORBA::ULong max_samples_;
  DDS::QueryCondition_ptr cond_;
  Operation_t oper_;

  class SortedSetCmp {
  public:
    bool operator()(const RakeData& lhs, const RakeData& rhs) const {
      if (!cmp_.in()) {
        // The following assumes that if no comparator is set
        // then PRESENTATION ordered access applies (TOPIC).
        return lhs.rde_->source_timestamp_ < rhs.rde_->source_timestamp_;
      }

      return cmp_->compare(lhs.rde_->registered_data_,
                           rhs.rde_->registered_data_);
    }

    explicit SortedSetCmp(ComparatorBase::Ptr cmp = 0) : cmp_(cmp) {}

  private:
    ComparatorBase::Ptr cmp_;
  };

  bool do_sort_, do_filter_;
  typedef std::multiset<RakeData, SortedSetCmp> SortedSet;

  // Contains data for QueryCondition/Ordered access
  SortedSet sorted_;

  // Contains data for all other use cases
  std::vector<RakeData> unsorted_;

  // data structures used by copy_into()
  typedef std::vector<CORBA::ULong> IndexList;
  struct InstanceData {
    bool most_recent_generation_;
    size_t MRSIC_index_;
    IndexList sampleinfo_positions_;
    CORBA::Long MRSIC_disposed_gc_, MRSIC_nowriters_gc_,
    MRS_disposed_gc_, MRS_nowriters_gc_;
    InstanceData() : most_recent_generation_(false), MRSIC_index_(0),
      MRSIC_disposed_gc_(0), MRSIC_nowriters_gc_(0), MRS_disposed_gc_(0),
      MRS_nowriters_gc_(0) {}
  };
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/RakeResults_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#include /**/ "ace/post.h"

#endif /* RAKERESULTS_H  */
