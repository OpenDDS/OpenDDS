// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpConnection.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpSynchResource.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpSendStrategy::SimpleTcpSendStrategy
                                     (SimpleTcpConfiguration* config,
                                      SimpleTcpConnection*    connection,
                                      SimpleTcpSynchResource* synch_resource)
  : TransportSendStrategy(config, synch_resource)
{
  DBG_ENTRY("SimpleTcpSendStrategy","SimpleTcpSendStrategy");

  // Keep a "copy" of the connection reference for ourselves
  connection->_add_ref();
  this->connection_ = connection;
}

