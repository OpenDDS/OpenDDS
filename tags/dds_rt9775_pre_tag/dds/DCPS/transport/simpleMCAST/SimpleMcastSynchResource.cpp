// -*- C++ -*-
//
// $Id$

#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSynchResource.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::SimpleMcastSynchResource::SimpleMcastSynchResource
                                            (SimpleMcastSocket*  socket,
                                             SimpleMcastTransport* transport,
                                             const int& max_output_pause_period_ms)
  : ThreadSynchResource (socket->get_handle())
{
  DBG_ENTRY_LVL("SimpleMcastSynchResource","SimpleMcastSynchResource",5);

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


TAO::DCPS::SimpleMcastSynchResource::~SimpleMcastSynchResource()
{
  DBG_ENTRY_LVL("SimpleMcastSynchResource","~SimpleMcastSynchResource",5);
}


void
TAO::DCPS::SimpleMcastSynchResource::notify_lost_on_backpressure_timeout ()
{
  this->transport_->notify_lost_on_backpressure_timeout ();
  this->socket_->close ();
}

