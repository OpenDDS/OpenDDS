// -*- C++ -*-
//
// $Id$

#include "DummyTcpAcceptor.h"
#include "DummyTcpDataLink.h"
#include "DummyTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "ace/SOCK_Connector.h"
#include "dds/DCPS/transport/framework/EntryExit.h"



ACE_INLINE void
TAO::DCPS::DummyTcpConnection::disconnect()
{
  DBG_ENTRY_LVL("DummyTcpConnection","disconnect",5);
  this->peer().close();
  this->connected_ = false;
}




ACE_INLINE void
TAO::DCPS::DummyTcpConnection::remove_receive_strategy()
{
  DBG_ENTRY_LVL("DummyTcpConnection","remove_receive_strategy",5);

  this->receive_strategy_ = 0;
}


ACE_INLINE void
TAO::DCPS::DummyTcpConnection::remove_send_strategy()
{
  DBG_ENTRY_LVL("DummyTcpConnection","remove_send_strategy",5);

  this->send_strategy_ = 0;
}


ACE_INLINE bool
TAO::DCPS::DummyTcpConnection::is_connector ()
{
  return this->is_connector_;
}


ACE_INLINE bool
TAO::DCPS::DummyTcpConnection::is_connected ()
{
  return this->connected_.value ();
}


ACE_INLINE void
TAO::DCPS::DummyTcpConnection::set_datalink (TAO::DCPS::DummyTcpDataLink* link)
{
  // Keep a "copy" of the reference to the data link for ourselves.
  link->_add_ref ();
  this->link_ = link;
}


ACE_INLINE ACE_INET_Addr
TAO::DCPS::DummyTcpConnection::get_remote_address ()
{
  return this->remote_address_;
}
