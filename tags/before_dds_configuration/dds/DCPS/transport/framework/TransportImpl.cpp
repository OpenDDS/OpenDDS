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



