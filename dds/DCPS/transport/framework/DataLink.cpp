// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataLink.h"
#include "RepoIdSet.h"

#include "DataLinkSetMap.h"
#include "ReceivedDataSample.h"

#include "TransportImpl.h"
#include "TransportConfiguration.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"

#include "EntryExit.h"
#include "tao/debug.h"

#if !defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

/// Only called by our TransportImpl object.
TAO::DCPS::DataLink::DataLink(TransportImpl* impl)
  : thr_per_con_send_task_ (0)
{
  DBG_ENTRY_LVL("DataLink","DataLink",5);

  impl->_add_ref();
  this->impl_ = impl;

  id_ = DataLink::get_next_datalink_id();

  if (this->impl_->config_->thread_per_connection_)
    {
      this->thr_per_con_send_task_ = new ThreadPerConnectionSendTask (this);
      if (this->thr_per_con_send_task_->open () == -1) {
        ACE_ERROR((LM_ERROR, "(%P|%t)DataLink: failed to open ThreadPerConnectionSendTask\n"));
      }
    }
}

TAO::DCPS::DataLink::~DataLink()
{
  DBG_ENTRY_LVL("DataLink","~DataLink",5);

  if (this->thr_per_con_send_task_ != 0)
    {
      this->thr_per_con_send_task_->close (1);
      delete this->thr_per_con_send_task_;
    }
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
  DBG_SUB_ENTRY("DataLink","make_reservation",5);
  int pub_result      = 0;
  int sub_result      = 0;
  int pub_undo_result = 0;

  {
    GuardType guard (this->strategy_lock_);
    if (!this->send_strategy_.is_nil()) {
      this->send_strategy_->link_released (false);
    }
  }

  {
    GuardType guard(this->pub_map_lock_);
    // Update our pub_map_.  The last argument is a 0 because remote
    // subscribers don't have a TransportReceiveListener object.
    pub_result = this->pub_map_.insert(publisher_id,subscriber_id,0);
  }

  if (pub_result == 0)
    {
      {
        GuardType guard2(this->sub_map_lock_);
        sub_result = this->sub_map_.insert(subscriber_id,publisher_id);
      }
      if (sub_result == 0)
        {
          // Success!
          return 0;
        }
      else
        {
          GuardType guard(this->pub_map_lock_);
          // Since we failed to insert into into the sub_map_, and have
          // already inserted it in the pub_map_, we better attempt to
          // undo the insert that we did to the pub_map_.  Otherwise,
          // the pub_map_ and sub_map_ will become inconsistent.
          pub_undo_result = this->pub_map_.remove(publisher_id,
                                                  subscriber_id);;
        }
    }

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (pub_result == 0)
    {
      if (sub_result != 0)
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
  else
    {
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
    GuardType guard (this->strategy_lock_);
    if (!this->send_strategy_.is_nil()) {
      this->send_strategy_->link_released (false);
    }
  }

  {
    GuardType guard(this->sub_map_lock_);

    // Update our sub_map_.
    sub_result = this->sub_map_.insert(subscriber_id,publisher_id);
  }

  if (sub_result == 0)
    {
      {
        GuardType guard(this->pub_map_lock_);
        pub_result = this->pub_map_.insert(publisher_id,
                                           subscriber_id,
                                           receive_listener);
      }
      if (pub_result == 0)
        {
          // Success!
          return 0;
        }
      else
        {
          GuardType guard(this->sub_map_lock_);
          // Since we failed to insert into into the pub_map_, and have
          // already inserted it in the sub_map_, we better attempt to
          // undo the insert that we did to the sub_map_.  Otherwise,
          // the sub_map_ and pub_map_ will become inconsistent.
          sub_undo_result = this->sub_map_.remove(subscriber_id,
                                                  publisher_id);
        }
    }

  //this->send_strategy_->link_released (false);

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (sub_result == 0)
    {
      if (pub_result != 0)
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
  else
    {
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
                                          RepoId          local_id,
                                          DataLinkSetMap& released_locals)
{
  DBG_ENTRY_LVL("DataLink","release_reservations",5);

  // See if the remote_id is a publisher_id.
  ReceiveListenerSet_rch listener_set;

  {
    GuardType guard(this->pub_map_lock_);
    listener_set = this->pub_map_.find(remote_id);
  }

  if (listener_set.is_nil())
    {
      // The remote_id is not a publisher_id.
      // See if it is a subscriber_id by looking in our sub_map_.
      RepoIdSet_rch id_set;

      {
        GuardType guard(this->sub_map_lock_);
        id_set = this->sub_map_.find(remote_id);
      }

      if (id_set.is_nil())
        {
          // We don't know about the remote_id.
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Unable to locate remote_id (%d) in pub_map_ "
                     "or sub_map_.\n", remote_id));
        }
      else
        {
          VDBG_LVL ((LM_DEBUG, "(%P|%t) The remote_id is a sub id.\n"), 5);
          //guard.release ();
          // The remote_id is a subscriber_id.
          this->release_remote_subscriber(remote_id,
                                          local_id,
                                          id_set,
                                          released_locals);

          if (id_set->size () == 0)
          {
            // Remove the remote_id(sub) after the remote/local ids is released 
            // and there are no local pubs associated with this sub.
            id_set = this->sub_map_.remove_set(remote_id);
          }

          //guard.acquire ();
        }
    }
  else
    {
      VDBG_LVL ((LM_DEBUG, "(%P|%t) The remote_id is a pub id.\n"), 5);
      //guard.release ();
      // The remote_id is a publisher_id.
      this->release_remote_publisher(remote_id,
                                     local_id,
                                     listener_set,
                                     released_locals);

      if (listener_set->size() == 0)
      {
        GuardType guard(this->pub_map_lock_);
        // Remove the remote_id(pub) after the remote/local ids is released 
        // and there are no local subs associated with this pub.
        listener_set = this->pub_map_.remove_set (remote_id);
      }
      //guard.acquire ();
    }

  VDBG_LVL ((LM_DEBUG, "(%P|%t) maps tot size: %d.\n"
             , this->pub_map_.size() + this->sub_map_.size()), 5);

  if ((this->pub_map_.size() + this->sub_map_.size()) == 0)
    {
      if ( ! this->impl_->config_->keep_link_)
        {
          this->pre_stop_i ();
          this->impl_->release_datalink(this);
          // The TransportImpl ptr should be cleaned in the dstr.
          // This link will be used as a callback after the actual
          // connection is closed.
          //this->impl_ = 0;

          TransportSendStrategy_rch send_strategy = 0;
          TransportReceiveStrategy_rch recv_strategy = 0;
          {
            GuardType guard2(this->strategy_lock_);

            if (!this->send_strategy_.is_nil())
              {
                send_strategy =  this->send_strategy_; // save copy
                this->send_strategy_ = 0;
              }

            if (!this->receive_strategy_.is_nil())
              {
                recv_strategy = this->receive_strategy_; // save copy
                this->receive_strategy_ = 0;
              }
          }
          if (!send_strategy.is_nil()) {
            send_strategy->stop();
          }
          if (!recv_strategy.is_nil()) {
            recv_strategy->stop();
          }

          // Tell our subclass to handle a "stop" event.
          this->stop_i();
        }
      else
        {
          GuardType guard2(this->strategy_lock_);
          if (!this->send_strategy_.is_nil())
            {
              this->send_strategy_->link_released (true);
              this->send_strategy_->clear ();
            }
        }
    }
}


/// This method will "deliver" the sample to all TransportReceiveListeners
/// within this DataLink that are interested in the (remote) publisher id
/// that sent the sample.
int
TAO::DCPS::DataLink::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataLink","data_received",5);

  // Which remote publisher sent this message?
  RepoId publisher_id = sample.header_.publication_id_;

  // Locate the set of TransportReceiveListeners associated with this
  // DataLink that are interested in hearing about any samples received
  // from the remote publisher_id.
  ReceiveListenerSet_rch listener_set;

  {
    GuardType guard(this->pub_map_lock_);
    listener_set = this->pub_map_.find(publisher_id);
  }

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
// Ciju: Don't believe a guard is necessary here
void
TAO::DCPS::DataLink::release_remote_subscriber
(RepoId          subscriber_id,
 RepoId          publisher_id,
 RepoIdSet_rch&      pubid_set,
 DataLinkSetMap& released_publishers)
{
  DBG_ENTRY_LVL("DataLink","release_remote_subscriber",5);
  RepoIdSet::MapType::ENTRY* entry;

  for (RepoIdSet::MapType::ITERATOR itr(pubid_set->map());
       itr.next(entry);
       itr.advance())
    {
      if (publisher_id == entry->ext_id_)
      {
        // Remove the publisher_id => subscriber_id association.
        if (this->pub_map_.release_subscriber(publisher_id,
                                              subscriber_id) == 1)
          {
            // This means that this release() operation has caused the
            // publisher_id to no longer be associated with *any* subscribers.
            released_publishers.insert_link(publisher_id,this);
            {
              GuardType guard(this->released_local_lock_);
              released_local_pubs_.insert_id (publisher_id, subscriber_id);
            }
          }
       }
    }

  // remove the publisher_id from the pubset that associate with the remote sub.
  if (pubid_set->remove_id (publisher_id) == -1)
      ACE_ERROR ((LM_ERROR,
                        "(%P|%t) ERROR: DataLink::release_remote_subscriber"
                        " failed to remove pub %d from PubSet.", 
                        publisher_id)); 
}


/// No locking needed because the caller (release_reservations()) should
/// have already acquired our lock_.
// Ciju: Don't believe a guard is necessary here
void
TAO::DCPS::DataLink::release_remote_publisher
(RepoId              publisher_id,
 RepoId              subscriber_id,
 ReceiveListenerSet_rch& listener_set,
 DataLinkSetMap&     released_subscribers)
{
  DBG_ENTRY_LVL("DataLink","release_remote_publisher",5);
  ReceiveListenerSet::MapType::ENTRY* entry;

  for (ReceiveListenerSet::MapType::ITERATOR itr(listener_set->map());
       itr.next(entry);
       itr.advance())
    {
      if (subscriber_id == entry->ext_id_)
      {
        // Remove the publisher_id => subscriber_id association.
        if (this->sub_map_.release_publisher(subscriber_id,publisher_id) == 1)
          {
            // This means that this release() operation has caused the
            // subscriber_id to no longer be associated with *any* publishers.
            released_subscribers.insert_link(subscriber_id,this);
            {
              GuardType guard(this->released_local_lock_);
              released_local_subs_.insert_id (subscriber_id, publisher_id);
            }
          }
      }
    }

  if (listener_set->remove (subscriber_id) == -1)
  {
      ACE_ERROR ((LM_ERROR,
                        "(%P|%t) ERROR: DataLink::release_remote_publisher"
                        " failed to remove sub %d from ListenerSet.", 
                        subscriber_id)); 
  }
}


// static
ACE_UINT64
TAO::DCPS::DataLink::get_next_datalink_id ()
{
  static ACE_UINT64 next_id = 0;
  static LockType lock;

  EntryExit dbg_0( "DataLink","get_next_datalink_id", 0 );

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


void
TAO::DCPS::DataLink::transport_shutdown()
{
  DBG_ENTRY_LVL("DataLink","transport_shutdown",5);

  {
    GuardType guard(this->strategy_lock_);
    // Stop the TransportSendStrategy and the TransportReceiveStrategy.
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
  }

  // Tell our subclass about the "stop" event.
  this->stop_i();

  // Drop our reference to the TransportImpl object
  this->impl_ = 0;
}


void
TAO::DCPS::DataLink::notify (enum ConnectionNotice notice)
{
  DBG_ENTRY_LVL("DataLink","notify",5);

  VDBG((LM_DEBUG, "(%P|%t) DBG: DataLink %X notify %s\n", this,
        connection_notice_as_str(notice)));

  {
    GuardType guard(this->pub_map_lock_);

    ReceiveListenerSetMap::MapType & map = this->pub_map_.map ();

    // Notify the datawriters registered with TransportImpl
    // the lost publications due to a connection problem.
    ReceiveListenerSetMap::MapType::ENTRY* entry;
    for (ReceiveListenerSetMap::MapType::ITERATOR itr(map);
         itr.next(entry);
         itr.advance())
      {
        DataWriterImpl* dw = this->impl_->find_publication(entry->ext_id_);
        if (dw != 0)
          {
            if (TAO_debug_level > 0)
              {
                ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify notify pub %d %s \n",
                           entry->ext_id_, connection_notice_as_str(notice)));
              }
            ReceiveListenerSet_rch subset = entry->int_id_;
            ReceiveListenerSet::MapType & imap = subset->map ();

            ReaderIdSeq subids;
            subids.length (subset->size ());
            CORBA::ULong i = 0;
            ReceiveListenerSet::MapType::ENTRY* ientry;
            for (ReceiveListenerSet::MapType::ITERATOR iitr(imap);
                 iitr.next(ientry);
                 iitr.advance())
              {
                subids[i++] = ientry->ext_id_;
              }

            switch (notice)
              {
              case DISCONNECTED:
                dw->notify_publication_disconnected (subids);
                break;
              case RECONNECTED:
                dw->notify_publication_reconnected (subids);
                break;
              case LOST:
                dw->notify_publication_lost (subids);
                break;
              default:
                ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: DataLink::notify  unknown notice to datawriter\n"));
                break;
              }
          }
        else
          {
            if (TAO_debug_level > 0)
              {
                ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify  not notify pub %d %s \n",
                           entry->ext_id_, connection_notice_as_str(notice)));
              }
          }

      }
  }

  {
    GuardType guard(this->sub_map_lock_);

    // Notify the datareaders registered with TransportImpl
    // the lost subscriptions due to a connection problem.
    RepoIdSetMap::MapType & map = this->sub_map_.map ();

    RepoIdSetMap::MapType::ENTRY* entry;
    for (RepoIdSetMap::MapType::ITERATOR itr(map);
         itr.next(entry);
         itr.advance())
      {
        // subscription_handles
        DataReaderImpl* dr = this->impl_->find_subscription (entry->ext_id_);

        if (dr != 0)
          {
            if (TAO_debug_level > 0)
              {
                ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify notify sub %d %s \n",
                           entry->ext_id_, connection_notice_as_str(notice)));
              }
            RepoIdSet_rch pubset = entry->int_id_;
            RepoIdSet::MapType & map = pubset->map ();

            WriterIdSeq pubids;
            pubids.length (pubset->size ());
            CORBA::ULong i = 0;
            RepoIdSet::MapType::ENTRY* ientry;
            for (RepoIdSet::MapType::ITERATOR iitr(map);
                 iitr.next(ientry);
                 iitr.advance())
              {
                pubids[i++] = ientry->ext_id_;
              }

            switch (notice)
              {
              case DISCONNECTED:
                dr->notify_subscription_disconnected (pubids);
                break;
              case RECONNECTED:
                dr->notify_subscription_reconnected (pubids);
                break;
              case LOST:
                dr->notify_subscription_lost (pubids);
                break;
              default:
                ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: DataLink::notify  unknown notice to datareader\n"));
                break;
              }
          }
        else
          {
            if (TAO_debug_level > 0)
              {
                ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify not notify sub %d subscription lost \n",
                           entry->ext_id_));
              }

          }
      }
  }
}


void
TAO::DCPS::DataLink::notify_connection_deleted ()
{
  GuardType guard(this->released_local_lock_);

  RepoIdSet::MapType& pmap = released_local_pubs_.map ();
  RepoIdSet::MapType::ENTRY* pentry;
  for (RepoIdSet::MapType::ITERATOR itr(pmap);
       itr.next(pentry);
       itr.advance())
    {
      DataWriterImpl* dw = this->impl_->find_publication(pentry->ext_id_);
      if (dw != 0)
        {
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify_connection_deleted notify pub %d "
                         "connection deleted \n", pentry->ext_id_));
            }

          dw->notify_connection_deleted ();
        }
    }

  RepoIdSet::MapType& smap = released_local_subs_.map ();
  RepoIdSet::MapType::ENTRY* sentry;
  for (RepoIdSet::MapType::ITERATOR itr2(smap);
       itr2.next(sentry);
       itr2.advance())
    {
      DataReaderImpl* dr = this->impl_->find_subscription(sentry->ext_id_);
      if (dr != 0)
        {
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG, "(%P|%t)DataLink::notify_connection_deleted notify sub %d "
                         "connection deleted \n", sentry->ext_id_));
            }

          dr->notify_connection_deleted ();
        }
    }
}


void
TAO::DCPS::DataLink::pre_stop_i()
{
  if (this->thr_per_con_send_task_ != 0)
    {
      this->thr_per_con_send_task_->close (1);
    }
}


ACE_Message_Block*
TAO::DCPS::DataLink::marshal_acks (bool byte_order)
{
  DBG_ENTRY_LVL("DataLink","marshal_acks",5);
  return this->sub_map_.marshal (byte_order);
}

bool
TAO::DCPS::DataLink::release_resources ()
{
  DBG_ENTRY_LVL("DataLink", "release_resources", 5);

  return impl_->release_link_resources (this);
  return true;
}


bool 
TAO::DCPS::DataLink::is_target (const RepoId& sub_id)
{
 GuardType guard(this->sub_map_lock_);
 RepoIdSet_rch pubs = this->sub_map_.find(sub_id);

 return ! pubs.is_nil ();
}


bool 
TAO::DCPS::DataLink::exist (const RepoId& remote_id,
                            const RepoId& local_id,
                            const bool&   pub_side,
                            bool& last)
{
  if (pub_side)
  {
    GuardType guard(this->pub_map_lock_);
    RepoIdSet_rch pubs = this->sub_map_.find(remote_id);
    return pubs->exist (local_id, last);
  }
  else
  {
    GuardType guard(this->sub_map_lock_);
    ReceiveListenerSet_rch subs = this->pub_map_.find(remote_id);
    return subs->exist (local_id, last);
  }
}

