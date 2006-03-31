// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpAcceptor.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "ace/SOCK_Connector.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpConnection::SimpleTcpConnection()
{
  DBG_ENTRY("SimpleTcpConnection","SimpleTcpConnection");
}


ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::disconnect()
{
  DBG_ENTRY("SimpleTcpConnection","disconnect");
  this->peer().close();
}




ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::remove_receive_strategy()
{
  DBG_ENTRY("SimpleTcpConnection","remove_receive_strategy");

  this->receive_strategy_ = 0;
}

