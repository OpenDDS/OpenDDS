// -*- C++ -*-
//
// $Id$

#include "ReliableMulticastDataLink.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportImpl::ReliableMulticastTransportImpl()
{
}

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportImpl::~ReliableMulticastTransportImpl()
{
}

ACE_INLINE
bool
OpenDDS::DCPS::ReliableMulticastTransportImpl::acked(RepoId)
{
  return true;
}
