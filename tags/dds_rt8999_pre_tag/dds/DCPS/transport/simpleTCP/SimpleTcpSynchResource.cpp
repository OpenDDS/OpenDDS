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
  : handle_(connection->peer().get_handle()),
    max_output_pause_period_ (max_output_pause_period)
{
  DBG_ENTRY("SimpleTcpSynchResource","SimpleTcpSynchResource");

  // Keep our own "copy" of the reference to the connection.
  connection->_add_ref();
  this->connection_ = connection;
}


TAO::DCPS::SimpleTcpSynchResource::~SimpleTcpSynchResource()
{
  DBG_ENTRY("SimpleTcpSynchResource","~SimpleTcpSynchResource");
}


int
TAO::DCPS::SimpleTcpSynchResource::wait_to_unclog()
{
  DBG_ENTRY("SimpleTcpSynchResource","wait_to_unclog");
 
  ACE_Time_Value* timeout = 0;
  if (this->max_output_pause_period_ != ACE_Time_Value::zero)
    timeout = &this->max_output_pause_period_;
  // Wait for the blocking to subside or timeout.
  if (ACE::handle_write_ready(this->handle_, timeout) == -1)
    {
      if (errno == ETIME)
        {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: handle_write_ready timed out\n"));
          this->connection_->notify_lost_on_backpressure_timeout ();
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

