// -*- C++ -*-
//
// $Id$
#include  "ReceiveListenerSet.h"
#include  "TransportReceiveListener.h"
#include  "ReceivedDataSample.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::ReceiveListenerSet::ReceiveListenerSet()
{
  DBG_ENTRY("ReceiveListenerSet","ReceiveListenerSet");
}


ACE_INLINE int
TAO::DCPS::ReceiveListenerSet::insert(RepoId                    subscriber_id,
                                      TransportReceiveListener* listener)
{
  DBG_ENTRY("ReceiveListenerSet","insert");
  return this->map_.bind(subscriber_id, listener);
}


ACE_INLINE int
TAO::DCPS::ReceiveListenerSet::remove(RepoId subscriber_id)
{
  DBG_ENTRY("ReceiveListenerSet","remove");
  if (this->map_.unbind(subscriber_id) != 0)
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
  DBG_ENTRY("ReceiveListenerSet","size");
  return this->map_.current_size();
}


ACE_INLINE void
TAO::DCPS::ReceiveListenerSet::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY("ReceiveListenerSet","data_received");
  MapType::ENTRY* entry;

  char* ptr = sample.sample_->rd_ptr ();

  for (MapType::ITERATOR itr(this->map_);
       itr.next(entry);
       itr.advance())
  {
    // reset read pointer because demarshal (in data_received()) moves it.
    sample.sample_->rd_ptr (ptr);
    entry->int_id_->data_received(sample);
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

