// -*- C++ -*-
//
// $Id$
#include "TransportReceiveListener.h"
#include "ReceivedDataSample.h"
#include "EntryExit.h"
#include "dds/DCPS/Util.h"

ACE_INLINE
TAO::DCPS::ReceiveListenerSet::ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet","ReceiveListenerSet",5);
}


ACE_INLINE int
TAO::DCPS::ReceiveListenerSet::insert(RepoId                    subscriber_id,
                                      TransportReceiveListener* listener)
{
  DBG_ENTRY_LVL("ReceiveListenerSet","insert",5);
  return bind(map_, std::make_pair(subscriber_id, listener));
}


ACE_INLINE int
TAO::DCPS::ReceiveListenerSet::remove(RepoId subscriber_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSet","remove",5);
  if (unbind(map_, subscriber_id) != 0)
    {
      ACE_ERROR_RETURN((LM_DEBUG,
                        "(%P|%t) subscriber_id (%d) not found in map_.\n",
                        subscriber_id),
                       -1);
    }

  return 0;
}


ACE_INLINE ssize_t
TAO::DCPS::ReceiveListenerSet::size() const
{
  DBG_ENTRY_LVL("ReceiveListenerSet","size",5);
  return map_.size();
}


ACE_INLINE void
TAO::DCPS::ReceiveListenerSet::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("ReceiveListenerSet","data_received",5);

  char* ptr = sample.sample_->rd_ptr ();

  for (MapType::iterator itr = map_.begin();
    itr != map_.end();
    ++itr)
  {
    // reset read pointer because demarshal (in data_received()) moves it.
    sample.sample_->rd_ptr (ptr);
    itr->second->data_received(sample);
  }
}


ACE_INLINE TAO::DCPS::ReceiveListenerSet::MapType&
TAO::DCPS::ReceiveListenerSet::map()
{
  DBG_SUB_ENTRY("ReceiveListenerSet","map",1);
  return this->map_;
}


ACE_INLINE const TAO::DCPS::ReceiveListenerSet::MapType&
TAO::DCPS::ReceiveListenerSet::map() const
{
  DBG_SUB_ENTRY("ReceiveListenerSet","map",2);
  return this->map_;
}

