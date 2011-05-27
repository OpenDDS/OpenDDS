/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportImpl.h"
#include "TransportImplFactory.h"
#include "TransportReactorTask.h"
#include "TransportExceptions.h"
#include "TransportGenerator.h"
//#include "dds/DCPS/transport/simpleTCP/SimpleTcpGenerator.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportFactory::TransportFactory()
{
  DBG_ENTRY_LVL("TransportFactory","TransportFactory",6);
}

ACE_INLINE
OpenDDS::DCPS::TransportFactory::~TransportFactory()
{
  DBG_ENTRY_LVL("TransportFactory","~TransportFactory",6);
  // Since our maps hold smart pointers, we don't have to iterate over them
  // and release references.  The map destructors will cause the smart
  // pointers to destruct, and that will release the references for us.
}

ACE_INLINE
const OpenDDS::DCPS::TransportFactory::ImplMap&
OpenDDS::DCPS::TransportFactory::get_transport_impl_map()
{
  return impl_map_;
}
