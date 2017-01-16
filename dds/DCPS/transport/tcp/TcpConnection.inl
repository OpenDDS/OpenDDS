/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TcpAcceptor.h"
#include "TcpDataLink.h"
#include "TcpSendStrategy.h"
#include "TcpReceiveStrategy.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "ace/SOCK_Connector.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
std::size_t&
OpenDDS::DCPS::TcpConnection::id()
{
  return id_;
}

ACE_INLINE void
OpenDDS::DCPS::TcpConnection::remove_receive_strategy()
{
  DBG_ENTRY_LVL("TcpConnection","remove_receive_strategy",6);

  this->receive_strategy_.reset();
}

ACE_INLINE void
OpenDDS::DCPS::TcpConnection::remove_send_strategy()
{
  DBG_ENTRY_LVL("TcpConnection","remove_send_strategy",6);

  this->send_strategy_.reset();
}

ACE_INLINE bool
OpenDDS::DCPS::TcpConnection::is_connector() const
{
  return this->is_connector_;
}

ACE_INLINE bool
OpenDDS::DCPS::TcpConnection::is_connected() const
{
  return this->connected_.value();
}

ACE_INLINE void
OpenDDS::DCPS::TcpConnection::set_datalink(const OpenDDS::DCPS::TcpDataLink_rch& link)
{
  // Keep a "copy" of the reference to the data link for ourselves.
  this->link_ = link;
}

ACE_INLINE ACE_INET_Addr
OpenDDS::DCPS::TcpConnection::get_remote_address()
{
  return this->remote_address_;
}

ACE_INLINE
OpenDDS::DCPS::Priority&
OpenDDS::DCPS::TcpConnection::transport_priority()
{
  return this->transport_priority_;
}

ACE_INLINE
OpenDDS::DCPS::Priority
OpenDDS::DCPS::TcpConnection::transport_priority() const
{
  return this->transport_priority_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
