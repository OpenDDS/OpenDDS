// -*- C++ -*-
//
// $Id$

#include  "TransportImpl.h"
#include  "TransportImplFactory.h"
#include  "TransportReactorTask.h"
#include  "TransportExceptions.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportFactory::TransportFactory()
{
  DBG_ENTRY("TransportFactory","TransportFactory");
}

ACE_INLINE
TAO::DCPS::TransportFactory::~TransportFactory()
{
  DBG_ENTRY("TransportFactory","~TransportFactory");
  // Since our maps hold smart pointers, we don't have to iterate over them
  // and release references.  The map destructors will cause the smart
  // pointers to destruct, and that will release the references for us.
}
