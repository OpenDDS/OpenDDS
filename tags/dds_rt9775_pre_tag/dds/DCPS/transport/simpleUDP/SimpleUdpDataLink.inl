// -*- C++ -*-
//
// $Id$

#include  "SimpleUdpTransport.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpDataLink::SimpleUdpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleUdpTransport* transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpDataLink","SimpleUdpDataLink",5);
}


ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleUdpDataLink::remote_address() const
{
  DBG_ENTRY_LVL("SimpleUdpDataLink","remote_address",5);
  return this->remote_address_;
}


ACE_INLINE int
TAO::DCPS::SimpleUdpDataLink::connect(TransportSendStrategy* send_strategy)
{
  DBG_ENTRY_LVL("SimpleUdpDataLink","connect",5);
  return this->start(send_strategy,0);
}

