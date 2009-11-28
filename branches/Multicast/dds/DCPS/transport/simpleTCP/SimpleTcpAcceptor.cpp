/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"
#include "SimpleTcpAcceptor.h"
#include "SimpleTcpTransport.h"
#include "SimpleTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

// This can not be inlined since it needs to have the internals of the
// SimpleTcpTransport available in order to call add_ref(), and that
// gets a bit circular in the dependencies.  Oh well.
OpenDDS::DCPS::SimpleTcpAcceptor::SimpleTcpAcceptor
(SimpleTcpTransport* transport_impl)
  : transport_(transport_impl, false)
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","SimpleTcpAcceptor",6);
}

OpenDDS::DCPS::SimpleTcpAcceptor::~SimpleTcpAcceptor()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","~SimpleTcpAcceptor",6);
}

OpenDDS::DCPS::SimpleTcpConfiguration*
OpenDDS::DCPS::SimpleTcpAcceptor::get_configuration()
{
  return this->transport_->get_configuration();
}

OpenDDS::DCPS::SimpleTcpTransport*
OpenDDS::DCPS::SimpleTcpAcceptor::transport()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","transport",6);
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  SimpleTcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}

void
OpenDDS::DCPS::SimpleTcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","transport_shutdown",6);

  // Drop the reference to the SimpleTcpTransport object.
  this->transport_ = 0;
}
