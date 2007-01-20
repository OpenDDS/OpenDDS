// -*- C++ -*-
//
// $Id$

#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSynchResource.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastSendStrategy.h"
#include  "SimpleMcastSendStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::SimpleMcastSynchResource::SimpleMcastSynchResource
                                            (SimpleMcastSocket*  socket)
  : handle_(socket->get_handle())
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


int
TAO::DCPS::SimpleMcastSynchResource::wait_to_unclog()
{
  DBG_ENTRY_LVL("SimpleMcastSynchResource","wait_to_unclog",5);

  ACE_Time_Value* timeout = 0;

  // Wait for the blocking to subside or timeout.
  if (ACE::handle_write_ready(this->handle_, timeout) == -1)
    {
      if (errno == ETIME)
        {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: handle_write_ready timed out\n"));
        }
      else
        {
          ACE_ERROR((LM_ERROR,
                    "(%P|%t) ERROR: ACE::handle_write_ready return -1 while waiting "
                    " to unclog. %p \n", "handle_write_ready"));
        }
      return -1;
    }
  return 0;
}

