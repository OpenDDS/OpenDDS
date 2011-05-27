// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleUdpSocket.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpSocket.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpSocket::~SimpleUdpSocket()
{
  DBG_ENTRY("SimpleUdpSocket","~SimpleUdpSocket");
}


ACE_HANDLE
TAO::DCPS::SimpleUdpSocket::get_handle() const
{
  DBG_ENTRY("SimpleUdpSocket","get_handle");
  return this->socket_.get_handle();
}


int
TAO::DCPS::SimpleUdpSocket::handle_input(ACE_HANDLE)
{
  DBG_ENTRY("SimpleUdpSocket","handle_input");

  if (!this->receive_strategy_.is_nil())
    {
      return this->receive_strategy_->handle_input();
    }

  return 0;
}


int
TAO::DCPS::SimpleUdpSocket::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY("SimpleUdpSocket","handle_close");

  return 0;
}

