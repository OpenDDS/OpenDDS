// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

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


ACE_INLINE TAO::DCPS::SimpleTcpTransport_rch 
TAO::DCPS::SimpleTcpDataLink::get_transport_impl ()
{
  return this->transport_;
}

