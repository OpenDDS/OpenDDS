// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastThreadSynchResource::ReliableMulticastThreadSynchResource()
  : OpenDDS::DCPS::ThreadSynchResource(ACE_INVALID_HANDLE)
{
}

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastThreadSynchResource::~ReliableMulticastThreadSynchResource()
{
}
