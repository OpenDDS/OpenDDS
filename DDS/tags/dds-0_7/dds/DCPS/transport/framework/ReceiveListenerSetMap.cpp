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
  DBG_ENTRY("ReceiveListenerSetMap","~ReceiveListenerSetMap");
}


int
TAO::DCPS::ReceiveListenerSetMap::insert
                                (RepoId                    publisher_id,
                                 RepoId                    subscriber_id,
                                 TransportReceiveListener* receive_listener)
{
  DBG_ENTRY("ReceiveListenerSetMap","insert");
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


