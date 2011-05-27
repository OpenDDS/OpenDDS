// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpConnection.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpSynchResource.h"
#include  "SimpleTcpDataLink.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpSendStrategy::SimpleTcpSendStrategy
                                     (SimpleTcpDataLink*      link,
                                      SimpleTcpConfiguration* config,
                                      SimpleTcpConnection*    connection,
                                      SimpleTcpSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource)
{
  DBG_ENTRY("SimpleTcpSendStrategy","SimpleTcpSendStrategy");

  // Keep a "copy" of the connection reference for ourselves
  connection->_add_ref();
  this->connection_ = connection;

  // Keep a "copy" of the SimpleTcpDataLink reference for ourselves
  link->_add_ref();
  this->link_ = link;
}

