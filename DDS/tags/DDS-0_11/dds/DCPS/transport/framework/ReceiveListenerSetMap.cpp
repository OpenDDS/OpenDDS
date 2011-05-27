// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "ReceiveListenerSetMap.h"

#if !defined (__ACE_INLINE__)
#include "ReceiveListenerSetMap.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::ReceiveListenerSetMap::~ReceiveListenerSetMap()
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","~ReceiveListenerSetMap",5);
}


int
TAO::DCPS::ReceiveListenerSetMap::insert
                                (RepoId                    publisher_id,
                                 RepoId                    subscriber_id,
                                 TransportReceiveListener* receive_listener)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","insert",5);
  ReceiveListenerSet_rch listener_set = this->find_or_create(publisher_id);

  if (listener_set.is_nil())
    {
      // find_or_create failure
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to find_or_create ReceiveListenerSet "
                        "for publisher_id %d.\n",
                        publisher_id),
                       -1);
    }

  int result = listener_set->insert(subscriber_id, receive_listener);

  if (result == 0)
    {
      // Success.  Leave now.
      return 0;
    }

  // This is error handling code from here on out...

  // Handle the two possible failure cases (duplicate key or unknown)
  if (result == 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: subscriber_id (%d) already exists "
                 "in ReceiveListenerSet for publisher_id (%d).\n",
                 subscriber_id, publisher_id));
    }
  else
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to insert subscriber_id (%d) "
                 "into ReceiveListenerSet for publisher_id (%d).\n",
                 subscriber_id, publisher_id));
    }

  // Deal with possibility that the listener_set just got
  // created - and just for us.  This is to make sure we don't leave any
  // empty ReceiveListenerSets in our map_.
  if (listener_set->size() == 0)
    {
      listener_set = this->remove_set(publisher_id);

      if (listener_set.is_nil())
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to remove (undo create) a "
                     "ReceiveListenerSet for publisher_id (%d).\n",
                     publisher_id));
        }
    }

  return -1;
}


int
TAO::DCPS::ReceiveListenerSetMap::remove(RepoId publisher_id,
                                         RepoId subscriber_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","remove",5);
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
int
TAO::DCPS::ReceiveListenerSetMap::release_subscriber(RepoId publisher_id,
                                                     RepoId subscriber_id)
{
  DBG_ENTRY_LVL("ReceiveListenerSetMap","release_subscriber",5);
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
