// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpAcceptor.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpSendStrategy.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "ace/SOCK_Connector.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"



ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::disconnect()
{
  DBG_ENTRY_LVL("SimpleTcpConnection","disconnect",5);
  this->peer().close();
  this->connected_ = false;
}




ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::remove_receive_strategy()
{
  DBG_ENTRY_LVL("SimpleTcpConnection","remove_receive_strategy",5);

  this->receive_strategy_ = 0;
}


ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::remove_send_strategy()
{
  DBG_ENTRY_LVL("SimpleTcpConnection","remove_send_strategy",5);

  this->send_strategy_ = 0;
}


ACE_INLINE bool
TAO::DCPS::SimpleTcpConnection::is_connector ()
{
  return this->is_connector_;
}


ACE_INLINE bool
TAO::DCPS::SimpleTcpConnection::is_connected ()
{
  return this->connected_.value ();
}


ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::set_datalink (TAO::DCPS::SimpleTcpDataLink* link)
{
  // Keep a "copy" of the reference to the data link for ourselves.
  link->_add_ref ();
  this->link_ = link;
}


ACE_INLINE ACE_INET_Addr
TAO::DCPS::SimpleTcpConnection::get_remote_address ()
{
  return this->remote_address_;
}
