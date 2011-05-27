// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpSynchResource::SimpleTcpSynchResource
                                            (SimpleTcpConnection* connection)
  : handle_(connection->peer().get_handle())
{
  DBG_ENTRY("SimpleTcpSynchResource","SimpleTcpSynchResource");

  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}

