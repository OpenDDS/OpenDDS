/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
