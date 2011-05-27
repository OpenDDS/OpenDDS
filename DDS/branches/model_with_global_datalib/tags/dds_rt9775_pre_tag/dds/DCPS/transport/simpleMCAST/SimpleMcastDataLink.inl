// -*- C++ -*-
//
// $Id$

#include  "SimpleMcastTransport.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleMcastDataLink::SimpleMcastDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleMcastTransport* transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastDataLink","SimpleMcastDataLink",5);
}


ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleMcastDataLink::remote_address() const
{
  DBG_ENTRY_LVL("SimpleMcastDataLink","remote_address",5);
  return this->remote_address_;
}


ACE_INLINE int
TAO::DCPS::SimpleMcastDataLink::connect(TransportSendStrategy* send_strategy)
{
  DBG_ENTRY_LVL("SimpleMcastDataLink","connect",5);
  return this->start(send_strategy,0);
}

