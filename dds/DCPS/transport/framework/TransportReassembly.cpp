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

TransportReassembly::FragKey::FragKey(const PublicationId& pubId,
                                      const SequenceNumber& dataSampleSeq)
  : publication_(pubId)
  , data_sample_seq_(dataSampleSeq)
{
}

GUID_tKeyLessThan TransportReassembly::FragKey::compare_;

TransportReassembly::FragRange::FragRange(const SequenceRange& seqRange,
                                          const ReceivedDataSample& data)
  : transport_seq_(seqRange)
  , rec_ds_(data)
{
}

namespace {
  inline void join_err(const char* detail)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: TransportReassembly::insert() - ")
      ACE_TEXT("DataSampleHeaders could not be joined: %C\n"), detail));
  }
}

bool
TransportReassembly::insert(OPENDDS_LIST(FragRange)& flist,
                            const SequenceRange& seqRange,
                            ReceivedDataSample& data)
{
  const SequenceNumber::Value prev = seqRange.first.getValue() - 1,
    next = seqRange.second.getValue() + 1;

  for (OPENDDS_LIST(FragRange)::iterator it = flist.begin(); it != flist.end(); ++it) {
    FragRange& fr = *it;
    if (next < fr.transport_seq_.first.getValue()) {
      // insert before 'it'
      flist.insert(it, FragRange(seqRange, data));
      VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
        "inserted on left\n"));
      return true;

    } else if (next == fr.transport_seq_.first.getValue()) {
      // combine on left of fr
      DataSampleHeader joined;
      if (!DataSampleHeader::join(data.header_, fr.rec_ds_.header_, joined)) {
        join_err("left");
        return false;
      }
      fr.rec_ds_.header_ = joined;
      if (fr.rec_ds_.sample_ && data.sample_) {
        ACE_Message_Block* last;
        for (last = data.sample_.get(); last->cont(); last = last->cont()) ;
        last->cont(fr.rec_ds_.sample_.release());
        fr.rec_ds_.sample_.reset(data.sample_.release());
      } else {
        fr.rec_ds_.sample_.reset();
      }
      data.sample_.reset();
      fr.transport_seq_.first = seqRange.first;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
        "combined on left\n"));
      return true;

    } else if (prev == fr.transport_seq_.second.getValue()) {
      // combine on right of fr
      if (!fr.rec_ds_.sample_) {
        fr.rec_ds_.header_.more_fragments_ = true;
      }
      DataSampleHeader joined;
      if (!DataSampleHeader::join(fr.rec_ds_.header_, data.header_, joined)) {
        join_err("right");
        return false;
      }
      fr.rec_ds_.header_ = joined;
      if (fr.rec_ds_.sample_ && data.sample_) {
        ACE_Message_Block* last;
        for (last = fr.rec_ds_.sample_.get(); last->cont(); last = last->cont()) ;
        last->cont(data.sample_.release());
      }
      else {
        fr.rec_ds_.sample_.reset();
        data.sample_.reset();
      }

      fr.transport_seq_.second = seqRange.second;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
        "combined on right\n"));

      // check if the next FragRange in the list needs to be combined
      if (++it != flist.end()) {
        if (next == it->transport_seq_.first.getValue()) {
          if (!fr.rec_ds_.sample_) {
            fr.rec_ds_.header_.more_fragments_ = true;
          }
          if (!DataSampleHeader::join(fr.rec_ds_.header_, it->rec_ds_.header_,
                                      joined)) {
            join_err("combined next");
            return false;
          }
          fr.rec_ds_.header_ = joined;
          if (!it->rec_ds_.sample_) {
            fr.rec_ds_.sample_.reset();
          } else {
            if (!fr.rec_ds_.sample_) {
              ACE_ERROR((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("OpenDDS::DCPS::TransportReassembly::insert, ")
                         ACE_TEXT("Cannot dereference null pointer fr.rec_ds_.sample_\n")));
              return false;
            }
            ACE_Message_Block* last;
            for (last = fr.rec_ds_.sample_.get(); last->cont(); last = last->cont()) ;
            last->cont(it->rec_ds_.sample_.release());
          }
          fr.transport_seq_.second = it->transport_seq_.second;
          flist.erase(it);
          VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
            "coalesced on right\n"));
        }
      }
      return true;
    } else if (fr.transport_seq_.first == seqRange.first && fr.transport_seq_.second == seqRange.second) {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
        "duplicate fragment range, dropping\n"));
      return false;
    }
  }

  // add to end of list
  flist.push_back(FragRange(seqRange, data));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::insert() "
    "inserted at end of list\n"));
  return true;
}

bool
TransportReassembly::has_frags(const SequenceNumber& seq,
                               const RepoId& pub_id) const
{
  return fragments_.count(FragKey(pub_id, seq));
}

CORBA::ULong
TransportReassembly::get_gaps(const SequenceNumber& seq, const RepoId& pub_id,
                              CORBA::Long bitmap[], CORBA::ULong length,
                              CORBA::ULong& numBits) const
{
  // length is number of (allocated) words in bitmap, max of 8
  // numBits is number of valid bits in the bitmap, <= length * 32, to account for partial words
  const FragMap::const_iterator iter = fragments_.find(FragKey(pub_id, seq));
  if (iter == fragments_.end() || length == 0) {
    // Nothing missing
    return 0;
  }

  // RTPS's FragmentNumbers are 32-bit values, so we'll only be using the
  // low 32 bits of the 64-bit generalized sequence numbers in
  // FragRange::transport_seq_.

  const OPENDDS_LIST(FragRange)& flist = iter->second;
  const SequenceNumber& first = flist.front().transport_seq_.first;
  const CORBA::ULong base = (first == 1)
    ? flist.front().transport_seq_.second.getLow() + 1
    : 1;

  if (first != 1) {
    // Represent the "gap" before the first list element.
    // base == 1 and the first 2 args to fill_bitmap_range() are deltas of base
    DisjointSequence::fill_bitmap_range(0, first.getLow() - 2,
                                        bitmap, length, numBits);
  } else if (flist.size() == 1) {
    // No gaps, but we know there is (at least 1) more_framents
    DisjointSequence::fill_bitmap_range(0, 0,
                                        bitmap, length, numBits);
    // NOTE: this could send a nack for fragments that are in flight
    // need to defer setting bitmap till heartbeat extending logic
    // in RtpsUdpDataLink::generate_nack_frags
    return base;
  }

  typedef OPENDDS_LIST(FragRange)::const_iterator list_iterator;
  for (list_iterator it = flist.begin(); it != flist.end(); ++it) {
    const list_iterator it_next = ++list_iterator(it);
    if (it_next == flist.end()) {
      break;
    }
    const CORBA::ULong low = it->transport_seq_.second.getLow() + 1 - base,
                       high = it_next->transport_seq_.first.getLow() - 1 - base;
    DisjointSequence::fill_bitmap_range(low, high, bitmap, length, numBits);
  }

  return base;
}

bool
TransportReassembly::reassemble(const SequenceRange& seqRange,
                                ReceivedDataSample& data)
{
  return reassemble_i(seqRange, seqRange.first == 1, data);
}

bool
TransportReassembly::reassemble(const SequenceNumber& transportSeq,
                                bool firstFrag,
                                ReceivedDataSample& data)
{
  return reassemble_i(SequenceRange(transportSeq, transportSeq),
                      firstFrag, data);
}

bool
TransportReassembly::reassemble_i(const SequenceRange& seqRange,
                                  bool firstFrag,
                                  ReceivedDataSample& data)
{
  if (Transport_debug_level > 5) {
    GuidConverter conv(data.header_.publication_id_);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::reassemble() "
      "tseq %q-%q first %d dseq %q pub %C\n", seqRange.first.getValue(),
      seqRange.second.getValue(), firstFrag ? 1 : 0,
      data.header_.sequence_.getValue(), OPENDDS_STRING(conv).c_str()));
  }

  const FragKey key(data.header_.publication_id_, data.header_.sequence_);

  if (firstFrag) {
    have_first_.insert(key);
  }

  FragMap::iterator iter = fragments_.find(key);
  if (iter == fragments_.end()) {
    fragments_[key].push_back(FragRange(seqRange, data));
    // since this is the first fragment we've seen, it can't possibly be done
    VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::reassemble() "
      "stored first frag, returning false (incomplete)\n"));
    return false;
  }

  if (!insert(iter->second, seqRange, data)) {
    // error condition, already logged by insert()
    return false;
  }

  // We can deliver data if all three of these conditions are met:
  // 1. we've seen the "first fragment" flag  [first frag is here]
  // 2. all fragments have been coalesced     [no gaps in the seq numbers]
  // 3. the "more fragments" flag is not set  [last frag is here]
  if (have_first_.count(key)
      && iter->second.size() == 1
      && !iter->second.front().rec_ds_.header_.more_fragments_) {
    swap(data, iter->second.front().rec_ds_);
    fragments_.erase(iter);
    have_first_.erase(key);
    VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::reassemble() "
      "removed frag, returning %C\n", data.sample_ ? "true" : "false"));
    return data.sample_.get(); // could be false if we had data_unavailable()
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::reassemble() "
    "returning false (incomplete)\n"));
  return false;
}

void
TransportReassembly::data_unavailable(const SequenceRange& dropped)
{
  VDBG((LM_DEBUG, "(%P|%t) DBG:   TransportReassembly::data_unavailable() "
    "dropped %q-%q\n", dropped.first.getValue(), dropped.second.getValue()));
  typedef OPENDDS_LIST(FragRange)::iterator list_iterator;

  for (FragMap::iterator iter = fragments_.begin(); iter != fragments_.end();
       ++iter) {
    const FragKey& key = iter->first;
    OPENDDS_LIST(FragRange)& flist = iter->second;

    ReceivedDataSample dummy(0);
    dummy.header_.sequence_ = key.data_sample_seq_;

    // check if we should expand the front element (only if !have_first)
    const SequenceNumber::Value prev =
      flist.front().transport_seq_.first.getValue() - 1;
    if (dropped.second.getValue() == prev && !have_first_.count(key)) {
      have_first_.insert(key);
      dummy.header_.more_fragments_ = true;
      insert(flist, dropped, dummy);
      continue;
    }

    // find a gap between list elements where "dropped" fits
    for (list_iterator it = flist.begin(); it != flist.end(); ++it) {
      list_iterator it_next = it;
      ++it_next;
      if (it_next == flist.end()) {
        break;
      }
      FragRange& fr1 = *it;
      FragRange& fr2 = *it_next;
      if (dropped.first > fr1.transport_seq_.second
          && dropped.second < fr2.transport_seq_.first) {
        dummy.header_.more_fragments_ = true;
        insert(flist, dropped, dummy);
        break;
      }
    }

    // check if we should expand the last element
    const SequenceNumber next =
      ++SequenceNumber(flist.back().transport_seq_.second);
    if (dropped.first == next) {
      flist.back().rec_ds_.header_.more_fragments_ = true;
      insert(flist, dropped, dummy);
    }
  }
}

void
TransportReassembly::data_unavailable(const SequenceNumber& dataSampleSeq,
                                      const RepoId& pub_id)
{
  const FragKey key(pub_id, dataSampleSeq);
  fragments_.erase(key);
  have_first_.erase(key);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
