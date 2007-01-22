// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpSynchResource.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpSendStrategy.h"

TAO::DCPS::SimpleTcpSynchResource::SimpleTcpSynchResource
                                            (SimpleTcpConnection*  connection,
                                             const ACE_Time_Value& max_output_pause_period)
  : ThreadSynchResource (connection->peer().get_handle(), max_output_pause_period)
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","SimpleTcpSynchResource",5);

  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}


TAO::DCPS::SimpleTcpSynchResource::~SimpleTcpSynchResource()
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","~SimpleTcpSynchResource",5);
}


void
TAO::DCPS::SimpleTcpSynchResource::notify_lost_on_backpressure_timeout ()
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","notify_lost_on_backpressure_timeout",5);

  this->connection_->notify_lost_on_backpressure_timeout ();
}

