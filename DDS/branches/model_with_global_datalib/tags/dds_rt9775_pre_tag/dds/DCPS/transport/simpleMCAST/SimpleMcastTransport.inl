// -*- C++ -*-
//
// $Id$

#include  "SimpleMcastConfiguration.h"
#include  "SimpleMcastDataLink.h"
#include  "SimpleMcastSocket.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleMcastTransport::SimpleMcastTransport()
  : receiver_(false)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","SimpleMcastTransport",5);
  this->socket_ = new SimpleMcastSocket();
  this->socket_->_add_ref() ;
}


ACE_INLINE void
TAO::DCPS::SimpleMcastTransport::deliver_sample
                                     (ReceivedDataSample&  sample,
                                      const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","deliver_sample",5);

  ACE_UNUSED_ARG(remote_address);

  SimpleMcastDataLink_rch link;

  {
    GuardType guard(this->links_lock_);

    // Override the remote_address passed in - we are always going to be
    // receiving on our multicast address!
    if (this->links_.find(this->multicast_group_address_, link) != 0)
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
TAO::DCPS::SimpleMcastTransport::acked (RepoId)
{
  return true;
}


