/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpAcceptor.h"
#include "TcpTransport.h"
#include "TcpSendStrategy.h"
#include "TcpInst.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// This can not be inlined since it needs to have the internals of the
// TcpTransport available in order to call add_ref(), and that
// gets a bit circular in the dependencies.  Oh well.
OpenDDS::DCPS::TcpAcceptor::TcpAcceptor
(const TcpTransport_rch& transport_impl)
  : transport_(transport_impl)
{
  DBG_ENTRY_LVL("TcpAcceptor","TcpAcceptor",6);
}

OpenDDS::DCPS::TcpAcceptor::~TcpAcceptor()
{
  DBG_ENTRY_LVL("TcpAcceptor","~TcpAcceptor",6);
}

OpenDDS::DCPS::TcpInst_rch
OpenDDS::DCPS::TcpAcceptor::get_configuration()
{
  return this->transport_->config();
}

OpenDDS::DCPS::TcpTransport_rch
OpenDDS::DCPS::TcpAcceptor::transport()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport",6);
  return this->transport_;
}

void
OpenDDS::DCPS::TcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("TcpAcceptor","transport_shutdown",6);

  // Drop the reference to the TcpTransport object.
  this->transport_.reset();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
