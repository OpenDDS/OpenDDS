// -*- C++ -*-
//
// $Id$

#include "SimpleTcp_pch.h"
#include "SimpleTcpSynchResource.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpSendStrategy.h"

OpenDDS::DCPS::SimpleTcpSynchResource::SimpleTcpSynchResource
                                            (SimpleTcpConnection*  connection,
                                             const int& max_output_pause_period_ms)
  : ThreadSynchResource (connection->peer().get_handle())
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","SimpleTcpSynchResource",6);

  if (max_output_pause_period_ms >= 0)
  {
    this->timeout_ = new ACE_Time_Value (max_output_pause_period_ms/1000, 
                                         max_output_pause_period_ms % 1000 * 1000);
  }
    
  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}


OpenDDS::DCPS::SimpleTcpSynchResource::~SimpleTcpSynchResource()
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","~SimpleTcpSynchResource",6);
}


void
OpenDDS::DCPS::SimpleTcpSynchResource::notify_lost_on_backpressure_timeout ()
{
  DBG_ENTRY_LVL("SimpleTcpSynchResource","notify_lost_on_backpressure_timeout",6);

  this->connection_->notify_lost_on_backpressure_timeout ();
}

