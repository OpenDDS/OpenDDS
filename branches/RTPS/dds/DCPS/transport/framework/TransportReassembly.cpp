/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReassembly.h"

namespace OpenDDS {
namespace DCPS {

TransportReassembly::FragRange::FragRange(const SequenceNumber& transportSeq,
                                          const ReceivedDataSample& data)
  : transport_seq_(transportSeq, transportSeq)
  , sample_(data)
{
}

bool
TransportReassembly::insert(std::list<FragRange>& flist,
                            const SequenceNumber& transportSeq,
                            ReceivedDataSample& data)
{
  using std::list;
  SequenceNumber next = transportSeq, prev = transportSeq.previous();
  ++next;

  for (list<FragRange>::iterator it = flist.begin(); it != flist.end(); ++it) {
    FragRange& fr = *it;
    if (next < fr.transport_seq_.first) {
      // insert before 'it'
      flist.insert(it, FragRange(transportSeq, data));
      return true;

    } else if (next == fr.transport_seq_.first) {
      // combine on left of *it
      DataSampleHeader joined;
      if (!DataSampleHeader::join(data.header_, fr.sample_.header_, joined)) {
        // log error
        return false;
      }
      fr.sample_.header_ = joined;
      data.sample_->cont(fr.sample_.sample_);
      data.sample_ = 0;
      fr.sample_.sample_ = data.sample_;
      fr.transport_seq_.first = transportSeq;
      return true;

    } else if (prev == fr.transport_seq_.second) {
      // combine on right of *it
      DataSampleHeader joined;
      if (!DataSampleHeader::join(fr.sample_.header_, data.header_, joined)) {
        // log error
        return false;
      }
      fr.sample_.header_ = joined;
      ACE_Message_Block* last;
      for (last = fr.sample_.sample_; last->cont(); last = last->cont()) ;
      last->cont(data.sample_);
      data.sample_ = 0;
      fr.transport_seq_.second = transportSeq;

      // check if the next FragRange in the list needs to be combined
      if (++it != flist.end()) {
        if (next == it->transport_seq_.first) {
          if (!DataSampleHeader::join(fr.sample_.header_, it->sample_.header_,
                                      joined)) {
            // log error
            return false;
          }
          fr.sample_.header_ = joined;
          for (last = fr.sample_.sample_; last->cont(); last = last->cont()) ;
          last->cont(it->sample_.sample_);
          it->sample_.sample_ = 0;
          flist.erase(it);
        }
      }
      return true;
    }
  }

  // add to end of list
  flist.push_back(FragRange(transportSeq, data));
  return true;
}

bool
TransportReassembly::reassemble(const SequenceNumber& transportSeq,
                                bool firstFrag,
                                ReceivedDataSample& data)
{
  if (firstFrag) {
    have_first_.insert(data.header_.sequence_);
  }

  FragMap::iterator iter = fragments_.find(data.header_.sequence_);
  if (iter == fragments_.end()) {
    fragments_[data.header_.sequence_].push_back(FragRange(transportSeq, data));
    return false;
  }

  if (!insert(iter->second, transportSeq, data)) {
    return false;
  }

  // We can deliver data if all three of these conditions are met:
  // 1. we've seen the "first fragment" flag  [first frag is here]
  // 2. all fragments have been coalesced     [no gaps in the seq numbers]
  // 3. the "more fragments" flag is not set  [last frag is here]
  if (have_first_.count(data.header_.sequence_)
      && iter->second.size() == 1
      && !iter->second.front().sample_.header_.more_fragments_) {
    swap(data, iter->second.front().sample_);
    fragments_.erase(iter);
    have_first_.erase(data.header_.sequence_);
    return true;
  }

  return false;
}

void
TransportReassembly::data_unavailable(const SequenceRange& dropped)
{
}

}
}
