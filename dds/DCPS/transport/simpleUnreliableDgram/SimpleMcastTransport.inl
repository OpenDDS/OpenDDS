// -*- C++ -*-
//
// $Id$

#include "SimpleMcastSocket.h"
#include "SimpleUnreliableDgramDataLink_rch.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleMcastTransport::SimpleMcastTransport()
  : receiver_(false)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","SimpleMcastTransport",6);
  this->socket_ = new SimpleMcastSocket();
}


ACE_INLINE void
OpenDDS::DCPS::SimpleMcastTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address,
                                      CORBA::Long          priority)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","deliver_sample",6);

  ACE_UNUSED_ARG(remote_address);

  SimpleUnreliableDgramDataLink_rch link;

  {
    GuardType guard(this->links_lock_);

    // Override the remote_address passed in - we are always going to be
    // receiving on our multicast address!
    PriorityKey key( priority, this->multicast_group_address_);
    if (this->links_.find( key, link) != 0)
      {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Unable to deliver received sample to DataLink.  "
                   "No DataLink found for remote_address.\n"));
        return;
      }
  }

  link->data_received(sample);
}

