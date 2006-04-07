// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"
#if 0
ACE_INLINE
TAO::DCPS::SimpleTcpDataLink::SimpleTcpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         TAO::DCPS::SimpleTcpTransport*  transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY("SimpleTcpDataLink","SimpleTcpDataLink");
}
#endif


ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleTcpDataLink::remote_address() const
{
  DBG_ENTRY("SimpleTcpDataLink","remote_address");
  return this->remote_address_;
}


ACE_INLINE TAO::DCPS::SimpleTcpConnection_rch 
TAO::DCPS::SimpleTcpDataLink::get_connection ()
{
  return this->connection_;
}


