// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpSynchResource::SimpleTcpSynchResource
                                            (SimpleTcpConnection*  connection,
                                             const ACE_Time_Value& max_output_pause_period)
  : handle_(connection->peer().get_handle()),
    max_output_pause_period_ (max_output_pause_period)
{
  DBG_ENTRY("SimpleTcpSynchResource","SimpleTcpSynchResource");

  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}

