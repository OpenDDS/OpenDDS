// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpDataLink::SimpleTcpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpTransport*  transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY("SimpleTcpDataLink","SimpleTcpDataLink");
}



ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleTcpDataLink::remote_address() const
{
  DBG_ENTRY("SimpleTcpDataLink","remote_address");
  return this->remote_address_;
}

