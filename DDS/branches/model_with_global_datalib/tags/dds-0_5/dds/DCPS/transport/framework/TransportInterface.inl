// -*- C++ -*-
//
// $Id$

#include  "TransportSendListener.h"
#include  "TransportImpl.h"
#include  "DataLinkSet.h"
#include  "DataLinkSetMap.h"
#include  "DataLink.h"
#include  "dds/DCPS/AssociationData.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "ace/Message_Block.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportInterface::TransportInterface()
  : swap_bytes_(0)
{
  DBG_ENTRY("TransportInterface","TransportInterface");
  this->send_links_ = new DataLinkSet();
}



/// Return the connection_info_ that we retrieved from the TransportImpl
/// in our attach_transport() method.  Once we retrieve it from the
/// TransportImpl (in the attach_transport() method), we "cache" it in
/// a data member (this->connection_info_) so that we don't have to ask
/// the TransportImpl for it each time.
ACE_INLINE const TAO::DCPS::TransportInterfaceInfo&
TAO::DCPS::TransportInterface::connection_info() const
{
  DBG_ENTRY("TransportInterface","connection_info");
  return this->connection_info_;
}


ACE_INLINE int
TAO::DCPS::TransportInterface::swap_bytes() const
{
  DBG_ENTRY("TransportInterface","swap_bytes");
  return this->swap_bytes_;
}


ACE_INLINE TAO::DCPS::SendControlStatus
TAO::DCPS::TransportInterface::send_control(RepoId                 pub_id,
                                            TransportSendListener* listener,
                                            ACE_Message_Block*     msg)
{
  DBG_ENTRY("TransportInterface","send_control");

  DataLinkSet_rch pub_links = this->local_map_.find_set(pub_id);

  if (pub_links.is_nil())
    {
      // We get here if there aren't any remote subscribers that (currently)
      // have an interest in this publisher id.  Just like in the case of
      // the send() method, this is not an error.  A better way to understand
      // would be if this send_control() method were renamed to:
      //   send_control_to_all_interested_remote_subscribers(pub_id,...).
      //
      // And when we get to this spot in the logic, we have still fulfilled
      // our duties - we sent it to all interested remote subscribers - all
      // zero of them.
      listener->control_delivered(msg);
      return SEND_CONTROL_OK;
    }
  else
    {
      // Just have the DataLinkSet do the send_control for us, on each
      // DataLink in the set.
      return pub_links->send_control(pub_id, listener, msg);
    }
}


ACE_INLINE int
TAO::DCPS::TransportInterface::remove_sample
                                     (const DataSampleListElement* sample)
{
  DBG_ENTRY("TransportInterface","remove_sample");

  DataLinkSet_rch pub_links =
                        this->local_map_.find_set(sample->publication_id_);

  // We only need to do something here if the publication_id_ is associated
  // with at least one DataLink.  If it is not associated with any DataLinks
  // (ie, pub_links.is_nil() is true), then we don't have anything to do
  // here, and we don't consider this an error condition - just return 0
  // in the "no DataLinks associated with the sample->publication_id_" case.
  if (!pub_links.is_nil())
    {
      // Tell the DataLinkSet to tell each DataLink in the set to attempt
      // the remove_sample() operation.
      return pub_links->remove_sample(sample);
    }

  // The sample->publication_id_ isn't associated with any DataLinks, so
  // there are no samples to even attempt to remove.  This isn't considered
  // an error condition (which would require a -1 to be returned) - it just
  // means we do nothing except return 0.
//MJM: What is the use-case for this not being an error?  I am trying to
//MJM: think of one, but have been unsuccessful so far.
  return 0;
}


ACE_INLINE int
TAO::DCPS::TransportInterface::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY("TransportInterface","remove_all_control_msgs");

  DataLinkSet_rch pub_links = this->local_map_.find_set(pub_id);

  if (!pub_links.is_nil())
    {
      return pub_links->remove_all_control_msgs(pub_id);
    }

//MJM: What is the use-case for this not being an error?  I am trying to
//MJM: think of one, but have been unsuccessful so far.
  return 0;
}


ACE_INLINE int
TAO::DCPS::TransportInterface::add_subscriptions
                                    (RepoId                 publisher_id,
                                     CORBA::Long            priority,
                                     ssize_t                size,
                                     const AssociationData* subscriptions)
{
  DBG_ENTRY("TransportInterface","add_subscriptions");
  // Delegate to generic add_associations operation
  return this->add_associations(publisher_id,
                                priority,
                                "publisher_id",
                                "subscriber_id",
                                size,
                                subscriptions);
}


ACE_INLINE int
TAO::DCPS::TransportInterface::add_publications
                                   (RepoId                    subscriber_id,
                                    TransportReceiveListener* receive_listener,
                                    CORBA::Long               priority,
                                    ssize_t                   size,
                                    const AssociationData*    publications)
{
  DBG_ENTRY("TransportInterface","add_publications");
  // Delegate to generic add_associations operation
  return this->add_associations(subscriber_id,
                                priority,
                                "subscriber_id",
                                "publisher_id",
                                size,
                                publications,
                                receive_listener);
}


ACE_INLINE void
TAO::DCPS::TransportInterface::remove_associations(ssize_t       size,
                                                   const RepoId* remote_ids)
{
  DBG_ENTRY("TransportInterface","remove_associations");
  // We need to perform the remove_associations logic while we know that
  // no other TransportInterface objects (that share our TransportImpl)
  // will be able to add or remove associations.
  TransportImpl::ReservationGuardType guard(this->impl_->reservation_lock());

  DataLinkSetMap released_locals;

  // Ask the remote_map_ to do the dirty work.  It will also populate
  // the supplied DataLinkSetMap (released_locals) with any local_id to link_id
  // associations that are no longer valid following this remove operation.
  this->remote_map_.release_reservations(size,remote_ids,released_locals);

  // Now we need to ask our local_map_ to remove any released_locals.
  this->local_map_.remove_released(released_locals);
}


ACE_INLINE void
TAO::DCPS::TransportInterface::send(const DataSampleList& samples)
{
  DBG_ENTRY("TransportInterface","send");

  DataSampleListElement* cur = samples.head_;

  while (cur)
    {
      // VERY IMPORTANT NOTE:
      //
      // We have to be very careful in how we deal with the current
      // DataSampleListElement.  The issue is that once we have invoked
      // data_delivered() on the send_listener_ object, or we have invoked
      // send() on the pub_links, we can no longer access the current
      // DataSampleListElement!  Thus, we need to get the next
      // DataSampleListElement (pointer) from the current element now,
      // while it is safe.
      DataSampleListElement* next_elem = cur->next_send_sample_;

      DataLinkSet_rch pub_links =
                          this->local_map_.find_set(cur->publication_id_);

      if (pub_links.is_nil())
        {
          // NOTE: This is the "local publisher id is not currently
          //       associated with any remote subscriber ids" case.

          VDBG((LM_DEBUG,"(%P|%t) DBG: "
               "TransportInterface::send no links for %d\n",
               cur->publication_id_));

          // We tell the send_listener_ that all of the remote subscriber ids
          // that wanted the data (all zero of them) have indeed received
          // the data.
          cur->send_listener_->data_delivered(cur);
        }
      else
        {
          // This will do several things, including adding to the membership
          // of the send_links_ set.  Any DataLinks added to the send_links_
          // set will be also told about the send_start() event.  Those
          // DataLinks (in the pub_links set) that are already in the
          // send_links_ set will not be told about the send_start() event
          // since they heard about it when they were inserted into the
          // send_links_ set.
          this->send_links_->send_start(pub_links.in());

          // Just have the DataLinkSet do the send for us, on each DataLink
          // in the set that is associated with the local publisher id.
          pub_links->send(cur);
        }

      // Move on to the next DataSampleListElement to send.
      cur = next_elem;
    }

  // This will inform each DataLink in the set about the stop_send() event.
  // It will then clear the send_links_ set.
  //
  // The reason that the send_links_ set is cleared is because we continually
  // reuse the same send_links_ object over and over for each call to this
  // send method.
  this->send_links_->send_stop();
}


/// The client application calls this to attach this TransportInterface
/// object to the supplied TransportImpl object.
ACE_INLINE TAO::DCPS::AttachStatus
TAO::DCPS::TransportInterface::attach_transport(TransportImpl* impl)
{
  DBG_ENTRY("TransportInterface","attach_transport");
  GuardType guard(this->lock_);

  if (!this->impl_.is_nil())
    {
      // This TransportInterface object is currently attached to
      // a TransportImpl.
      return ATTACH_BAD_TRANSPORT;
    }

  // Ask the impl for the swap_bytes() value, and cache the answer in
  // a data member (this->swap_bytes_) so that we don't have to ask
  // the impl for it anymore.
  this->swap_bytes_ = impl->swap_bytes();
//MJM: Of course the OO way would be to have a mutator and do it thus:
//MJM:   this->swap_bytes( impl->swap_bytes());

  // Ask the impl for the connection_info() value, and cache the answer in
  // a data member (this->connection_info_) so that we don't have to ask
  // the impl for it anymore.
  impl->connection_info(this->connection_info_);

  // Attempt to attach ourselves to the TransportImpl object now.
  if (impl->attach_interface(this) == ATTACH_BAD_TRANSPORT)
    {
      // The TransportImpl didn't accept our attachment for some reason.
      return ATTACH_BAD_TRANSPORT;
    }

  // Now the TransportImpl object knows about us.  Let's remember the
  // TransportImpl object by saving a "copy" of the reference to our
  // impl_ data member.
  impl->_add_ref();
  this->impl_ = impl;

  return ATTACH_OK;
}


/// This is called by the client application or a subclass of
/// TransportInterface when it has decided to detach this TransportInterface
/// from the TransportImpl to which it is currently attached.  If this
/// TransportInterface isn't currently attached to a TransportImpl, then
/// this is essentially a no-op.
///
/// We assume that this method is only called when the client application (or
/// a subclass of ours) has stopped using this TransportInterface, and no
/// other client application thread is currently executing another one of
/// the TransportInterface methods right now.  This means that the only
/// possible thread collision here would be if the TransportImpl was being
/// shutdown() by another client application thread while the current
/// client application thread is calling this method.  This assumption makes
/// the locking concerns much easier to deal with.
ACE_INLINE void
TAO::DCPS::TransportInterface::detach_transport()
{
  DBG_ENTRY("TransportInterface","detach_transport");
  TransportImpl_rch impl;

  {
    GuardType guard(this->lock_);

    if (this->impl_.is_nil())
      {
        // We are already detached from the transport.
        return;
      }

    // Give away our "copy" of the TransportImpl reference to a local
    // (smart pointer) variable.
    impl = this->impl_._retn();
  }

  {
    // Remove any remaining associations.
    TransportImpl::ReservationGuardType guard(impl->reservation_lock());

    this->remote_map_.release_all_reservations();
    this->local_map_.release_all_reservations();
//MJM: Althgough these are not currently implemented, right?
  }

  // Tell the (detached) TransportImpl object that it should detach
  // this TransportInterface object.  This is essentially opposite of the
  // attach_interface() method which was used to attach this
  // TransportInterface object to the TransportImpl object in the first place
  // (see our attach_transport() method).
  impl->detach_interface(this);
}


/// This is called by the attached TransportImpl object when it wants
/// to detach itself from this TransportInterface.  This is called
/// as a result of the TransportImpl being shutdown().
ACE_INLINE void
TAO::DCPS::TransportInterface::transport_detached()
{
  DBG_ENTRY("TransportInterface","transport_detached");
  {
    GuardType guard(this->lock_);

    if (this->impl_.is_nil())
      {
        // This TransportInterface is not currently attached to any
        // TransportImpl, so we can leave right now.
        return;
      }

    // Drop our reference to the TransportImpl.
    this->impl_ = 0;
  }

  // Note that we do not clear out the DataLinkSetMap data members.  This
  // is on purpose because we don't protect them with a lock, and there
  // could be code that is currently executing in another thread (a thread
  // other than the one that shutdown() our TransportImpl object), and that
  // currently executing code could be actively using the maps.

  // Tell our subclass about the "transport detached event", in case it
  // is interested (ie, it is interested if it has provided its own
  // implementation of the transport_detached_i() method.
  this->transport_detached_i();
}



// NOTES for add_associations() method:
//
//   Things are majorly foo-barred if we are getting errors in this method.
//   We don't undo any associations that were made successfully, so there
//   really is no comprehensive rollback strategy here.  We really treat all
//   failures here as fatal - leaving the TransportInterface and DataLinks in
//   a potentially goofy state.
//
/// The generic add_associations() method used by both add_publications()
/// and add_subscriptions().
ACE_INLINE int
TAO::DCPS::TransportInterface::add_associations
                         (RepoId                    local_id,
                          CORBA::Long               priority,
                          const char*               local_id_str,
                          const char*               remote_id_str,
                          ssize_t                   num_remote_associations,
                          const AssociationData*    remote_associations,
                          TransportReceiveListener* receive_listener)
{
  DBG_ENTRY("TransportInterface","add_associations");
  // We are going to need to obtain our own local "copy" (reference counted)
  // of our TransportImpl object while we have the lock.
  TransportImpl_rch impl;

  {
    GuardType guard(this->lock_);

    if (this->impl_.is_nil())
      {
        // There is no TransportImpl object - the transport must have
        // been detached.  Return the failure code.
        return -1;
      }

    // We have a TransportImpl object.  Make our own "copy" so that we
    // can release our lock_.
    impl = this->impl_;
  }
//MJM: I do not understand why this lock is required.  The refcount is
//MJM: protected as an atomic operation, no?  So what are we doing
//MJM: besides bumping the ref count?

  // Obtain (via find_or_create_set) the local DataLinkSet just once for
  // this entire method.
  DataLinkSet_rch local_set = this->local_map_.find_or_create_set(local_id);

  if (local_set.is_nil())
    {
      // find_or_create_set failure
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to find or create a DataLinkSet for "
                        "local side of association [%s %d].\n",
                        local_id_str,local_id),
                       -1);
    }

  // We need to acquire the reservation lock from the TransportImpl object.
  // Once we have acquired the lock, we know that other TransportInterface
  // objects that share our TransportImpl object will not be able to
  // add or remove reservations at the same time as us.
  TransportImpl::ReservationGuardType guard(impl->reservation_lock());

  for (ssize_t i = 0; i < num_remote_associations; ++i)
    {
      RepoId remote_id = remote_associations[i].remote_id_;

      DataLink_rch link;

      // There are two ways to reserve the DataLink -- as a local publisher,
      // or as a local subscriber.  If the receive_listener argument is
      // a NULL pointer (0), then use the local publisher version.  Otherwise,
      // use the local subscriber version.
      if (receive_listener == 0)
        {
          VDBG((LM_DEBUG,"(%P|%t) TransportInterface::add_associations() pub %d to sub %d\n",
            local_id, remote_id));
          // Local publisher, remote subscriber.
          link = impl->reserve_datalink(remote_associations[i].remote_data_,
                                        remote_id,
                                        local_id,
                                        priority);
        }
      else
        {
          VDBG((LM_DEBUG,"(%P|%t) TransportInterface::add_associations() sub %d to pub %d\n",
            local_id, remote_id));
          // Local subscriber, remote publisher.
          link = impl->reserve_datalink(remote_associations[i].remote_data_,
                                        remote_id,
                                        local_id,
                                        receive_listener,
                                        priority);
        }

      if (link.is_nil())
        {
          // reserve_datalink failure
          ACE_ERROR_RETURN((LM_ERROR,
                            "(%P|%t) ERROR: Failed to reserve a DataLink with the "
                            "TransportImpl for association from local "
                            "[%s %d] to remote [%s %d].\n",
                            local_id_str,local_id,
                            remote_id_str,remote_id),
                           -1);
        }

      // At this point, the DataLink knows about our association.

//MJM: vvv CONNECTION ESTABLISHMENT CHANGES vvv

//MJM: The logic from here to the end of the loop needs to be broken out
//MJM: and moved to another method.  That method should collect up all
//MJM: of the new connections.  When they _all_ have been established,
//MJM: then this routine can move forward.  In the meantime, after this
//MJM: method has made all of the reserve_datalink() calls, it should
//MJM: then wait on a condition that indicates that _all_ of the
//MJM: datalinks are available.

//MJM: It is then the responsibility of the datalinks to not report
//MJM: their successful connection until both sides have checked in.
//MJM: This means adding some additional traffic during connection
//MJM: establishment.  See the datalink details for more info.

//MJM: Hmm...
//MJM: 1) make another method to do the rest of the registration
//MJM:    code here.
//MJM: 2) Make that method aware of how many datalinks need to
//MJM:    be established.
//MJM: 3) After that many links have been established, signal the
//MJM:    condition.
//MJM: 4) Have this method wait() on the signal() after all the
//MJM:    reserve_datalinks() calls have been made.

//MJM: What remains is to add a ready protocol to the connection
//MJM: establishment.

//MJM: ^^^ CONNECTION ESTABLISHMENT CHANGES ^^^

      // Now we need to update the local_map_ to associate the local_id
      // with the new DataLink.  We do this by inserting the DataLink into
      // the local_set that we found (or created) earlier.

      // We consider a result of 1 or 0 to indicate success here.  A result
      // of 0 means that the insert_link() succeeded, and a result of 1 means
      // that the local_id already has an existing reservation with the
      // DataLink.  This is expected to happen.
      if (local_set->insert_link(link.in()) != -1)
        {
          // Now that we know the local_set contains the new DataLink,
          // we need to get the new DataLink into a remote_set within
          // our remote_map_.

          if (this->remote_map_.insert_link(remote_id, link.in()) != -1)
            {
              // Ok.  We are done handling the current association.
              // This means we can continue the loop and go on to
              // the next association.
              continue;
            }
          else
            {
              // The remote_set->insert_link() failed.
              ACE_ERROR((LM_ERROR,
                         "(%P|%t) ERROR: Failed to insert DataLink into remote_map_ "
                         "(DataLinkSetMap) for remote [%s %d].\n",
                         remote_id_str,remote_id));
            }

          // "Undo" logic due to error would go here.
          //   * We would need to undo the local_set->insert_link() operation.
//MJM: Make it so.
        }
      else
        {
          // The local_set->insert_link() failed.
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to insert DataLink into "
                     "DataLinkSet for local [%s %d].\n",
                     local_id_str,local_id));
        }

      // "Undo" logic due to error would go here.
      //   * We would need to undo the impl->reserve_datalink() operation.
      //   * We also would need to iterate over the remote_assocations that
      //     have worked thus far, and nuke them.
//MJM: Right.
//MJM: We will need to make sure that we do _not_ leave the transport in
//MJM: a funky state after a failure to associate.  This will probably
//MJM: be a fairly common occurance in some environments.

      // Only failure conditions will cause the logic to get here.
      return -1;
    }

  // We completed everything without a problem.
  return 0;
}

