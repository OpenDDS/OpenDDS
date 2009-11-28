// -*- C++ -*-
//
// $Id$

#include "DummyTcpTransport.h"
#include "DummyTcpConnection.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE const ACE_INET_Addr&
OpenDDS::DCPS::DummyTcpDataLink::remote_address() const
{
  DBG_ENTRY_LVL("DummyTcpDataLink","remote_address",5);
  return this->remote_address_;
}


ACE_INLINE OpenDDS::DCPS::DummyTcpConnection_rch
OpenDDS::DCPS::DummyTcpDataLink::get_connection ()
{
  return this->connection_;
}


ACE_INLINE OpenDDS::DCPS::DummyTcpTransport_rch
OpenDDS::DCPS::DummyTcpDataLink::get_transport_impl ()
{
  return this->transport_;
}

