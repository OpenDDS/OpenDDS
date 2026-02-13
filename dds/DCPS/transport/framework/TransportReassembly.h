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
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#pragma pack(push, 1)

// A FragKey represents the identifier for an original (pre-fragmentation)
// message.  Since DataSampleHeader sequence numbers are distinct for each
// "publication" (DataWriter), the partially-received messages need to be
// stored in a structure that's keyed off of both the GUID_t and the
// SequenceNumber.
struct FragKey {
  FragKey(const GUID_t& pubId, const SequenceNumber& dataSampleSeq);

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
  GUID_t publication_;
  SequenceNumber data_sample_seq_;
};

#pragma pack(pop)

#if defined ACE_HAS_CPP11
  OPENDDS_OOAT_CUSTOM_HASH(FragKey, OpenDDS_Dcps_Export, FragKeyHash);
#endif

typedef std::pair<FragmentNumber, FragmentNumber> FragmentRange;

class OpenDDS_Dcps_Export TransportReassembly : public RcObject {
public:
  explicit TransportReassembly(const TimeDuration& timeout = TimeDuration(300));

  /// Called by TransportReceiveStrategy if the fragmentation header flag
  /// is set.  Returns true/false to indicate if data should be delivered to
  /// the datalink.  The 'data' argument may be modified by this method.
  bool reassemble(const SequenceNumber& transportSeq, bool firstFrag,
                  ReceivedDataSample& data, ACE_UINT32 total_frags = 0);

  bool reassemble(const FragmentRange& fragRange, ReceivedDataSample& data, ACE_UINT32 total_frags = 0);

  /// Called by TransportReceiveStrategy to indicate that we can
  /// stop tracking partially-reassembled messages when we know the
  /// remaining fragments are not expected to arrive.
  void data_unavailable(const FragmentRange& transportSeqDropped);

  void data_unavailable(const SequenceNumber& dataSampleSeq,
                        const GUID_t& pub_id);

  /// Clears out "completed" sequence numbers in order to allow resends for
  /// new associations to the same (given) publication id
  void clear_completed(const GUID_t& pub_id);

  /// Returns true if this object is storing fragments for the given
  /// DataSampleHeader sequence number from the given publication.
  bool has_frags(const SequenceNumber& seq, const GUID_t& pub_id) const
  {
    ACE_UINT32 total_frags;
    return has_frags(seq, pub_id, total_frags);
  }

  /// Returns true if this object is storing fragments for the given
  /// DataSampleHeader sequence number from the given publication.
  bool has_frags(const SequenceNumber& seq, const GUID_t& pub_id, ACE_UINT32& total_frags) const;

  /// Populates bitmap for missing fragment sequence numbers and set numBits
  /// for the given message sequence and publisher ID.
  /// @returns the base fragment sequence number for bit zero in the bitmap
  CORBA::ULong get_gaps(const SequenceNumber& msg_seq, const GUID_t& pub_id,
                        CORBA::Long bitmap[], CORBA::ULong length,
                        CORBA::ULong& numBits) const;

  size_t fragments_size() const { return fragments_.size(); }
  size_t queue_size() const { return expiration_queue_.size(); }
  size_t completed_size() const { return completed_.size(); }
  size_t total_frags() const;

private:

  bool reassemble_i(const FragmentRange& fragRange, bool firstFrag,
                    ReceivedDataSample& data, ACE_UINT32 total_frags);

  // A FragSample represents a chunk of a partially-reassembled message.
  // The frag_range_ range is the range of transport sequence numbers
  // that were used to send the given chunk of data.
  struct FragSample {
    FragSample(const FragmentRange& fragRange,
              const ReceivedDataSample& data);

#ifdef ACE_HAS_CPP11
    FragSample(const FragmentRange& fragRange,
              ReceivedDataSample&& data);
    FragSample(const FragSample&) = default;
    FragSample(FragSample&&) = default;

    FragSample& operator=(const FragSample&) = default;
    FragSample& operator=(FragSample&&) = default;
#endif

    FragmentRange frag_range_;
    ReceivedDataSample rec_ds_;
  };

  struct FragInfo {
    typedef OPENDDS_LIST(FragSample) FragSampleList;
    typedef OPENDDS_MAP(FragmentNumber, FragSampleList::iterator) FragSampleListIterMap;

    typedef OPENDDS_LIST(FragmentRange) FragGapList;
    typedef OPENDDS_MAP(FragmentNumber, FragGapList::iterator) FragGapListIterMap;

    FragInfo();
    FragInfo(bool hf, const FragSampleList& rl, ACE_UINT32 tf, const MonotonicTimePoint& expiration);
    FragInfo(const FragInfo& val);

    FragInfo& operator=(const FragInfo& rhs);

#ifdef ACE_HAS_CPP11
    FragInfo(FragInfo&& other) = default;
    FragInfo& operator=(FragInfo&& rhs) = default;
#endif

    bool insert(const FragmentRange& fragRange, ReceivedDataSample& data);

    bool have_first_;
    FragSampleList sample_list_;
    FragSampleListIterMap sample_finder_;
    FragGapList gap_list_;
    FragGapListIterMap gap_finder_;
    ACE_UINT32 total_frags_;
    MonotonicTimePoint expiration_;
  };

  mutable ACE_Thread_Mutex mutex_;

#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP_CHASH(FragKey, FragInfo, FragKeyHash) FragInfoMap;
#else
  typedef OPENDDS_MAP(FragKey, FragInfo) FragInfoMap;
#endif
  FragInfoMap fragments_;

  typedef std::pair<MonotonicTimePoint, FragKey> ElementType;
  typedef OPENDDS_LIST(ElementType) ExpirationQueue;
  ExpirationQueue expiration_queue_;

  typedef OPENDDS_MAP_CMP(GUID_t, DisjointSequence, GUID_tKeyLessThan) CompletedMap;
  CompletedMap completed_;

  TimeDuration timeout_;

  void check_expirations(const MonotonicTimePoint& now);
};

typedef RcHandle<TransportReassembly> TransportReassembly_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
