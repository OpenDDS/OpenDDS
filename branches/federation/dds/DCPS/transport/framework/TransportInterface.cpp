// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportInterface.h"

#if !defined (__ACE_INLINE__)
#include "TransportInterface.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::TransportInterface::~TransportInterface()
{
  DBG_ENTRY_LVL("TransportInterface","~TransportInterface",6);
}

void
OpenDDS::DCPS::TransportInterface::transport_detached_i()
{
  DBG_ENTRY_LVL("TransportInterface","transport_detached_i",6);
  // Subclass should override if interested in the "transport detached event".
}


/// The client application calls this to attach this TransportInterface
/// object to the supplied TransportImpl object.
OpenDDS::DCPS::AttachStatus
OpenDDS::DCPS::TransportInterface::attach_transport(TransportImpl* impl)
{
  DBG_ENTRY_LVL("TransportInterface","attach_transport",6);

  { // guard scope
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
  }

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
void
OpenDDS::DCPS::TransportInterface::detach_transport()
{
  DBG_ENTRY_LVL("TransportInterface","detach_transport",6);
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
int
OpenDDS::DCPS::TransportInterface::add_associations
                         (RepoId                    local_id,
                          CORBA::Long               priority,
                          const char*               local_id_str,
                          const char*               remote_id_str,
                          size_t                    num_remote_associations,
                          const AssociationData*    remote_associations,
                          TransportReceiveListener* receive_listener)
{
  DBG_ENTRY_LVL("TransportInterface","add_associations",6);

  if (this->impl_.is_nil())
    {
      // There is no TransportImpl object - the transport must have
      // been detached.  Return the failure code.
      return -1;
    }

  {
    // We need to acquire the reservation lock from the TransportImpl object.
    // Once we have acquired the lock, we know that other TransportInterface
    // objects that share our TransportImpl object will not be able to
    // add or remove reservations at the same time as us.
    TransportImpl::ReservationGuardType guard(this->impl_->reservation_lock());

    for (size_t i = 0; i < num_remote_associations; ++i)
      {
        RepoId remote_id = remote_associations[i].remote_id_;

        DataLink_rch link;

        // There are two ways to reserve the DataLink -- as a local publisher,
        // or as a local subscriber.  If the receive_listener argument is
        // a NULL pointer (0), then use the local publisher version.  Otherwise,
        // use the local subscriber version.
        if (receive_listener == 0)
          {
            if( DCPS_debug_level > 0) {
              ACE_TCHAR ebuffer[4096] ;
              ACE::format_hexdump(
                (const char*)&remote_associations[i].remote_data_.data[0],
                remote_associations[i].remote_data_.data.length(),
                ebuffer, sizeof(ebuffer)
              ) ;
              ::OpenDDS::DCPS::GuidConverter localConverter( local_id);
              ::OpenDDS::DCPS::GuidConverter remoteConverter( remote_id);
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) TransportInterface::add_associations: ")
                ACE_TEXT("publication %s to subscription %s.\n%s\n"),
                (const char*) localConverter,
                (const char*) remoteConverter,
                ebuffer
              ));
            }
            // Local publisher, remote subscriber.
            link = this->impl_->reserve_datalink(remote_associations[i].remote_data_,
                                                 remote_id,
                                                 local_id,
                                                 priority);
          }
        else
          {
            if( DCPS_debug_level > 0) {
              ACE_TCHAR ebuffer[4096] ;
              ACE::format_hexdump(
                (const char*)&remote_associations[i].remote_data_.data[0],
                remote_associations[i].remote_data_.data.length(),
                ebuffer, sizeof(ebuffer)
              ) ;
              ::OpenDDS::DCPS::GuidConverter localConverter( local_id);
              ::OpenDDS::DCPS::GuidConverter remoteConverter( remote_id);
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) TransportInterface::add_associations: ")
                ACE_TEXT("subscription %s to publication %s.\n%s\n"),
                (const char*) localConverter,
                (const char*) remoteConverter,
                ebuffer
              ));
            }
            // Local subscriber, remote publisher.
            link = this->impl_->reserve_datalink(remote_associations[i].remote_data_,
                                                 remote_id,
                                                 local_id,
                                                 receive_listener,
                                                 priority);
          }

        if (link.is_nil())
          {
            // reserve_datalink failure
            ::OpenDDS::DCPS::GuidConverter localConverter( local_id);
            ::OpenDDS::DCPS::GuidConverter remoteConverter( remote_id);
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Failed to reserve a DataLink with the ")
              ACE_TEXT("TransportImpl for association from local ")
              ACE_TEXT("[%s %s] to remote [%s %s].\n"),
              local_id_str,
              (const char*) localConverter,
              remote_id_str,
              (const char*) remoteConverter
            ),-1);
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
        if (this->local_map_.insert_link(local_id, link.in()) != -1)
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
                ::OpenDDS::DCPS::GuidConverter converter( remote_id);
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: Failed to insert DataLink into remote_map_ ")
                  ACE_TEXT("(DataLinkSetMap) for remote [%s %s].\n"),
                  remote_id_str,
                  (const char*) converter
                ));
              }

            // "Undo" logic due to error would go here.
            //   * We would need to undo the local_set->insert_link() operation.
            //MJM: Make it so.
          }
        else
          {
            // The local_set->insert_link() failed.
            ::OpenDDS::DCPS::GuidConverter converter( local_id);
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Failed to insert DataLink into ")
              ACE_TEXT("local_map_ for local [%s %s].\n"),
              local_id_str,
              (const char*) converter
            ));
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
      } // for scope

    if (ACE_OS::strcmp (local_id_str, "publisher_id") == 0)
      {
        if (this->impl_->add_pending_association (local_id,
                                                  num_remote_associations,
                                                  remote_associations) != 0)
          return -1;
      }
    // We completed everything without a problem.
  } // guard scope

  return 0;
}


void
OpenDDS::DCPS::TransportInterface::remove_associations(ssize_t       size,
                                                   const RepoId* remote_ids,
                                                   const RepoId  local_id,
                                                   const bool pub_side)
{
  DBG_ENTRY_LVL("TransportInterface","remove_associations",6);

  DataLinkSetMap released_locals;

  { // guard scope
    // We need to perform the remove_associations logic while we know that
    // no other TransportInterface objects (that share our TransportImpl)
    // will be able to add or remove associations.
    TransportImpl::ReservationGuardType guard(this->impl_->reservation_lock());

    // Ask the remote_map_ to do the dirty work.  It will also populate
    // the supplied DataLinkSetMap (released_locals) with any local_id to link_id
    // associations that are no longer valid following this remove operation.
    this->remote_map_.release_reservations(size,remote_ids, local_id, released_locals, pub_side);

    // Now we need to ask our local_map_ to remove any released_locals.
    this->local_map_.remove_released(released_locals);
  }
}


/// This is called by the attached TransportImpl object when it wants
/// to detach itself from this TransportInterface.  This is called
/// as a result of the TransportImpl being shutdown().
void
OpenDDS::DCPS::TransportInterface::transport_detached()
{
  DBG_ENTRY_LVL("TransportInterface","transport_detached",6);
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
