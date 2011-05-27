// -*- C++ -*-
//
// $Id$

#include  "dds/DCPS/transport/framework/TransportImpl.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUnreliableDgramDataLink::SimpleUnreliableDgramDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         TransportImpl* transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramDataLink","SimpleUnreliableDgramDataLink",5);
}


ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleUnreliableDgramDataLink::remote_address() const
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramDataLink","remote_address",5);
  return this->remote_address_;
}


ACE_INLINE int
TAO::DCPS::SimpleUnreliableDgramDataLink::connect(TransportSendStrategy* send_strategy)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramDataLink","connect",5);
  return this->start(send_strategy,0);
}

