// -*- C++ -*-
//
// $Id$

#include "DummyTcp_pch.h"
#include "DummyTcpSynchResource.h"
#include "DummyTcpConnection.h"
#include "DummyTcpSendStrategy.h"

OpenDDS::DCPS::DummyTcpSynchResource::DummyTcpSynchResource
                                            (DummyTcpConnection*  connection,
                                             const int& max_output_pause_period_ms)
  : ThreadSynchResource (connection->peer().get_handle())
{
  DBG_ENTRY_LVL("DummyTcpSynchResource","DummyTcpSynchResource",5);

  if (max_output_pause_period_ms >= 0)
  {
    this->timeout_ = new ACE_Time_Value (max_output_pause_period_ms/1000,
                                         max_output_pause_period_ms % 1000 * 1000);
  }

  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}


OpenDDS::DCPS::DummyTcpSynchResource::~DummyTcpSynchResource()
{
  DBG_ENTRY_LVL("DummyTcpSynchResource","~DummyTcpSynchResource",5);
}


void
OpenDDS::DCPS::DummyTcpSynchResource::notify_lost_on_backpressure_timeout ()
{
  DBG_ENTRY_LVL("DummyTcpSynchResource","notify_lost_on_backpressure_timeout",5);

  this->connection_->notify_lost_on_backpressure_timeout ();
}

