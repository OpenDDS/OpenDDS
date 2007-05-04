// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "LinkImpl.h"

#if !defined (__ACE_INLINE__)
#include "LinkImpl.inl"
#endif /* __ACE_INLINE__ */

#include "DataView.h"
#include "ace/Guard_T.h"

int
TAO::DCPS::LinkImpl::open(void* args)
{
  Guard guard(lock_);
  if (running_)
  {
    return -1;
  }
  running_ = (activate() == 0);

  return running_ ? 0 : -1;
}

int
TAO::DCPS::LinkImpl::close(u_long flags)
{
  Guard guard(lock_);
  if (!running_ || shutdown_)
  {
    return -1;
  }
  shutdown_ = true;
  condition_.signal();
  if (threadId_ != ACE_OS::thr_self())
  {
    wait();
  }
  return 0;
}

int
TAO::DCPS::LinkImpl::svc()
{
  {
    Guard guard(lock_);
    threadId_ = ACE_OS::thr_self();
  }
  while (true)
  {
    bool shutdown = false;
    bool queueEmpty = true;
    bool connected = false;
    bool backpressure = true;
    {
      Guard guard(lock_);
      while (!shutdown_ && queue_.empty() && !connected_ && backpressure_)
      {
        condition_.wait();
      }
      shutdown = shutdown_;
      queueEmpty = queue_.empty();
      connected = connected_;
      backpressure = backpressure_;
    }
    if (shutdown)
    {
      running_ = false;
      return 0;
    }
    bool extract = false;
    while (!queueEmpty && connected && !backpressure)
    {
      // Try to send the head of the queue
      // If we get immediate success as the return value, then remove it and
      // try the next element.
      {
        Guard guard(lock_);
        if (extract)
        {
          queue_.pop();
          //entry = queue_.front();
        }
        queueEmpty = queue_.empty();
        connected = connected_;
        backpressure = backpressure_;
      }
      if (!queueEmpty)
      {
        // Process the head of the queue
      }
    }
  }
  return 0;
}

TransportAPI::Status
TAO::DCPS::LinkImpl::connect(
  TransportAPI::BLOB* endpoint
  )
{
  Guard guard(lock_);
  TransportAPI::Id requestId(getNextRequestId(guard));
  return link_.establish(endpoint, requestId);
}

TransportAPI::Status
TAO::DCPS::LinkImpl::disconnect(
  )
{
  Guard guard(lock_);
  TransportAPI::Id requestId(getNextRequestId(guard));
  return link_.shutdown(requestId);
}

TransportAPI::Status
TAO::DCPS::LinkImpl::send(
  ACE_Message_Block& mb
  )
{
  Guard guard(lock_);
  TransportAPI::Id requestId(getNextRequestId(guard));
  ACE_Message_Block copy(mb, 0);
  if (!enqueue(guard, copy, requestId))
  {
    // Error enqueueing request
  }
  return TransportAPI::make_success();
}

void
TAO::DCPS::LinkImpl::connected(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
  if (connected_)
  {
    return;
  }
  connected_ = true;
  condition_.signal();
}

void
TAO::DCPS::LinkImpl::disconnected(const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
  connected_ = false;
}

void
TAO::DCPS::LinkImpl::sendSucceeded(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
}

void
TAO::DCPS::LinkImpl::sendFailed(const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
}

void
TAO::DCPS::LinkImpl::backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
}

void
TAO::DCPS::LinkImpl::received(const iovec buffers[], size_t iovecSize)
{
  Guard guard(lock_);
  ACE_Message_Block mb;
}

TransportAPI::Id
TAO::DCPS::LinkImpl::getNextRequestId(
  const Guard&
  )
{
  return ++currentRequestId_;
}

bool
TAO::DCPS::LinkImpl::enqueue(
  const Guard&,
  ACE_Message_Block& mb,
  const TransportAPI::Id& requestId
  )
{
  DataView view(mb, max_transport_buffer_size_);
  DataView::View packets;

  view.get(packets);
  for (DataView::View::iterator iter = packets.begin(); iter != packets.end(); ++iter)
  {
    IOItem item(mb, iter->first, iter->second, requestId);
    queue_.push(item);
  }
  // Only signal if we're connected!
  if (connected_)
  {
    condition_.signal();
  }
  return true;
}
