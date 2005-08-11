// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpAcceptor.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpAcceptor.inl"
#endif /* __ACE_INLINE__ */

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

