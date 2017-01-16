/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/GuidConverter.h"

#include "ReceiveListenerSet.h"

#if !defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ReceiveListenerSet::~ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet","~ReceiveListenerSet",6);
}

bool
ReceiveListenerSet::exist(const RepoId& local_id, bool& last)
{
  GuardType guard(this->lock_);

  last = true;

  TransportReceiveListener_rch listener;

  if (find(map_, local_id, listener) == -1) {
    GuidConverter converter(local_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ReceiveListenerSet::exist: ")
               ACE_TEXT("could not find local %C.\n"),
               OPENDDS_STRING(converter).c_str()));

    return false;
  }

  if (!listener) {
    GuidConverter converter(local_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ReceiveListenerSet::exist: ")
               ACE_TEXT("listener for local %C is nil.\n"),
               OPENDDS_STRING(converter).c_str()));

    return false;
  }

  last = map_.size() == 1;
  return true;
}

void
ReceiveListenerSet::get_keys(ReaderIdSeq & ids)
{
  GuardType guard(this->lock_);

  for (MapType::iterator iter = map_.begin();
       iter != map_.end(); ++ iter) {
    push_back(ids, iter->first);
  }
}

bool
ReceiveListenerSet::exist(const RepoId& local_id)
{
  GuardType guard(this->lock_);

  TransportReceiveListener_rch listener;
  return (find(map_, local_id, listener) == -1 ? false : true);
}

void
ReceiveListenerSet::clear()
{
  GuardType guard(this->lock_);
  this->map_.clear();
}

void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample,
                                  const RepoIdSet& incl_excl,
                                  ConstrainReceiveSet constrain)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received", 6);
  OPENDDS_VECTOR(RcHandle<TransportReceiveListener>) handles;
  {
    GuardType guard(this->lock_);
    for (MapType::iterator itr = map_.begin(); itr != map_.end(); ++itr) {
      if (constrain == ReceiveListenerSet::SET_EXCLUDED) {
        if (itr->second && incl_excl.count(itr->first) == 0) {
          handles.push_back(itr->second);
        }
      } else if (constrain == ReceiveListenerSet::SET_INCLUDED) { //SET_INCLUDED
        if (itr->second && incl_excl.count(itr->first) != 0) {
          handles.push_back(itr->second);
        }
      } else {
        ACE_DEBUG((LM_ERROR, "(%P|%t) ERROR: ReceiveListenerSet::data_received - NOTHING\n"));
      }
    }
  }

  for (size_t i = 0; i < handles.size(); ++i) {
    if (i < handles.size() - 1 && sample.sample_) {
      // demarshal (in data_received()) updates the rd_ptr() of any of
      // the message blocks in the chain, so give it a duplicated chain.
      ReceivedDataSample rds(sample);
      handles[i]->data_received(rds);
    } else {
      handles[i]->data_received(sample);
    }
  }
}

void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample,
                                  const RepoId& readerId)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received(sample, readerId)", 6);
  RcHandle<TransportReceiveListener> h;
  {
    GuardType guard(this->lock_);
    MapType::iterator itr = map_.find(readerId);
    if (itr != map_.end() && itr->second) {
      h = itr->second;
    }
  }
  if (h) h->data_received(sample);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
