// -*- C++ -*-
//
// $Id$
#include  "ReceiveListenerSet.h"
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


ACE_INLINE int
TAO::DCPS::ReceiveListenerSetMap::remove(RepoId publisher_id,
                                         RepoId subscriber_id)
{
  DBG_ENTRY("ReceiveListenerSetMap","remove");
  ReceiveListenerSet_rch listener_set;

  if (this->map_.find(publisher_id, listener_set) != 0)
    {
      return 0;
    }

  int result = listener_set->remove(subscriber_id);

  // Ignore the result
  ACE_UNUSED_ARG(result);

  if (listener_set->size() == 0)
    {
      if (this->map_.unbind(publisher_id) != 0)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            "(%P|%t) ERROR: Failed to remove an empty "
                            "ReceiveListenerSet for publisher_id (%d).\n",
                            publisher_id),
                           -1);
        }
    }

  return 0;
}

//MJM: Other than funky return values, the previous and next methods
//MJM: appear to be identical.  Can't you implement remove(a,b) as
//MJM: "release_subscriber(a,b) ; return 0 ;"  Oh.  I guess it returns
//MJM: -1 from one spot as well.  Could the calling code be happy with
//MJM: either of these?  Is this to place where I found no return value
//MJM: of "1".  Could you have been meaning to call the other method?

/// This method is called when the (remote) subscriber is being
/// released.  This method will return a 0 if the subscriber_id is
/// successfully disassociated with the publisher_id *and* there
/// are still other subscribers associated with the publisher_id.
/// This method will return 1 if, after the disassociation, the
/// publisher_id is no longer associated with any subscribers (which
/// also means it's element was removed from our map_).
ACE_INLINE int
TAO::DCPS::ReceiveListenerSetMap::release_subscriber(RepoId publisher_id,
                                                     RepoId subscriber_id)
{
  DBG_ENTRY("ReceiveListenerSetMap","release_subscriber");
  ReceiveListenerSet_rch listener_set;

  if (this->map_.find(publisher_id, listener_set) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: publisher id (%d) not found in map_.\n",
                 publisher_id));
      // Return 1 to indicate that the publisher_id is no longer associated
      // with any subscribers at all.
      return 1;
    }

  int result = listener_set->remove(subscriber_id);

  // Ignore the result
  ACE_UNUSED_ARG(result);

  if (listener_set->size() == 0)
    {
      if (this->map_.unbind(publisher_id) != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to remove an empty "
                     "ReceiveListenerSet for publisher_id (%d).\n",
                     publisher_id));
        }

      // We always return 1 if we know the publisher_id is no longer
      // associated with any ReceiveListeners.
      return 1;
    }

  // There are still ReceiveListeners associated with the publisher_id.
  // We return a 0 in this case.
  return 0;
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
