// -*- C++ -*-
//
// $Id$

#include  "TransportImpl.h"
#include  "TransportImplFactory.h"
#include  "TransportReactorTask.h"
#include  "TransportExceptions.h"
#include  "TransportGenerator.h"
#include  "dds/DCPS/transport/simpleTCP/SimpleTcpGenerator.h"
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::TransportFactory::TransportFactory()
{
  DBG_ENTRY("TransportFactory","TransportFactory");
  this->register_simpletcp ();
}

ACE_INLINE
TAO::DCPS::TransportFactory::~TransportFactory()
{
  DBG_ENTRY("TransportFactory","~TransportFactory");
  // Since our maps hold smart pointers, we don't have to iterate over them
  // and release references.  The map destructors will cause the smart
  // pointers to destruct, and that will release the references for us.
}


