// -*- C++ -*-
//
// $Id$

#include  "TransportInterface.h"
#include  "TransportConfiguration.h"
#include  "TransportReactorTask.h"
#include  "DataLink_rch.h"
#include  "DataLink.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportImpl::TransportImpl()
{
  DBG_ENTRY("TransportImpl","TransportImpl");
}

ACE_INLINE int
TAO::DCPS::TransportImpl::configure(TransportConfiguration* config)
{
  DBG_ENTRY("TransportImpl","configure");

  GuardType guard(this->lock_);

  if (!this->config_.is_nil())
    {
      // We are rejecting this configuration attempt since this
      // TransportImpl object has already been configured.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl already configured.\n"),
                       -1);
    }
//MJM: So we disallow dynamic reconfiguration?  So if we want a
//MJM: different configuration, we should kill this one and create and
//MJM: configure a new one...

  // Let our subclass take a shot at the configuration object.
  if (this->configure_i(config) == 0)
    {
      // Our subclass accepted the configuration attempt.

      // Save off a "copy" of the reference for ourselves.
      config->_add_ref();
      this->config_ = config;

      // Success.
      return 0;
    }

  // The subclass rejected the configuration attempt.
  return -1;
}


/// NOTE: Should only be called if this->lock_ has already been acquired.
//MJM: I am not convinced that this needs to be guarded by the caller.
//MJM: He gets a current snapshot of the value.  If it changes, his copy
//MJM: is stale.  Or do you mean that his stale copy may be stopped if
//MJM: his _use_ of the reactor task is not guarded?
ACE_INLINE TAO::DCPS::TransportReactorTask*
TAO::DCPS::TransportImpl::reactor_task()
{
  DBG_ENTRY("TransportImpl","reactor_task");
  TransportReactorTask_rch task = this->reactor_task_;
  return task._retn();
}


ACE_INLINE int
TAO::DCPS::TransportImpl::set_reactor(TransportReactorTask* task)
{
  DBG_ENTRY("TransportImpl","set_reactor");

  GuardType guard(this->lock_);

  if (!this->reactor_task_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl already has a reactor.\n"),
                       -1);
    }

  // Keep a copy for ourselves.
  task->_add_ref();
  this->reactor_task_ = task;

  return 0;
}


/// The DataLink itself calls this when it has determined that, due
/// to some remove_associations() call being handled by a TransportInterface
/// object, the DataLink has lost all of its associations, and is not needed
/// any longer.
ACE_INLINE void
TAO::DCPS::TransportImpl::release_datalink(DataLink* link)
{
  DBG_ENTRY("TransportImpl","release_datalink");

  // Delegate to our subclass.
  this->release_datalink_i(link);
}


/// This is called by a TransportInterface object when it is handling
/// its own request to attach_transport(TransportImpl*), and this
/// TransportImpl object is the one to which it should be attached.
ACE_INLINE TAO::DCPS::AttachStatus
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


/// This is called by a TransportInterface object when it is handling
/// its own request to detach_transport(), and this TransportImpl object
/// is the one to which it is currently attached.
ACE_INLINE void
TAO::DCPS::TransportImpl::detach_interface(TransportInterface* interface)
{
  DBG_ENTRY("TransportImpl","detach_interface");

  GuardType guard(this->lock_);

  // We really don't care if this unbind "works" or not.  As long as we
  // don't have the interface pointer in our interfaces_ collection, then
  // we are happy.
  this->interfaces_.unbind(interface);
}


ACE_INLINE TAO::DCPS::DataLink*
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


ACE_INLINE TAO::DCPS::DataLink*
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


ACE_INLINE TAO::DCPS::TransportImpl::ReservationLockType&
TAO::DCPS::TransportImpl::reservation_lock()
{
  DBG_SUB_ENTRY("TransportImpl","reservation_lock",1);
  return this->reservation_lock_;
}


ACE_INLINE const TAO::DCPS::TransportImpl::ReservationLockType&
TAO::DCPS::TransportImpl::reservation_lock() const
{
  DBG_SUB_ENTRY("TransportImpl","reservation_lock",2);
  return this->reservation_lock_;
}


/// Note that this will return -1 if the TransportImpl has not been
/// configure()'d yet.
ACE_INLINE int
TAO::DCPS::TransportImpl::connection_info
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY("TransportImpl","connection_info");

  GuardType guard(this->lock_);

  if (this->config_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl cannot populate "
                        "connection_info - TransportImpl has not "
                        "been configure()'d.\n"),
                       -1);
    }

  // Delegate to our subclass.
  return this->connection_info_i(local_info);
}


/// Note that this will return -1 if the TransportImpl has not been
/// configure()'d yet.
ACE_INLINE int
TAO::DCPS::TransportImpl::swap_bytes() const
{
  DBG_ENTRY("TransportImpl","swap_bytes");

  GuardType guard(this->lock_);

  if (this->config_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl cannot return swap_bytes "
                        "value - TransportImpl has not been configure()'d.\n"),
                       -1);
    }

  return this->config_->swap_bytes_;
}
