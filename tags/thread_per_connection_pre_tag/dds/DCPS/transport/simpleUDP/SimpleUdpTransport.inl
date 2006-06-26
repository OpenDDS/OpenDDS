// -*- C++ -*-
//
// $Id$

#include  "SimpleUdpConfiguration.h"
#include  "SimpleUdpDataLink.h"
#include  "SimpleUdpSocket.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpTransport::SimpleUdpTransport()
{
  DBG_ENTRY("SimpleUdpTransport","SimpleUdpTransport");
  this->socket_ = new SimpleUdpSocket();
  this->socket_->_add_ref() ;
}


ACE_INLINE void
TAO::DCPS::SimpleUdpTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY("SimpleUdpTransport","deliver_sample");

  SimpleUdpDataLink_rch link;

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


ACE_INLINE bool 
TAO::DCPS::SimpleUdpTransport::acked (RepoId pub_id)
{
  return true;
}


