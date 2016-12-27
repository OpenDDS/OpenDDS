/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef GROUPRAKEDATA_H
#define GROUPRAKEDATA_H

#include /**/ "ace/pre.h"
#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "RakeData.h"
#include "Comparator_T.h"

#include "PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Rake is an abbreviation for "read or take".  This class manages the
/// results from a read() or take() operation, which are the received_data
/// and the info_seq sequences passed in by-reference from the user.
class OpenDDS_Dcps_Export GroupRakeData {
public:
  GroupRakeData();

  /// Returns false if the sample will definitely not be part of the
  /// resulting dataset, however if this returns true it still may be
  /// excluded (due to sorting and max_samples).
  bool insert_sample(ReceivedDataElement* sample, SubscriptionInstance_rch i,
                     size_t index_in_instance);

  void get_datareaders (DDS::DataReaderSeq & readers);

  bool copy_to_user();

  void reset ();

  RakeData get_data ();

private:


  GroupRakeData(const GroupRakeData&); // no copy construction
  GroupRakeData& operator=(const GroupRakeData&); // no assignment

  class SortedSetCmp {
  public:
    bool operator()(const RakeData& lhs, const RakeData& rhs) const {
      if (!cmp_.in()) {
        // The following assumes that if no comparator is set
        // then BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS is used.
        return lhs.rde_->source_timestamp_ < rhs.rde_->source_timestamp_;
      }

      return false;
    }
    SortedSetCmp(){}
    explicit SortedSetCmp(ComparatorBase::Ptr cmp) : cmp_(cmp){}

  private:
    ComparatorBase::Ptr cmp_;
  };

  typedef OPENDDS_MULTISET_CMP(RakeData, SortedSetCmp) SortedSet;

  // Contains data for QueryCondition/Ordered access
  SortedSet sorted_;

  SortedSet::iterator current_sample_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* GROUPRAKEDATA_H  */
