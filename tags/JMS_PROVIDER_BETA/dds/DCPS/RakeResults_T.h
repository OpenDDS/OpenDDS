// -*- C++ -*-
#ifndef RAKERESULTS_H
#define RAKERESULTS_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsSubscriptionC.h"
#include "RakeData.h"

#include <vector>
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#  include <set>
#  include "Comparator_T.h" //TODO: remove once the generated code includes it
#endif

namespace OpenDDS
{
  namespace DCPS
  {

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    //TODO: this can be removed once code-generation is done
    template <class Sample>
    ComparatorBase::Ptr create_qc_comparator(Sample*, const char* field,
                                             ComparatorBase::Ptr next)
    {
      return 0;
    }

#endif

    enum Operation_t { DDS_OPERATION_READ, DDS_OPERATION_TAKE };

    /// Rake is an abbreviation for "read or take".  This class manages the
    /// results from a read() or take() operation, which are the received_data
    /// and the info_seq sequences passed in by-reference from the user.
    template <class SampleSeq>
    class RakeResults
    {
    public:
      RakeResults(SampleSeq& received_data, ::DDS::SampleInfoSeq& info_seq,
                  ::CORBA::Long max_samples, ::DDS::QueryCondition_ptr cond,
                  Operation_t oper);

      /// Returns false if the sample will definitely not be part of the
      /// resulting dataset, however if this returns true it still may be
      /// excluded (due to sorting and max_samples).
      bool insert_sample(ReceivedDataElement* sample, SubscriptionInstance* i,
                         size_t index_in_instance);

      bool copy_to_user();

    private:
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
      bool where_filter(typename SampleSeq::value_type*) const
      { return true; } //FUTURE: implement
#endif

      template <class FwdIter>
      bool copy_into(FwdIter begin, FwdIter end,
                     typename SampleSeq::PrivateMemberAccess& received_data_p);

      RakeResults(const RakeResults&); // no copy construction
      RakeResults& operator=(const RakeResults&); // no assignment

      SampleSeq& received_data_;
      ::DDS::SampleInfoSeq& info_seq_;
      ::CORBA::ULong max_samples_;
      ::DDS::QueryCondition_ptr cond_;
      Operation_t oper_;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

      class QueryConditionCmp
      {
      public:
        bool operator()(const RakeData& lhs, const RakeData& rhs) const
        {
          if (!cmp_.in())
            {
              return lhs.rde_->registered_data_ < rhs.rde_->registered_data_;
            }
          return cmp_->compare(lhs.rde_->registered_data_, 
                               rhs.rde_->registered_data_);
        }

        explicit QueryConditionCmp(ComparatorBase::Ptr cmp = 0) : cmp_(cmp) {}

      private:
        ComparatorBase::Ptr cmp_;
      };

      bool do_sort_;
      typedef std::multiset<RakeData, QueryConditionCmp> SortedSet;
      SortedSet sorted_;
#endif
      std::vector<RakeData> unsorted_;

      // data strucutres used by copy_into()
      typedef std::vector<CORBA::ULong> IndexList;
      struct InstanceData
      {
        bool most_recent_generation_;
        size_t MRSIC_index_;
        IndexList sampleinfo_positions_;
        CORBA::Long MRSIC_disposed_gc_, MRSIC_nowriters_gc_,
          MRS_disposed_gc_, MRS_nowriters_gc_;
        InstanceData() : most_recent_generation_(false), MRSIC_index_(0) {}
      };
    };

  }
}

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/RakeResults_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#include /**/ "ace/post.h"

#endif /* RAKERESULTS_H  */
