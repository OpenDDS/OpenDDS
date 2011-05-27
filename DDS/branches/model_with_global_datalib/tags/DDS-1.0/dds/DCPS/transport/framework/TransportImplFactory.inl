// -*- C++ -*-
//
// $Id$
#include "TransportImpl_rch.h"
#include "TransportImpl.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportImplFactory::TransportImplFactory()
{
  DBG_ENTRY_LVL("TransportImplFactory","TransportImplFactory",5);
}


ACE_INLINE OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::TransportImplFactory::create_impl()
{
  DBG_SUB_ENTRY("TransportImplFactory","create_impl",1);
  // This one is easy.  Simply delegate to the concrete subclass.
  return this->create();
}


ACE_INLINE OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::TransportImplFactory::create_impl
                                       (TransportReactorTask* reactor_task)
{
  DBG_SUB_ENTRY("TransportImplFactory","create_impl",2);
  // Ask the concrete subclass to create the transport impl object.
  TransportImpl_rch impl = this->create();

  if (impl.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to create() the TransportImpl.\n"),
                       0);
    }

//MJM: Make this conditional on the reactor_task being non-null and you
//MJM: can eliminate the previous method.
  // Attempt to supply the reactor to the transport impl object.
  if (impl->set_reactor(reactor_task) != 0)
    {
      // The transport impl didn't accept it for some reason.
      // That's a failure condition.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) TransportImpl rejected the reactor_task.\n"),
                       0);
    }

  return impl._retn ();
}





