// -*- C++ -*-
//
// $Id$
#include  "ReceivedDataSample.h"
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::ReceiveListenerSetMap::ReceiveListenerSetMap()
{
  DBG_ENTRY("ReceiveListenerSetMap","ReceiveListenerSetMap");
}



ACE_INLINE TAO::DCPS::ReceiveListenerSet*
TAO::DCPS::ReceiveListenerSetMap::find(RepoId publisher_id)
{
  DBG_ENTRY("ReceiveListenerSetMap","find");
  ReceiveListenerSet_rch listener_set;

  if (this->map_.find(publisher_id, listener_set) != 0)
    {
      return 0;
    }

  return listener_set._retn();
}



ACE_INLINE TAO::DCPS::ReceiveListenerSet*
TAO::DCPS::ReceiveListenerSetMap::find_or_create(RepoId publisher_id)
{
  DBG_ENTRY("ReceiveListenerSetMap","find_or_create");
  ReceiveListenerSet_rch listener_set;

  if (this->map_.find(publisher_id, listener_set) != 0)
    {
      // It wasn't found.  Create one and insert it.
      listener_set = new ReceiveListenerSet();

      if (this->map_.bind(publisher_id, listener_set) != 0)
        {
           ACE_ERROR((LM_ERROR,
                      "(%P|%t) ERROR: Unable to insert ReceiveListenerSet into the "
                      "ReceiveListenerSetMap for publisher_id %d.\n",
                      publisher_id));
           // Return a 'nil' ReceiveListenerSet*
           return 0;
        }
    }

  return listener_set._retn();
}


ACE_INLINE TAO::DCPS::ReceiveListenerSet*
TAO::DCPS::ReceiveListenerSetMap::remove_set(RepoId publisher_id)
{
  DBG_ENTRY("ReceiveListenerSetMap","remove_set");
  ReceiveListenerSet_rch listener_set;

  if (this->map_.unbind(publisher_id, listener_set) != 0)
    {
       VDBG((LM_DEBUG,
                  "(%P|%t) Unable to remove ReceiveListenerSet from the "
                  "ReceiveListenerSetMap for id %d.\n",
                  publisher_id));
       // Return a 'nil' ReceiveListenerSet*
       return 0;
    }

  return listener_set._retn();
}


ACE_INLINE ssize_t
TAO::DCPS::ReceiveListenerSetMap::size() const
{
  DBG_ENTRY("ReceiveListenerSetMap","size");
  return this->map_.current_size();
}


ACE_INLINE int
TAO::DCPS::ReceiveListenerSetMap::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY("ReceiveListenerSetMap","data_received");
  MapType::ENTRY* entry;

  char* ptr = sample.sample_->rd_ptr ();

  // Iterate over each entry in our map_.
  for (MapType::ITERATOR itr(this->map_);
       itr.next(entry);
       itr.advance())
    {
      // reset read pointer because demarshal (in data_received()) moves it.
      sample.sample_->rd_ptr (ptr);
      // Deliver the sample to the set of TransportReceiveListener objects
      entry->int_id_->data_received(sample);
    }

  return 0;
}


ACE_INLINE TAO::DCPS::ReceiveListenerSetMap::MapType&
TAO::DCPS::ReceiveListenerSetMap::map()
{
  DBG_SUB_ENTRY("ReceiveListenerSetMap","map",1);
  return this->map_;
}


ACE_INLINE const TAO::DCPS::ReceiveListenerSetMap::MapType&
TAO::DCPS::ReceiveListenerSetMap::map() const
{
  DBG_SUB_ENTRY("ReceiveListenerSetMap","map",2);
  return this->map_;
}
