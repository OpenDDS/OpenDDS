// -*- C++ -*-
//
// $Id$

#include  "SimpleUdp_pch.h"
#include  "SimpleUdpSynchResource.h"
#include  "SimpleUdpSocket.h"
#include  "SimpleUdpSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::SimpleUdpSynchResource::SimpleUdpSynchResource
                                            (SimpleUdpSocket*  socket)
  : ThreadSynchResource(socket->get_handle())
{
  DBG_ENTRY_LVL("SimpleUdpSynchResource","SimpleUdpSynchResource",5);

  // Keep our own "copy" of the reference to the connection.
  socket->_add_ref();
  this->socket_ = socket;
}


TAO::DCPS::SimpleUdpSynchResource::~SimpleUdpSynchResource()
{
  DBG_ENTRY_LVL("SimpleUdpSynchResource","~SimpleUdpSynchResource",5);
}





