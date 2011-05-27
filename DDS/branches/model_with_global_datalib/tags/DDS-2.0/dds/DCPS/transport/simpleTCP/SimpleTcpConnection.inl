/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcpAcceptor.h"
#include "SimpleTcpDataLink.h"
#include "SimpleTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "ace/SOCK_Connector.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE void
OpenDDS::DCPS::SimpleTcpConnection::remove_receive_strategy()
{
  DBG_ENTRY_LVL("SimpleTcpConnection","remove_receive_strategy",6);

  this->receive_strategy_ = 0;
}

ACE_INLINE void
OpenDDS::DCPS::SimpleTcpConnection::remove_send_strategy()
{
  DBG_ENTRY_LVL("SimpleTcpConnection","remove_send_strategy",6);

  this->send_strategy_ = 0;
}

ACE_INLINE bool
OpenDDS::DCPS::SimpleTcpConnection::is_connector()
{
  return this->is_connector_;
}

ACE_INLINE bool
OpenDDS::DCPS::SimpleTcpConnection::is_connected()
{
  return this->connected_.value();
}

ACE_INLINE void
OpenDDS::DCPS::SimpleTcpConnection::set_datalink(OpenDDS::DCPS::SimpleTcpDataLink* link)
{
  // Keep a "copy" of the reference to the data link for ourselves.
  link->_add_ref();
  this->link_ = link;
}

ACE_INLINE ACE_INET_Addr
OpenDDS::DCPS::SimpleTcpConnection::get_remote_address()
{
  return this->remote_address_;
}

ACE_INLINE
CORBA::Long&
OpenDDS::DCPS::SimpleTcpConnection::transport_priority()
{
  return this->transport_priority_;
}

ACE_INLINE
CORBA::Long
OpenDDS::DCPS::SimpleTcpConnection::transport_priority() const
{
  return this->transport_priority_;
}
