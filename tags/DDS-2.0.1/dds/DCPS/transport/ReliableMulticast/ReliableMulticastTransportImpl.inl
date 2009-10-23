/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
OpenDDS::DCPS::ReliableMulticastTransportImpl::acked(RepoId, RepoId)
{
  return true;
}

ACE_INLINE void
OpenDDS::DCPS::ReliableMulticastTransportImpl::remove_ack(RepoId /*pub_id*/, RepoId /*sub_id*/)
{
  //noop
}
