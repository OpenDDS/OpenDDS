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


ACE_INLINE bool
OpenDDS::DCPS::TcpConnection::is_connector() const
{
  return this->is_connector_;
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
