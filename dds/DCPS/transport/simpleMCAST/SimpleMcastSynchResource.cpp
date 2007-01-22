// -*- C++ -*-
//
// $Id$

#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSynchResource.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::SimpleMcastSynchResource::SimpleMcastSynchResource
                                            (SimpleMcastSocket*  socket)
  : ThreadSynchResource (socket->get_handle())
{
  DBG_ENTRY_LVL("SimpleMcastSynchResource","SimpleMcastSynchResource",5);

  // Keep our own "copy" of the reference to the connection.
  socket->_add_ref();
  this->socket_ = socket;
}


TAO::DCPS::SimpleMcastSynchResource::~SimpleMcastSynchResource()
{
  DBG_ENTRY_LVL("SimpleMcastSynchResource","~SimpleMcastSynchResource",5);
}



