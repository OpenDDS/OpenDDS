// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "TransportImpl.h"


#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportImpl::~TransportImpl()
{
  DBG_ENTRY("TransportImpl","~TransportImpl");
}

void
TAO::DCPS::TransportImpl::shutdown()
{
  DBG_ENTRY("TransportImpl","shutdown");

  {
    GuardType guard(this->lock_);

    if (this->config_.is_nil())
      {
        // This TransportImpl is already shutdown.
//MJM: So, I read here that config_i() actually "starts" us?
        return;
      }

    InterfaceMapType::ENTRY* entry;

    for (InterfaceMapType::ITERATOR itr(this->interfaces_);
         itr.next(entry);
         itr.advance())
      {
        entry->int_id_->transport_detached();
      }

    // Clear our collection of TransportInterface pointers.
    this->interfaces_.unbind_all();

    // Drop our references to the config_ and reactor_task_.
    this->config_ = 0;
    this->reactor_task_ = 0;

//MJM: Won't you need to ACE_UNUSED_ARG here since you are depending on
//MJM: side effects here?

    // We can release our lock_ now.
  }

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();
}


TAO::DCPS::DataLink*
TAO::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_subscriber_info,
                       RepoId                        subscriber_id,
                       RepoId                        publisher_id,
                       CORBA::Long                   priority)
{
  DBG_SUB_ENTRY("TransportImpl","reserve_datalink",1);

  // Not used right now - not sure how it would apply either.
  ACE_UNUSED_ARG(priority);

  // Ask our concrete subclass to find or create a (concrete) DataLink
  // that matches the supplied criterea.

  // Note that we pass-in a 1 as the second argument.  This means that
  // if a new DataLink needs to be created (ie, the find operation fails),
  // then the connection establishment logic will treat the local endpoint
  // as a publisher.  This knowledge dictates whether a passive or active
  // connection establishment procedure should be followed.
  DataLink_rch link = this->find_or_create_datalink(remote_subscriber_info,1);

  if (link.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl subclass was unable to find "
                        "or create a DataLink for local publisher_id (%d) "
                        "to remote subscriber_id (%d).\n",
                        publisher_id, subscriber_id),
                       0);
    }

  link->make_reservation(subscriber_id,publisher_id);

  return link._retn();
}


TAO::DCPS::DataLink*
TAO::DCPS::TransportImpl::reserve_datalink
                      (const TransportInterfaceInfo& remote_publisher_info,
                       RepoId                        publisher_id,
                       RepoId                        subscriber_id,
                       TransportReceiveListener*     receive_listener,
                       CORBA::Long                   priority)
{
  DBG_SUB_ENTRY("TransportImpl","reserve_datalink",2);

  // Not used right now - not sure how it would apply either.
  ACE_UNUSED_ARG(priority);

  // Ask our concrete subclass to find or create a DataLink (actually, a
  // concrete subclass of DataLink) that matches the supplied criterea.
  // Since find_or_create() is pure virtual, the concrete subclass must
  // provide an implementation for us to use.

  // Note that we pass-in a 0 as the second argument.  This means that
  // if a new DataLink needs to be created (ie, the find operation fails),
  // then the connection establishment logic will treat the local endpoint
  // as a subscriber.  This knowledge dictates whether a passive or active
  // connection establishment procedure should be followed.
  DataLink_rch link = this->find_or_create_datalink(remote_publisher_info,0);

  if (link.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl subclass was unable to find "
                        "or create a DataLink for local publisher_id (%d) "
                        "to remote subscriber_id (%d).\n",
                        publisher_id, subscriber_id),
                       0);
    }

  link->make_reservation(publisher_id,
                         subscriber_id,
                         receive_listener);

  return link._retn();
}


/// This is called by a TransportInterface object when it is handling
/// its own request to attach_transport(TransportImpl*), and this
/// TransportImpl object is the one to which it should be attached.
TAO::DCPS::AttachStatus
TAO::DCPS::TransportImpl::attach_interface(TransportInterface* interface)
{
  DBG_ENTRY("TransportImpl","attach_interface");

  GuardType guard(this->lock_);

  if (this->config_.is_nil())
    {
      // Can't attach to a TransportImpl that isn't currently configured.
      // This could mean that this TransportImpl object has never had its
      // configure() method called, or it could mean that this TransportImpl
      // object was shutdown() after the configure() method was called.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Cannot attach_listener() to TransportImpl "
                        "object because TransportImpl is not configured, "
                        "or has been shutdown.\n"),
                       ATTACH_BAD_TRANSPORT);
    }

  if (this->interfaces_.bind(interface,interface) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Cannot attach_listener() to TransportImpl "
                        "object because TransportImpl thinks the "
                        "TransportInterface object is already attached.\n"),
                       ATTACH_BAD_TRANSPORT);
    }

  // Everything worked.  Return success code.
  return ATTACH_OK;
}

