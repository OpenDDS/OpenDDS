// -*- C++ -*-
//
// $Id$

#include "TransportInterface.h"
#include "TransportConfiguration.h"
#include "TransportReactorTask.h"
#include "DataLink_rch.h"
#include "DataLink.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportImpl::TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl","TransportImpl",6);
}

ACE_INLINE
OpenDDS::DCPS::TransportConfiguration*
OpenDDS::DCPS::TransportImpl::config() const
{
  return this->config_.in();
}

ACE_INLINE int
OpenDDS::DCPS::TransportImpl::configure(TransportConfiguration* config)
{
  DBG_ENTRY_LVL("TransportImpl","configure",6);

  GuardType guard(this->lock_);

  if (config == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: invalid configuration.\n"),
                        -1);
    }

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
  if (this->configure_i(config) == -1) {
    // The subclass rejected the configuration attempt.
    ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) ERROR: TransportImpl configuration failed.\n"),
         -1);
  }

  // Our subclass accepted the configuration attempt.
  // Save off a "copy" of the reference for ourselves.
  config->_add_ref();
  this->config_ = config;

  // Open the DL Cleanup task
  // We depend upon the existing config logic to ensure the
  // DL Cleanup task is opened only once
  if (this->dl_clean_task_.open ()) {
    ACE_ERROR_RETURN ((LM_ERROR,
           "(%P|%t) ERROR: DL Cleanup task failed to open : %p\n",
           "open"), -1);
  }

  // Success.
  return 0;
}


/// NOTE: Should only be called if this->lock_ has already been acquired.
//MJM: I am not convinced that this needs to be guarded by the caller.
//MJM: He gets a current snapshot of the value.  If it changes, his copy
//MJM: is stale.  Or do you mean that his stale copy may be stopped if
//MJM: his _use_ of the reactor task is not guarded?
ACE_INLINE OpenDDS::DCPS::TransportReactorTask*
OpenDDS::DCPS::TransportImpl::reactor_task()
{
  DBG_ENTRY_LVL("TransportImpl","reactor_task",6);
  TransportReactorTask_rch task = this->reactor_task_;
  return task._retn();
}


ACE_INLINE int
OpenDDS::DCPS::TransportImpl::set_reactor(TransportReactorTask* task)
{
  DBG_ENTRY_LVL("TransportImpl","set_reactor",6);

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
OpenDDS::DCPS::TransportImpl::release_datalink(DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl","release_datalink",6);

  // Delegate to our subclass.
  this->release_datalink_i(link);
}


/// This is called by a TransportInterface object when it is handling
/// its own request to detach_transport(), and this TransportImpl object
/// is the one to which it is currently attached.
ACE_INLINE void
OpenDDS::DCPS::TransportImpl::detach_interface(TransportInterface* transport_interface)
{
  DBG_ENTRY_LVL("TransportImpl","detach_interface",6);

  GuardType guard(this->lock_);

  // We really don't care if this unbind "works" or not.  As long as we
  // don't have the interface pointer in our interfaces_ collection, then
  // we are happy.
  unbind(interfaces_, transport_interface);
}


ACE_INLINE OpenDDS::DCPS::TransportImpl::ReservationLockType&
OpenDDS::DCPS::TransportImpl::reservation_lock()
{
  DBG_ENTRY_LVL("TransportImpl","reservation_lock",6);
  return this->reservation_lock_;
}


ACE_INLINE const OpenDDS::DCPS::TransportImpl::ReservationLockType&
OpenDDS::DCPS::TransportImpl::reservation_lock() const
{
  DBG_ENTRY_LVL("TransportImpl","reservation_lock",6);
  return this->reservation_lock_;
}


/// Note that this will return -1 if the TransportImpl has not been
/// configure()'d yet.
ACE_INLINE int
OpenDDS::DCPS::TransportImpl::connection_info
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY_LVL("TransportImpl","connection_info",6);

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
OpenDDS::DCPS::TransportImpl::swap_bytes() const
{
  DBG_ENTRY_LVL("TransportImpl","swap_bytes",6);

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


ACE_INLINE void
OpenDDS::DCPS::TransportImpl::pre_shutdown_i ()
{
  //noop
}
