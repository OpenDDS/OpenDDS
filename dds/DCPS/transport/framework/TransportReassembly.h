/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTREASSEMBLY_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTREASSEMBLY_H

#include "ReceivedDataSample.h"

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportReassembly {
public:

  /// Called by TransportReceiveStrategy if the fragmentation header flag
  /// is set.  Returns true/false to indicate if data should be delivered to
  /// the datalink.  The 'data' argument may be modified by this method.
  bool reassemble(const SequenceNumber& transportSeq, bool firstFrag,
                  ReceivedDataSample& data, ACE_UINT32 total_frags = 0);

  bool reassemble(const SequenceRange& seqRange, ReceivedDataSample& data, ACE_UINT32 total_frags = 0);

  /// Called by TransportReceiveStrategy to indicate that we can
  /// stop tracking partially-reassembled messages when we know the
  /// remaining fragments are not expected to arrive.
  void data_unavailable(const SequenceRange& transportSeqDropped);

  void data_unavailable(const SequenceNumber& dataSampleSeq,
                        const RepoId& pub_id);

  /// Clears out "completed" sequence numbers in order to allow resends for
  /// new associations to the same (given) publication id
  void clear_completed(const RepoId& pub_id);

  /// Returns true if this object is storing fragments for the given
  /// DataSampleHeader sequence number from the given publication.
  bool has_frags(const SequenceNumber& seq, const RepoId& pub_id) const;

  /// Populates bitmap for missing fragment sequence numbers and set numBits
  /// for the given message sequence and publisher ID.
  /// @returns the base fragment sequence number for bit zero in the bitmap
  CORBA::ULong get_gaps(const SequenceNumber& msg_seq, const RepoId& pub_id,
                        CORBA::Long bitmap[], CORBA::ULong length,
                        CORBA::ULong& numBits) const;

private:

  bool reassemble_i(const SequenceRange& seqRange, bool firstFrag,
                    ReceivedDataSample& data, ACE_UINT32 total_frags);

  // A FragKey represents the identifier for an original (pre-fragmentation)
  // message.  Since DataSampleHeader sequence numbers are distinct for each
  // "publication" (DataWriter), the partially-received messages need to be
  // stored in a structure that's keyed off of both the PublicationId and the
  // SequenceNumber.
  struct FragKey {
    FragKey(const PublicationId& pubId, const SequenceNumber& dataSampleSeq);

    bool operator<(const FragKey& rhs) const
    {
      if (compare_(this->publication_, rhs.publication_)) return true;
      if (compare_(rhs.publication_, this->publication_)) return false;
      return this->data_sample_seq_ < rhs.data_sample_seq_;
    }

    bool operator==(const FragKey& other) const
    {
      return publication_ == other.publication_ && data_sample_seq_ == other.data_sample_seq_;
    }

    bool operator!=(const FragKey& other) const
    {
      return !(*this == other);
    }

    static GUID_tKeyLessThan compare_;
    PublicationId publication_;
    SequenceNumber data_sample_seq_;
  };

  // A FragRange represents a chunk of a partially-reassembled message.
  // The transport_seq_ range is the range of transport sequence numbers
  // that were used to send the given chunk of data.
  struct FragRange {
    FragRange(const SequenceRange& seqRange,
              const ReceivedDataSample& data);

    SequenceRange transport_seq_;
    ReceivedDataSample rec_ds_;
  };

  // Each element of the FragRangeList represents one sent message
  // (one DataSampleHeader before fragmentation).  The list must have at
  // least one value in it.  If a FragRange in the list has a sample_ with
  // a null ACE_Message_Block*, it's one that was data_unavailable().
  typedef OPENDDS_LIST(FragRange) FragRangeList;

  struct FragInfo {
    FragInfo()
      : have_first_(false), range_list_(), total_frags_(0) {}
    FragInfo(bool hf, const FragRangeList& rl, ACE_UINT32 tf, const MonotonicTimePoint& expiration)
      : have_first_(hf), range_list_(rl), total_frags_(tf), expiration_(expiration) {}

    bool have_first_;
    FragRangeList range_list_;
    ACE_UINT32 total_frags_;
    MonotonicTimePoint expiration_;
  };

  typedef OPENDDS_MAP(FragKey, FragInfo) FragInfoMap;
  FragInfoMap fragments_;

  typedef OPENDDS_MULTIMAP(MonotonicTimePoint, FragKey) ExpirationQueue;
  ExpirationQueue expiration_queue_;

  typedef OPENDDS_MAP_CMP(PublicationId, DisjointSequence, GUID_tKeyLessThan) CompletedMap;
  CompletedMap completed_;

  void erase_i(FragInfoMap::iterator pos);

  static bool insert(OPENDDS_LIST(FragRange)& flist,
                     const SequenceRange& seqRange,
                     ReceivedDataSample& data);
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
