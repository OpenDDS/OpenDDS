// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpAcceptor.h"
#include  "SimpleTcpDataLink.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "ace/SOCK_Connector.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpConnection::SimpleTcpConnection()
: connected_ (false),
  is_connector_ (true)
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


ACE_INLINE bool
TAO::DCPS::SimpleTcpConnection::is_connector ()
{
  return this->is_connector_;
}


ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::set_datalink (TAO::DCPS::SimpleTcpDataLink* link)
{
  // Keep a "copy" of the reference to the data link for ourselves.
  link->_add_ref ();
  this->link_ = link;
}
