// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpAcceptor.h"
#include  "SimpleTcpTransport.h"
#include  "SimpleTcpSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


// This can not be inlined since it needs to have the internals of the
// SimpleTcpTransport available in order to call add_ref(), and that
// gets a bit circular in the dependencies.  Oh well.
TAO::DCPS::SimpleTcpAcceptor::SimpleTcpAcceptor
                                         (SimpleTcpTransport* transport_impl)
{
  DBG_ENTRY("SimpleTcpAcceptor","SimpleTcpAcceptor");
  // Keep a reference for ourselves
  transport_impl->_add_ref();
  this->transport_ = transport_impl;
}

TAO::DCPS::SimpleTcpAcceptor::~SimpleTcpAcceptor()
{
  DBG_ENTRY("SimpleTcpAcceptor","~SimpleTcpAcceptor");
}

TAO::DCPS::SimpleTcpConfiguration*
TAO::DCPS::SimpleTcpAcceptor::get_configuration()
{
  return this->transport_->get_configuration();
}


TAO::DCPS::SimpleTcpTransport*
TAO::DCPS::SimpleTcpAcceptor::transport()
{
  DBG_ENTRY("SimpleTcpAcceptor","transport");
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  SimpleTcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}


void
TAO::DCPS::SimpleTcpAcceptor::transport_shutdown()
{
  DBG_ENTRY("SimpleTcpAcceptor","transport_shutdown");

  // Drop the reference to the SimpleTcpTransport object.
  this->transport_ = 0;
}

