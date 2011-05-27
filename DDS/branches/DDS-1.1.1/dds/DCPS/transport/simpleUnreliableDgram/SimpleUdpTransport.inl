// -*- C++ -*-
//
// $Id$

#include "SimpleUdpConfiguration.h"
#include "SimpleUnreliableDgramDataLink.h"
#include "SimpleUnreliableDgramDataLink_rch.h"
#include "SimpleUdpSocket.h"
#include "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::SimpleUdpTransport::SimpleUdpTransport()
{
  DBG_ENTRY_LVL("SimpleUdpTransport","SimpleUdpTransport",6);
  this->socket_ = new SimpleUdpSocket();
}


ACE_INLINE void
OpenDDS::DCPS::SimpleUdpTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpTransport","deliver_sample",6);

  SimpleUnreliableDgramDataLink_rch link;

  {
    GuardType guard(this->links_lock_);

    if (this->links_.find(remote_address, link) != 0)
      {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Unable to deliver received sample to DataLink.  "
                   "No DataLink found for remote_address.\n"));
        return;
      }
  }

  link->data_received(sample);
}



