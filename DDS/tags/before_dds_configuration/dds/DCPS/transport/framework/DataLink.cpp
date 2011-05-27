// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "DataLink.h"
#include  "RepoIdSet.h"

#include  "ReceiveListenerSet.h"
#include  "DataLinkSetMap.h"
#include  "ReceivedDataSample.h"

#include  "TransportImpl.h"
#include  "TransportConfiguration.h"

#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

/// Only called by our TransportImpl object.
TAO::DCPS::DataLink::DataLink(TransportImpl* impl)
{
  DBG_ENTRY("DataLink","DataLink");

  impl->_add_ref();
  this->impl_ = impl;
  id_ = DataLink::get_next_datalink_id();
}

TAO::DCPS::DataLink::~DataLink()
{
  DBG_ENTRY("DataLink","~DataLink");
}


//MJM: Include the return value meanings to the header documentation as
//MJM: well.

/// Only called by our TransportImpl object.
///
/// Return Codes: 0 means successful reservation made.
///              -1 means failure.
int
TAO::DCPS::DataLink::make_reservation(RepoId subscriber_id,  /* remote */
                                      RepoId publisher_id)   /* local */
{
  DBG_SUB_ENTRY("DataLink","make_reservation",1);
  int pub_result      = 0;
  int sub_result      = 0;
  int pub_undo_result = 0;

  {
    GuardType guard(this->lock_);

    // Update our pub_map_.  The last argument is a 0 because remote
    // subscribers don't have a TransportReceiveListener object.
    pub_result = this->pub_map_.insert(publisher_id,subscriber_id,0);

    if (pub_result == 0)
      {
        sub_result = this->sub_map_.insert(subscriber_id,publisher_id);

        if (sub_result == 0)
          {
            // Success!
            return 0;
          }
        else
          {
            // Since we failed to insert into into the sub_map_, and have
            // already inserted it in the pub_map_, we better attempt to
            // undo the insert that we did to the pub_map_.  Otherwise,
            // the pub_map_ and sub_map_ will become inconsistent.
            pub_undo_result = this->pub_map_.remove(publisher_id,
                                                    subscriber_id);;
          }
      }

    // We can release our lock_ now.
  }

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (pub_result == 0)
    {
      if (sub_result == 1)
        {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Did you mean -1?  Or were you thinking of the set::insert?
          ACE_ERROR((LM_ERROR,
            "(%P|%t) ERROR: Reservation between remote subscriber_id (%d) "
                     "and local publisher_id (%d) already exists "
                     "in sub_map_.  Reservation failed.\n",
                     subscriber_id, publisher_id));
        }
      else
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to insert remote subscriber_id (%d) "
                     "to local publisher_id (%d) reservation into "
                     "sub_map_.  Reservation failed.\n",
                     subscriber_id, publisher_id));
        }

      if (pub_undo_result != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to remove (undo) local publisher_id (%d) "
                     "to remote subscriber_id (%d) reservation from "
                     "pub_map_.\n",
                     publisher_id, subscriber_id));
        }
    }
  else if (pub_result == 1)
    {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Did you mean -1?
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Reservation between local publisher_id (%d) "
                 "and remote subscriber_id (%d) already exists "
                 "in pub_map_.  Reservation failed.\n",
                 publisher_id, subscriber_id));
    }
  else
    {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Was the intent to return other values?
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to insert local publisher_id (%d) "
                 "to remote subscriber_id (%d) reservation into "
                 "pub_map_.  Reservation failed.\n",
                 publisher_id, subscriber_id));
    }

  return -1;
}


/// Only called by our TransportImpl object.
int
TAO::DCPS::DataLink::make_reservation
                    (RepoId                    publisher_id,     /* remote */
                     RepoId                    subscriber_id,    /* local */
                     TransportReceiveListener* receive_listener)
{
  DBG_SUB_ENTRY("DataLink","make_reservation",2);
  int sub_result      = 0;
  int pub_result      = 0;
  int sub_undo_result = 0;

  {
    GuardType guard(this->lock_);

    // Update our sub_map_.
    sub_result = this->sub_map_.insert(subscriber_id,publisher_id);

    if (sub_result == 0)
      {
        pub_result = this->pub_map_.insert(publisher_id,
                                           subscriber_id,
                                           receive_listener);

        if (pub_result == 0)
          {
            // Success!
            return 0;
          }
        else
          {
            // Since we failed to insert into into the pub_map_, and have
            // already inserted it in the sub_map_, we better attempt to
            // undo the insert that we did to the sub_map_.  Otherwise,
            // the sub_map_ and pub_map_ will become inconsistent.
            sub_undo_result = this->sub_map_.remove(subscriber_id,
                                                    publisher_id);
          }
      }

    // We can release our lock_ now.
  }

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (sub_result == 0)
    {
      if (pub_result == 1)
        {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Did you mean -1?
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Reservation between remote publisher_id (%d) "                     "and local subscriber_id (%d) already exists "
                     "in pub_map_.  Reservation failed.\n",
                     publisher_id,subscriber_id));
        }
      else
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to insert remote publisher_id (%d) "
                     "to local subscriber_id (%d) reservation into "
                     "pub_map_.  Reservation failed.\n",
                     publisher_id,subscriber_id));
        }

      if (sub_undo_result != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to remove (undo) local subscriber_id (%d) "
                     "to remote publisher_id (%d) reservation from "
                     "sub_map_.\n",
                     subscriber_id, publisher_id));
        }
    }
  else if (sub_result == 1)
    {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Did you mean -1?
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Reservation between local subscriber_id (%d) "
                 "and remote publisher_id (%d) already exists "
                 "in sub_map_.  Reservation failed.\n",
                 subscriber_id, publisher_id));
    }
  else
    {
//MJM: I don't see that this branch will ever be reached?
//MJM: A quick trip through the map shows that only 0/-1 is ever returned.
//MJM: Was the intent to return other values?
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to insert local subscriber_id (%d) "
                 "to remote publisher_id (%d) reservation into "
                 "sub_map_.  Reservation failed.\n",
                 subscriber_id, publisher_id));
    }

  return -1;
}


/// This gets invoked when a TransportInterface::remove_associations()
/// call has been made.  Because this DataLink can be shared amongst
/// different TransportInterface objects, and different threads could
/// be "managing" the different TransportInterface objects, we need
/// to make sure that this release_reservations() works in conjunction
/// with a simultaneous call (in another thread) to one of this
/// DataLink's make_reservation() methods.
void
TAO::DCPS::DataLink::release_reservations(RepoId          remote_id,
                                          DataLinkSetMap& released_locals)
{
  DBG_ENTRY("DataLink","release_reservations");
  GuardType guard(this->lock_);

  // See if the remote_id is a publisher_id.
  ReceiveListenerSet_rch listener_set = this->pub_map_.remove_set(remote_id);

  if (listener_set.is_nil())
    {
      // The remote_id is not a publisher_id.
      // See if it is a subscriber_id by looking in our sub_map_.
      RepoIdSet_rch id_set = this->sub_map_.remove_set(remote_id);

      if (id_set.is_nil())
        {
          // We don't know about the remote_id.
          ACE_ERROR((LM_ERROR,
            "(%P|%t) ERROR: Unable to locate remote_id (%d) in pub_map_ "
                     "or sub_map_.\n", remote_id));
        }
      else
        {
          // The remote_id is a subscriber_id.
          this->release_remote_subscriber(remote_id,
                                          id_set.in(),
                                          released_locals);
        }
    }
  else
    {
      // The remote_id is a publisher_id.
      this->release_remote_publisher(remote_id,
                                     listener_set.in(),
                                     released_locals);
    }

  if ((this->pub_map_.size() + this->sub_map_.size()) == 0)
    {
      this->impl_->release_datalink(this);
      this->impl_ = 0;

      if (!this->send_strategy_.is_nil())
        {
          this->send_strategy_->stop();
          this->send_strategy_ = 0;
        }

      if (!this->receive_strategy_.is_nil())
        {
          this->receive_strategy_->stop();
          this->receive_strategy_ = 0;
        }

      // Tell our subclass to handle a "stop" event.
      this->stop_i();
    }
}


/// This method will "deliver" the sample to all TransportReceiveListeners
/// within this DataLink that are interested in the (remote) publisher id
/// that sent the sample.
int
TAO::DCPS::DataLink::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY("DataLink","data_received");

  // Which remote publisher sent this message?
  RepoId publisher_id = sample.header_.publication_id_;

  GuardType guard(this->lock_);

  // Locate the set of TransportReceiveListeners associated with this
  // DataLink that are interested in hearing about any samples received
  // from the remote publisher_id.
  ReceiveListenerSet_rch listener_set = this->pub_map_.find(publisher_id);

  if (listener_set.is_nil())
    {
      // Nobody has any interest in this message.  Drop it on the floor.
      VDBG((LM_DEBUG,
                 "(%P|%t) DataLink received sample from remote publisher_id "
                 "(%d), but is dropping sample since there are no interested "
                 "TransportReceiveListener objects.\n", publisher_id));
      return 0;
    }

  // Just get the set to do our dirty work by having it iterate over its
  // collection of TransportReceiveListeners, and invoke the data_received()
  // method on each one.
  listener_set->data_received(sample);

  return 0;
}


/// No locking needed because the caller (release_reservations()) should
/// have already acquired our lock_.
void
TAO::DCPS::DataLink::release_remote_subscriber
                                        (RepoId          subscriber_id,
                                         RepoIdSet*      pubid_set,
                                         DataLinkSetMap& released_publishers)
{
  DBG_ENTRY("DataLink","release_remote_subscriber");
  RepoIdSet::MapType::ENTRY* entry;

  for (RepoIdSet::MapType::ITERATOR itr(pubid_set->map());
       itr.next(entry);
       itr.advance())
    {
      RepoId publisher_id = entry->ext_id_;

      // Remove the publisher_id => subscriber_id association.
      if (this->pub_map_.release_subscriber(publisher_id,
                                            subscriber_id) == 1)
        {
          // This means that this release() operation has caused the
          // publisher_id to no longer be associated with *any* subscribers.
          released_publishers.insert_link(publisher_id,this);
        }
    }
}


/// No locking needed because the caller (release_reservations()) should
/// have already acquired our lock_.
void
TAO::DCPS::DataLink::release_remote_publisher
                                  (RepoId              publisher_id,
                                   ReceiveListenerSet* listener_set,
                                   DataLinkSetMap&     released_subscribers)
{
  DBG_ENTRY("DataLink","release_remote_publisher");
  ReceiveListenerSet::MapType::ENTRY* entry;

  for (ReceiveListenerSet::MapType::ITERATOR itr(listener_set->map());
       itr.next(entry);
       itr.advance())
    {
      RepoId subscriber_id = entry->ext_id_;

      // Remove the publisher_id => subscriber_id association.
      if (this->sub_map_.release_publisher(subscriber_id,publisher_id) == 1)
        {
          // This means that this release() operation has caused the
          // subscriber_id to no longer be associated with *any* publishers.
          released_subscribers.insert_link(subscriber_id,this);
        }
    }
}


// static
ACE_UINT64
TAO::DCPS::DataLink::get_next_datalink_id ()
{
  static ACE_UINT64 next_id = 0;
  static LockType lock;

  DBG_ENTRY("DataLink","get_next_datalink_id");

  ACE_UINT64 id;
  {
    GuardType guard(lock);
    id = next_id++;
    if (0 == next_id)
    {
      ACE_ERROR((LM_ERROR, 
                 ACE_TEXT("ERROR: DataLink::get_next_datalink_id has rolled over and is reusing ids!\n") ));
    }
  }

  return id;
}

