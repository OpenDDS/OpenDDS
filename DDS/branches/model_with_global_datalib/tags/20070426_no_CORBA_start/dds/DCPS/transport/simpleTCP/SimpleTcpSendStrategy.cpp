// -*- C++ -*-
//
// $Id$

#include "SimpleTcp_pch.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpSendStrategy.h"
#include "SimpleTcpTransport.h"
#include "SimpleTcpConfiguration.h"
#include "SimpleTcpSynchResource.h"
#include "SimpleTcpDataLink.h"

TAO::DCPS::SimpleTcpSendStrategy::SimpleTcpSendStrategy
                                     (SimpleTcpDataLink*      link,
                                      SimpleTcpConfiguration* config,
                                      SimpleTcpConnection*    connection,
                                      SimpleTcpSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","SimpleTcpSendStrategy",5);

  // Keep a "copy" of the connection reference for ourselves
  connection->_add_ref();
  this->connection_ = connection;

  // Give a "copy" of this send strategy to the connection object.
  connection->set_send_strategy (this);

  // Keep a "copy" of the SimpleTcpDataLink reference for ourselves
  link->_add_ref();
  this->link_ = link;
}


TAO::DCPS::SimpleTcpSendStrategy::~SimpleTcpSendStrategy()
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","~SimpleTcpSendStrategy",5);
}


ssize_t
TAO::DCPS::SimpleTcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","send_bytes",5);

  return this->non_blocking_send (iov, n, bp);
}

ACE_HANDLE 
TAO::DCPS::SimpleTcpSendStrategy::get_handle ()
{
  return this->connection_->peer().get_handle();
}


ssize_t 
TAO::DCPS::SimpleTcpSendStrategy::send_bytes_i (const iovec iov[], int n)
{
  return this->connection_->peer().sendv(iov, n);
}


void
TAO::DCPS::SimpleTcpSendStrategy::relink (bool do_suspend)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","relink",5);
  this->connection_->relink (do_suspend);
}

void
TAO::DCPS::SimpleTcpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","stop_i",5);

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Take back the "copy" of connection object given. (see constructor).
  this->connection_ = 0;
}

