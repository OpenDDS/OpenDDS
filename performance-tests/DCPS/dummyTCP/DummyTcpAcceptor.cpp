// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpAcceptor.h"
#include "DummyTcpTransport.h"
#include "DummyTcpSendStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"


// This can not be inlined since it needs to have the internals of the
// DummyTcpTransport available in order to call add_ref(), and that
// gets a bit circular in the dependencies.  Oh well.
OpenDDS::DCPS::DummyTcpAcceptor::DummyTcpAcceptor
                                         (DummyTcpTransport* transport_impl)
{
  DBG_ENTRY_LVL("DummyTcpAcceptor","DummyTcpAcceptor",5);
  // Keep a reference for ourselves
  transport_impl->_add_ref();
  this->transport_ = transport_impl;
}

OpenDDS::DCPS::DummyTcpAcceptor::~DummyTcpAcceptor()
{
  DBG_ENTRY_LVL("DummyTcpAcceptor","~DummyTcpAcceptor",5);
}

OpenDDS::DCPS::DummyTcpInst*
OpenDDS::DCPS::DummyTcpAcceptor::get_configuration()
{
  return this->transport_->get_configuration();
}


OpenDDS::DCPS::DummyTcpTransport*
OpenDDS::DCPS::DummyTcpAcceptor::transport()
{
  DBG_ENTRY_LVL("DummyTcpAcceptor","transport",5);
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  DummyTcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}


void
OpenDDS::DCPS::DummyTcpAcceptor::transport_shutdown()
{
  DBG_ENTRY_LVL("DummyTcpAcceptor","transport_shutdown",5);

  // Drop the reference to the DummyTcpTransport object.
  this->transport_ = 0;
}

