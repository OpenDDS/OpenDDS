// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "EventHandler.h"
#include <stdexcept>

#if !defined (__ACE_INLINE__)
#include "EventHandler.inl"
#endif /* __ACE_INLINE__ */

void
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::close()
{
  if (socket_.get_handle() != ACE_INVALID_HANDLE)
  {
    reactor()->remove_handler(
      this,
      ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL
      );
    socket_.close();
  }
}

void
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::send(
  char* buffer,
  size_t size,
  const ACE_INET_Addr& dest
  )
{
  bool reregister =  false;
  {
    ACE_Guard<ACE_Thread_Mutex> lock(output_mutex_);
    reregister = output_queue_.empty();

    output_queue_.push(std::make_pair(std::string(buffer, size), dest));
  }
  if (reregister)
  {
    if (reactor()->register_handler(
      this,
      ACE_Event_Handler::WRITE_MASK
      ) == -1)
    {
      throw std::runtime_error("failure to register_handler");
    }
  }
}

ACE_HANDLE
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::get_handle() const
{
  return socket_.get_handle();
}

ACE_SOCK&
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::socket()
{
  return socket_;
}

int
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::handle_input(
  ACE_HANDLE fd
  )
{
  ACE_UNUSED_ARG(fd);

  char buffer[8192];
  ACE_INET_Addr peer;

  ssize_t bytes_read = socket_.recv(
    static_cast<void*>(buffer), sizeof(buffer), peer
    );
  if (bytes_read > 0)
  {
    ACE_Guard<ACE_Thread_Mutex> lock(input_mutex_);
    receive(buffer, bytes_read, peer);
  }
  return 0;
}

int
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::handle_output(
  ACE_HANDLE fd
  )
{
  ACE_UNUSED_ARG(fd);

  ACE_Guard<ACE_Thread_Mutex> lock(output_mutex_);
  if (!output_queue_.empty())
  {
    Queue::value_type& item = output_queue_.front();
    ssize_t bytes_sent = socket_.ACE_SOCK_Dgram::send(
      static_cast<const void*>(item.first.data()),
      item.first.size(),
      item.second
      );

    if (size_t(bytes_sent) == item.first.size())
    {
      output_queue_.pop();
    }
    else if (bytes_sent > 0)
    {
      item.first = item.first.substr(bytes_sent);
    }
  }
  return output_queue_.empty() ? -1 : 0;
}

int
OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::handle_close(
  ACE_HANDLE fd,
  ACE_Reactor_Mask mask
  )
{
  ACE_UNUSED_ARG(fd);
  ACE_UNUSED_ARG(mask);

  return 0;
}
