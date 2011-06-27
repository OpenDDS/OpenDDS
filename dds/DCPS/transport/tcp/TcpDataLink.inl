/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TcpTransport.h"
#include "TcpConnection.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE const ACE_INET_Addr&
OpenDDS::DCPS::TcpDataLink::remote_address() const
{
  DBG_ENTRY_LVL("TcpDataLink","remote_address",6);
  return this->remote_address_;
}

ACE_INLINE OpenDDS::DCPS::TcpConnection_rch
OpenDDS::DCPS::TcpDataLink::get_connection()
{
  return this->connection_;
}

ACE_INLINE OpenDDS::DCPS::TcpTransport_rch
OpenDDS::DCPS::TcpDataLink::get_transport_impl()
{
  return this->transport_;
}
