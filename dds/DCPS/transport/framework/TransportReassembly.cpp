/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReassembly.h"
#include "TransportDebug.h"

#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/DisjointSequence.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

FragKey::FragKey(const GUID_t& pubId,
                 const SequenceNumber& dataSampleSeq)
  : publication_(pubId)
  , data_sample_seq_(dataSampleSeq)
{
}

GUID_tKeyLessThan FragKey::compare_;

TransportReassembly::FragSample::FragSample(const FragmentRange& fragRange,
                                            const ReceivedDataSample& data)
  : frag_range_(fragRange)
  , rec_ds_(data)
{
}

#ifdef ACE_HAS_CPP11
TransportReassembly::FragSample::FragSample(const FragmentRange& fragRange,
                                            ReceivedDataSample&& data)
  : frag_range_(fragRange)
  , rec_ds_(std::move(data))
{
}
#endif

TransportReassembly::TransportReassembly(const TimeDuration& timeout)
  : timeout_(timeout)
{
}

bool
TransportReassembly::has_frags(const SequenceNumber& seq,
                               const GUID_t& pub_id,
                               ACE_UINT32& total_frags) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  const FragInfoMap::const_iterator iter = fragments_.find(FragKey(pub_id, seq));
  if (iter != fragments_.end()) {
    total_frags = iter->second.total_frags_;
    return true;
  }
  return false;
}

void
TransportReassembly::clear_completed(const GUID_t& pub_id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  completed_.erase(pub_id);
}

CORBA::ULong
TransportReassembly::get_gaps(const SequenceNumber& seq, const GUID_t& pub_id,
                              CORBA::Long bitmap[], CORBA::ULong length,
                              CORBA::ULong& numBits) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  // length is number of (allocated) words in bitmap, max of 8
  // numBits is number of valid bits in the bitmap, <= length * 32, to account for partial words
  if (length == 0) {
    return 0;
  }

  const FragInfoMap::const_iterator iter = fragments_.find(FragKey(pub_id, seq));
  if (iter == fragments_.end()) {
    // Nothing missing
    return 0;
  }

  // RTPS's FragmentNumbers are 32-bit values, so we'll only be using the
  // low 32 bits of the 64-bit generalized sequence numbers in
  // FragSample::frag_range_.

  const OPENDDS_LIST(FragSample)& flist = iter->second.sample_list_;
  const FragmentNumber first = flist.front().frag_range_.first;
  const CORBA::ULong base = static_cast<CORBA::ULong>((first == 1)
    ? flist.front().frag_range_.second + 1
    : 1);

  if (first != 1) {
    // Represent the "gap" before the first list element.
    // base == 1 and the first 2 args to fill_bitmap_range() are deltas of base
    ACE_CDR::ULong bits_added = 0;
    DisjointSequence::fill_bitmap_range(0, static_cast<CORBA::ULong>(first - 2),
                                        bitmap, length, numBits, bits_added);
  } else if (flist.size() == 1) {
    // No gaps, but we know there are (at least 1) more_fragments
    if (iter->second.total_frags_ == 0) {
      ACE_CDR::ULong bits_added = 0;
      DisjointSequence::fill_bitmap_range(0, 0, bitmap, length, numBits, bits_added);
    } else {
      const size_t rlimit = static_cast<size_t>(flist.back().frag_range_.second - 1);
      const CORBA::ULong ulimit = static_cast<CORBA::ULong>(iter->second.total_frags_ - (base < rlimit ? rlimit : base));
      ACE_CDR::ULong bits_added = 0;
      DisjointSequence::fill_bitmap_range(0,
                                          ulimit,
                                          bitmap, length, numBits, bits_added);
    }
    // NOTE: this could send a nack for fragments that are in flight
    // need to defer setting bitmap till heartbeat extending logic
    // in RtpsUdpDataLink::generate_nack_frags
    return base;
  }

  typedef OPENDDS_LIST(FragSample)::const_iterator list_iterator;
  for (list_iterator it = flist.begin(); it != flist.end(); ++it) {
    const list_iterator it_next = ++list_iterator(it);
    if (it_next == flist.end()) {
      break;
    }
    const CORBA::ULong low = static_cast<CORBA::ULong>(it->frag_range_.second + 1 - base),
                       high = static_cast<CORBA::ULong>(it_next->frag_range_.first - 1 - base);
    ACE_CDR::ULong bits_added = 0;
    DisjointSequence::fill_bitmap_range(low, high, bitmap, length, numBits, bits_added);
  }

  return base;
}

bool
TransportReassembly::reassemble(const FragmentRange& fragRange,
                                ReceivedDataSample& data,
                                ACE_UINT32 total_frags)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return reassemble_i(fragRange, fragRange.first == 1, data, total_frags);
}

bool
TransportReassembly::reassemble(const SequenceNumber& transportSeq,
                                bool firstFrag,
                                ReceivedDataSample& data,
                                ACE_UINT32 total_frags)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return reassemble_i(FragmentRange(transportSeq.getValue(), transportSeq.getValue()),
                      firstFrag, data, total_frags);
}

bool
TransportReassembly::reassemble_i(const FragmentRange& fragRange,
                                  bool firstFrag,
                                  ReceivedDataSample& data,
                                  ACE_UINT32 total_frags)
{
  if (Transport_debug_level > 5) {
    LogGuid logger(data.header_.publication_id_);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TransportReassembly::reassemble_i: "
      "tseq %q-%q first %d dseq %q pub %C\n", fragRange.first,
      fragRange.second, firstFrag ? 1 : 0,
      data.header_.sequence_.getValue(), logger.c_str()));
  }

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  check_expirations(now);

  const FragKey key(data.header_.publication_id_, data.header_.sequence_);
  FragInfoMap::iterator iter = fragments_.find(key);
  const MonotonicTimePoint expiration = now + timeout_;

  if (iter == fragments_.end()) {
    FragInfo& finfo = fragments_[key];
    finfo = FragInfo(firstFrag, FragInfo::FragSampleList(), total_frags, expiration);
    finfo.insert(fragRange, data);
    expiration_queue_.push_back(std::make_pair(expiration, key));
    data.clear();
    // since this is the first fragment we've seen, it can't possibly be done
    if (Transport_debug_level > 5 || transport_debug.log_fragment_storage) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TransportReassembly::reassemble_i: "
                 "stored first frag, returning false (incomplete) with %B fragments\n",
                 fragments_.size()));
    }
    return false;
  } else {
    const CompletedMap::const_iterator citer = completed_.find(key.publication_);
    if (citer != completed_.end() && citer->second.contains(key.data_sample_seq_)) {
      // already completed, not storing or delivering this message
      return false;
    }
    if (firstFrag) {
      iter->second.have_first_ = true;
    }
    if (iter->second.total_frags_ < total_frags) {
      iter->second.total_frags_ = total_frags;
    }
    iter->second.expiration_ = expiration;
  }

  if (!iter->second.insert(fragRange, data)) {
    // error condition, already logged by insert()
    return false;
  }

  // We can deliver data if all three of these conditions are met:
  // 1. we've seen the "first fragment" flag  [first frag is here]
  // 2. all fragments have been coalesced     [no gaps in the seq numbers]
  // 3. the "more fragments" flag is not set  [last frag is here]
  if (iter->second.have_first_
      && iter->second.sample_list_.size() == 1
      && !iter->second.sample_list_.front().rec_ds_.header_.more_fragments_) {
    std::swap(data, iter->second.sample_list_.front().rec_ds_);
    fragments_.erase(iter);
    completed_[key.publication_].insert(key.data_sample_seq_);
    if (Transport_debug_level > 5 || transport_debug.log_fragment_storage) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TransportReassembly::reassemble_i: "
                 "removed frag, returning %C with %B fragments\n",
                 data.has_data() ? "true (complete)" : "false (incomplete)", fragments_.size()));
    }
    return data.has_data(); // could be false if we had data_unavailable()
  }

  VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::reassemble_i: "
    "returning false (incomplete)\n"));
  return false;
}

void
TransportReassembly::data_unavailable(const FragmentRange& dropped)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::data_unavailable(): "
    "dropped %q-%q\n", dropped.first, dropped.second));
  typedef OPENDDS_LIST(FragSample)::iterator list_iterator;

  for (FragInfoMap::iterator iter = fragments_.begin(); iter != fragments_.end();
       ++iter) {
    const FragKey& key = iter->first;
    FragInfo& finfo = iter->second;
    FragInfo::FragSampleList& flist = finfo.sample_list_;

    ReceivedDataSample dummy;
    dummy.header_.sequence_ = key.data_sample_seq_;

    // check if we should expand the front element (only if !have_first)
    const FragmentNumber prev = flist.front().frag_range_.first - 1;
    if (dropped.second == prev && !finfo.have_first_) {
      finfo.have_first_ = true;
      dummy.header_.more_fragments_ = true;
      finfo.insert(dropped, dummy);
      continue;
    }

    // find a gap between list elements where "dropped" fits
    for (list_iterator it = flist.begin(); it != flist.end(); ++it) {
      list_iterator it_next = it;
      ++it_next;
      if (it_next == flist.end()) {
        break;
      }
      FragSample& fr1 = *it;
      FragSample& fr2 = *it_next;
      if (dropped.first > fr1.frag_range_.second
          && dropped.second < fr2.frag_range_.first) {
        dummy.header_.more_fragments_ = true;
        finfo.insert(dropped, dummy);
        break;
      }
    }

    // check if we should expand the last element
    const FragmentNumber next = flist.back().frag_range_.second + 1;
    if (dropped.first == next) {
      flist.back().rec_ds_.header_.more_fragments_ = true;
      finfo.insert(dropped, dummy);
    }
  }
}

void
TransportReassembly::data_unavailable(const SequenceNumber& dataSampleSeq,
                                      const GUID_t& pub_id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (fragments_.erase(FragKey(pub_id, dataSampleSeq)) &&
      (Transport_debug_level > 5 || transport_debug.log_fragment_storage)) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TransportReassembly::data_unavailable: "
                  "removed leaving %B fragments\n", fragments_.size()));
  }
}

void TransportReassembly::check_expirations(const MonotonicTimePoint& now)
{
  while (!expiration_queue_.empty() && expiration_queue_.front().first <= now) {
    FragInfoMap::iterator iter = fragments_.find(expiration_queue_.front().second);
    if (iter != fragments_.end()) {
      // FragInfo::expiration_ may have changed after insertion into expiration_queue_
      if (iter->second.expiration_ <= now) {
        fragments_.erase(iter);
        if (Transport_debug_level > 5 || transport_debug.log_fragment_storage) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) TransportReassembly::check_expirations: "
                     "purge expired leaving %B fragments\n", fragments_.size()));
        }
      } else {
        expiration_queue_.push_back(std::make_pair(iter->second.expiration_, iter->first));
      }
    }
    expiration_queue_.pop_front();
  }
}

size_t TransportReassembly::fragments_size() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return fragments_.size();
}

size_t TransportReassembly::queue_size() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return expiration_queue_.size();
}

size_t TransportReassembly::completed_size() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return completed_.size();
}

size_t TransportReassembly::total_frags() const
{
  size_t total = 0;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (FragInfoMap::const_iterator iter = fragments_.begin(); iter != fragments_.end(); ++iter) {
    total += iter->second.total_frags_;
  }
  return total;
}

TransportReassembly::FragInfo::FragInfo()
  : have_first_(false)
  , total_frags_(0)
{}

TransportReassembly::FragInfo::FragInfo(bool hf, const FragSampleList& rl, ACE_UINT32 tf, const MonotonicTimePoint& expiration)
  : have_first_(hf)
  , sample_list_(rl)
  , total_frags_(tf)
  , expiration_(expiration)
{
  for (FragSampleList::iterator it = sample_list_.begin(), prev = it; it != sample_list_.end(); ++it) {
    sample_finder_[it->frag_range_.second] = it;
    if (it != sample_list_.begin()) {
      gap_list_.push_back(FragmentRange(prev->frag_range_.second + 1, it->frag_range_.first - 1));
    }
    prev = it;
  }
  for (FragGapList::iterator it = gap_list_.begin(); it != gap_list_.end(); ++it) {
    gap_finder_[it->second] = it;
  }
}

TransportReassembly::FragInfo::FragInfo(const FragInfo& val)
{
  *this = val;
}

TransportReassembly::FragInfo&
TransportReassembly::FragInfo::operator=(const FragInfo& rhs)
{
  if (this != &rhs) {
    have_first_ = rhs.have_first_;
    sample_list_ = rhs.sample_list_;
    gap_list_ = rhs.gap_list_;
    total_frags_ = rhs.total_frags_;
    expiration_ = rhs.expiration_;
    sample_finder_.clear();
    gap_finder_.clear();
    for (FragSampleList::iterator it = sample_list_.begin(); it != sample_list_.end(); ++it) {
      sample_finder_[it->frag_range_.second] = it;
    }
    for (FragGapList::iterator it = gap_list_.begin(); it != gap_list_.end(); ++it) {
      gap_finder_[it->second] = it;
    }
  }
  return *this;
}

namespace {
  inline void join_err(const char* detail)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: TransportReassembly::FragInfo::insert: ")
      ACE_TEXT("DataSampleHeaders could not be joined: %C\n"), detail));
  }
}

#ifdef ACE_HAS_CPP11
#define OPENDDS_MAYBE_MOVE std::move
#else
#define OPENDDS_MAYBE_MOVE
#endif


bool
TransportReassembly::FragInfo::insert(const FragmentRange& fragRange, ReceivedDataSample& data)
{
  const FragmentNumber prev = fragRange.first - 1, next = fragRange.second + 1;

  FragSampleList::iterator start = sample_list_.begin();
  FragSampleListIterMap::iterator fit = sample_finder_.begin();
  if (!sample_list_.empty()) {
    fit = sample_finder_.lower_bound(prev);
    if (fit != sample_finder_.end()) {
      start = fit->second;
      if (start->frag_range_.second != prev && start != sample_list_.begin()) {
        --start;
        --fit;
      }
    } else {
      start = sample_list_.end();
      --start;
      --fit;
    }
  }

  const SequenceNumber::Value sn = data.header_.sequence_.getValue();

  for (FragSampleList::iterator it = start; it != sample_list_.end(); ++it) {
    FragSample& fr = *it;
    if (next < fr.frag_range_.first) {
      // insert before 'it'
      sample_finder_[fragRange.second] = sample_list_.insert(it, FragSample(fragRange, OPENDDS_MAYBE_MOVE(data)));
      data.clear();
      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) inserted %q-%q on the left of %q-%q\n", sn, fragRange.first, fragRange.second, fr.frag_range_.first, fr.frag_range_.second));
      return true;

    } else if (next == fr.frag_range_.first) {
      // combine on left of fr
      DataSampleHeader joined;
      if (!DataSampleHeader::join(data.header_, fr.rec_ds_.header_, joined)) {
        join_err("left");
        return false;
      }
      fr.rec_ds_.header_ = joined;
      if (fr.rec_ds_.has_data() && data.has_data()) {
        fr.rec_ds_.prepend(data);
      } else {
        fr.rec_ds_.clear();
        data.clear();
      }
      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) combined %q-%q with %q-%q on the left\n", sn, fragRange.first, fragRange.second, fr.frag_range_.first, fr.frag_range_.second));
      fr.frag_range_.first = fragRange.first;
      return true;

    } else if (fragRange.first < fr.frag_range_.first) {
      // split and recursively insert both parts
      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) splitting %q-%q into %q-%q and %q-%q and recursively inserting both\n", sn, fragRange.first, fragRange.second, fragRange.first, fr.frag_range_.first - 1, fr.frag_range_.first, fragRange.second));
      ReceivedDataSample front_split = data.get_fragment_range(0, fr.frag_range_.first - fragRange.first - 1);
      ReceivedDataSample back_split = data.get_fragment_range(fr.frag_range_.first - fragRange.first);
      data.clear();
      const bool r1 = insert(FragmentRange(fragRange.first, fr.frag_range_.first - 1), front_split);
      const bool r2 = insert(FragmentRange(fr.frag_range_.first, fragRange.second), back_split);
      return r1 || r2; // r1 will likely always be true, but check both

    } else if (fragRange.first < fr.frag_range_.second && fr.frag_range_.second < fragRange.second) {
      // split and recursively insert just the back
      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) splitting %q-%q in order to recursively insert %q-%q\n", sn, fragRange.first, fragRange.second, fr.frag_range_.second + 1, fragRange.second));
      ReceivedDataSample back_split = data.get_fragment_range(fr.frag_range_.second - fragRange.first);
      data.clear();
      return insert(FragmentRange(fr.frag_range_.second + 1, fragRange.second), back_split);

    } else if (prev == fr.frag_range_.second) {
      // combine on right of fr
      if (!fr.rec_ds_.has_data()) {
        fr.rec_ds_.header_.more_fragments_ = true;
      }
      DataSampleHeader joined;
      if (!DataSampleHeader::join(fr.rec_ds_.header_, data.header_, joined)) {
        join_err("right");
        return false;
      }
      fr.rec_ds_.header_ = joined;
      if (fr.rec_ds_.has_data() && data.has_data()) {
        fr.rec_ds_.append(data);
      } else {
        fr.rec_ds_.clear();
        data.clear();
      }

      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) combined %q-%q with %q-%q on the right, removing and recursingly inserting\n", sn, fragRange.first, fragRange.second, fr.frag_range_.first, fr.frag_range_.second));

      FragmentRange range(fr.frag_range_.first, fragRange.second);
      ReceivedDataSample copy(OPENDDS_MAYBE_MOVE(fr.rec_ds_));

      sample_list_.erase(it);
      sample_finder_.erase(fit);

      return insert(range, copy);

    } else if (fr.frag_range_.first <= fragRange.first && fr.frag_range_.second >= fragRange.second) {
      VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) duplicate fragment range %q-%q, dropping\n", sn, fragRange.first, fragRange.second));
      return false;
    }
    ++fit;
  }

  // add to end of list
  sample_finder_[fragRange.second] = sample_list_.insert(sample_list_.end(), FragSample(fragRange, OPENDDS_MAYBE_MOVE(data)));
  VDBG((LM_DEBUG, "(%P|%t) TransportReassembly::insert: (SN: %q) inserting %q-%q at the end of the fragment buffer list\n", sn, fragRange.first, fragRange.second));
  data.clear();
  return true;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
