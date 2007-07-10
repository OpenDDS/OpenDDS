// -*- C++ -*-
//
// $Id$
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
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","SimpleTcpAcceptor",5);
  // Keep a reference for ourselves
  transport_impl->_add_ref();
  this->transport_ = transport_impl;
}

OpenDDS::DCPS::SimpleTcpAcceptor::~SimpleTcpAcceptor()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","~SimpleTcpAcceptor",5);
}

OpenDDS::DCPS::SimpleTcpConfiguration*
OpenDDS::DCPS::SimpleTcpAcceptor::get_configuration()
{
  return this->transport_->get_configuration();
}


OpenDDS::DCPS::SimpleTcpTransport*
OpenDDS::DCPS::SimpleTcpAcceptor::transport()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","transport",5);
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  SimpleTcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}


void
OpenDDS::DCPS::SimpleTcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("SimpleTcpAcceptor","transport_shutdown",5);

  // Drop the reference to the SimpleTcpTransport object.
  this->transport_ = 0;
}

