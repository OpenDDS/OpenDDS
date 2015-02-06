/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpAcceptor.h"
#include "TcpTransport.h"
#include "TcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

// This can not be inlined since it needs to have the internals of the
// TcpTransport available in order to call add_ref(), and that
// gets a bit circular in the dependencies.  Oh well.
OpenDDS::DCPS::TcpAcceptor::TcpAcceptor
(TcpTransport* transport_impl)
  : transport_(transport_impl, false)
{
  DBG_ENTRY_LVL("TcpAcceptor","TcpAcceptor",6);
}

OpenDDS::DCPS::TcpAcceptor::~TcpAcceptor()
{
  DBG_ENTRY_LVL("TcpAcceptor","~TcpAcceptor",6);
}

OpenDDS::DCPS::TcpInst*
OpenDDS::DCPS::TcpAcceptor::get_configuration()
{
  return this->transport_->get_configuration();
}

OpenDDS::DCPS::TcpTransport*
OpenDDS::DCPS::TcpAcceptor::transport()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport",6);
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  TcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}

void
OpenDDS::DCPS::TcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport_shutdown",6);

  // Drop the reference to the TcpTransport object.
  this->transport_ = 0;
}
