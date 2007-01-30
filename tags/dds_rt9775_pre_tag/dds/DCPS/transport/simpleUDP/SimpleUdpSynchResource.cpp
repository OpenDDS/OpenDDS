// -*- C++ -*-
//
// $Id$

#include  "SimpleUdp_pch.h"
#include  "SimpleUdpSynchResource.h"
#include  "SimpleUdpSocket.h"
#include  "SimpleUdpSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::SimpleUdpSynchResource::SimpleUdpSynchResource
                                            (SimpleUdpSocket*  socket,
                                             SimpleUdpTransport * transport,
                                             const int& max_output_pause_period_ms)
  : ThreadSynchResource(socket->get_handle())
{
  DBG_ENTRY_LVL("SimpleUdpSynchResource","SimpleUdpSynchResource",5);

  if (max_output_pause_period_ms >= 0)
    {
      this->timeout_ = new ACE_Time_Value (max_output_pause_period_ms/1000, 
                                           max_output_pause_period_ms % 1000 * 1000);
    }

  // Keep our own "copy" of the reference to the connection.
  socket->_add_ref();
  this->socket_ = socket;

  transport->_add_ref ();
  this->transport_ = transport;
}


TAO::DCPS::SimpleUdpSynchResource::~SimpleUdpSynchResource()
{
  DBG_ENTRY_LVL("SimpleUdpSynchResource","~SimpleUdpSynchResource",5);
}


void
TAO::DCPS::SimpleUdpSynchResource::notify_lost_on_backpressure_timeout ()
{
  this->transport_->notify_lost_on_backpressure_timeout ();
  this->socket_->close_socket ();
}


