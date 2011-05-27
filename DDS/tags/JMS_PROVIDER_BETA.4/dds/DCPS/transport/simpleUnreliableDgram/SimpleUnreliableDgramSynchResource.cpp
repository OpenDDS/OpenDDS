// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUnreliableDgramSynchResource.h"
#include "SimpleUnreliableDgramSocket.h"
#include "dds/DCPS/transport/framework/EntryExit.h"


OpenDDS::DCPS::SimpleUnreliableDgramSynchResource::SimpleUnreliableDgramSynchResource
                                            (SimpleUnreliableDgramSocket*  socket,
                                             SimpleUnreliableDgramTransport* transport,
                                             const int& max_output_pause_period_ms)
  : ThreadSynchResource (socket->get_handle())
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSynchResource","SimpleUnreliableDgramSynchResource",6);

  if (max_output_pause_period_ms >= 0)
    {
      this->timeout_ = new ACE_Time_Value (max_output_pause_period_ms/1000, 
                                           max_output_pause_period_ms % 1000 * 1000);
    }
  // Keep our own "copy" of the reference to the connection.
  socket->_add_ref();
  this->socket_ = socket;
  transport->_add_ref();
  this->transport_ = transport;
}


OpenDDS::DCPS::SimpleUnreliableDgramSynchResource::~SimpleUnreliableDgramSynchResource()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSynchResource","~SimpleUnreliableDgramSynchResource",6);
}


void
OpenDDS::DCPS::SimpleUnreliableDgramSynchResource::notify_lost_on_backpressure_timeout ()
{
  this->transport_->notify_lost_on_backpressure_timeout ();
  this->socket_->close_socket ();
}

