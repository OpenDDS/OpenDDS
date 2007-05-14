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

namespace
{
  template <typename ConditionVariable>
  TransportAPI::Status
  handleDeferredResolution(
    bool& deferred,
    ConditionVariable& condition,
    TransportAPI::Status& status
    )
  {
    while (deferred)
    {
      condition.wait();
    }
    return status;
  }
}

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
    bool extract = true;
    IOItem entry;
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
          entry = queue_.front();
          extract = false;
        }
        queueEmpty = queue_.empty();
        connected = connected_;
        backpressure = backpressure_;
      }
      if (!queueEmpty && connected && !backpressure)
      {
        extract = trySending(entry, false);
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
  if (!connected_)
  {
    TransportAPI::Id requestId(getNextRequestId(guard));
    deferredConnectionStatus_ = TransportAPI::make_deferred();
    connectionDeferred_ = true;
    TransportAPI::Status status = link_.establish(endpoint, requestId);
    if (status.first == TransportAPI::DEFERRED)
    {
      // Wait for deferred resolution
      status = handleDeferredResolution(connectionDeferred_, connectedCondition_, deferredConnectionStatus_);
    }
    else
    {
      connectionDeferred_ = false;
    }
    return status;
  }
  return TransportAPI::make_success();
}

TransportAPI::Status
TAO::DCPS::LinkImpl::disconnect(
  )
{
  Guard guard(lock_);
  if (connected_)
  {
    TransportAPI::Id requestId(getNextRequestId(guard));
    deferredConnectionStatus_ = TransportAPI::make_deferred();
    connectionDeferred_ = true;
    TransportAPI::Status status = link_.shutdown(requestId);
    if (status.first == TransportAPI::DEFERRED)
    {
      // Wait for deferred resolution
      status = handleDeferredResolution(connectionDeferred_, connectedCondition_, deferredConnectionStatus_);
    }
    else
    {
      connectionDeferred_ = false;
    }
    return status;
  }
  return TransportAPI::make_success();
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
  connected_ = true;
  if (connectionDeferred_)
  {
    connectionDeferred_ = false;
    connectedCondition_.signal();
  }
}

void
TAO::DCPS::LinkImpl::disconnected(const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
  connected_ = false;
  if (connectionDeferred_)
  {
    connectionDeferred_ = false;
    connectedCondition_.signal();
  }
}

void
TAO::DCPS::LinkImpl::sendSucceeded(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
  // TBD: check requestId w/front of queue
  if (deferred_)
  {
    deferredStatus_ = TransportAPI::make_success();
  }
  condition_.signal();
}

void
TAO::DCPS::LinkImpl::sendFailed(const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
  // TBD: check requestId w/front of queue
  if (deferred_)
  {
    deferredStatus_ = TransportAPI::make_failure();
  }
  condition_.signal();
}

void
TAO::DCPS::LinkImpl::backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
  backpressure_ = applyBackpressure;
  condition_.signal();
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
  size_t sequenceNumber = 0;
  bool enqueued = false;
  if (packets.empty())
  {
    return true;
  }

  DataView::View::iterator first = packets.begin();
  DataView::View::iterator last = packets.end();
  --last;
  for (DataView::View::iterator iter = packets.begin(); iter != packets.end(); ++iter)
  {
    IOItem item(mb, iter->first, iter->second, requestId, sequenceNumber, iter == first, iter == last);
    if (!queue_.empty())
    {
      queue_.push(item);
      enqueued = true;
    }
    else if (!trySending(item, true))
    {
      queue_.push(item);
      enqueued = true;
    }
    ++sequenceNumber;
  }
  // Only signal if we're connected!
  if (connected_ && enqueued)
  {
    condition_.signal();
  }
  return true;
}

bool
TAO::DCPS::LinkImpl::trySending(
  IOItem& item,
  bool locked
  )
{
  // Process the head of the queue
  unsigned char buffer[10];
  iovec iovs[2];

  buffer[0] = item.requestId_ >> 24;
  buffer[1] = item.requestId_ >> 16;
  buffer[2] = item.requestId_ >> 8;
  buffer[3] = item.requestId_ & 255;
  buffer[4] = item.sequenceNumber_ >> 24;
  buffer[5] = item.sequenceNumber_ >> 16;
  buffer[6] = item.sequenceNumber_ >> 8;
  buffer[7] = item.sequenceNumber_ & 255;
  buffer[8] = (item.beginning_ ? 1 : 0);
  buffer[9] = (item.ending_ ? 1 : 0);

  iovs[0].iov_base = buffer; // TBD: Marshal a simple header into a buffer
  iovs[0].iov_len = sizeof(buffer);
  iovs[1].iov_base = item.data_begin_;
  iovs[1].iov_len = item.data_size_;

  if (!locked)
  {
    Guard guard(lock_);
    deferred_ = true;
    deferredStatus_ = TransportAPI::make_failure();
  }
  else
  {
    deferred_ = true;
    deferredStatus_ = TransportAPI::make_failure();
  }
  TransportAPI::Status status = link_.send(iovs, 2, 0);
  while (status.first == TransportAPI::DEFERRED)
  {
    if (!locked)
    {
      Guard guard(lock_);
      status = handleDeferredResolution(deferred_, condition_, deferredStatus_);
    }
    else
    {
      status = handleDeferredResolution(deferred_, condition_, deferredStatus_);
    }
  }
  if (status.first == TransportAPI::SUCCESS)
  {
    return true;
  }
  return false;
}
