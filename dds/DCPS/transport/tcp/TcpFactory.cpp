/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpFactory.h"
#include "TcpTransport.h"
#include "TcpConnection.h"
#include "TcpSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "TcpFactory.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TcpFactory::~TcpFactory()
{
  DBG_ENTRY_LVL("TcpFactory","~TcpFactory",6);
}

int
OpenDDS::DCPS::TcpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("TcpFactory","requires_reactor",6);
  // return "true"
  return 1;
}

OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::TcpFactory::create()
{
  DBG_ENTRY_LVL("TcpFactory","create",6);
  return new TcpTransport();
}
