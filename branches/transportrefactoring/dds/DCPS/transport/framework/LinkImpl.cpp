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
  bool ok = true;
  while (ok)
  {
    ok = performWork(
      lock_,
      condition_,
      shutdown_,
      queue_,
      connected_,
      backpressure_,
      running_
      );
  }
  return 0;
}

bool
TAO::DCPS::LinkImpl::performWork(
  ACE_Thread_Mutex& extLock,
  ACE_Condition<ACE_Thread_Mutex>& extCondition,
  bool& extShutdown,
  std::deque<IOItem>& extQueue,
  bool& extConnected,
  bool& extBackpressure,
  bool& extRunning
  )
{
  bool shutdown = false;
  bool queueEmpty = true;
  bool connected = false;
  bool backpressure = true;
  {
    Guard guard(extLock);
    while (!extShutdown && extQueue.empty() && !extConnected && extBackpressure)
    {
      extCondition.wait();
    }
    shutdown = extShutdown;
    queueEmpty = extQueue.empty();
    connected = extConnected;
    backpressure = extBackpressure;
  }
  if (shutdown)
  {
    extRunning = false;
    return false;
  }
  bool extract = true;
  IOItem entry;
  while (!queueEmpty && connected && !backpressure)
  {
    // Try to send the head of the queue
    // If we get immediate success as the return value, then remove it and
    // try the next element.
    {
      Guard guard(extLock);
      if (extract)
      {
        extQueue.pop_front();
        entry = extQueue.front();
        extract = false;
      }
      queueEmpty = extQueue.empty();
      connected = extConnected;
      backpressure = extBackpressure;
    }
    if (!queueEmpty && connected && !backpressure)
    {
      extract = trySending(entry, false);
    }
  }
  return true;
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
  ACE_Message_Block& mb,
  TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  requestId = getNextRequestId(guard);
  ACE_Message_Block copy(mb, 1);
  if (!deliver(guard, copy, requestId))
  {
    return TransportAPI::make_failure();
  }
  return TransportAPI::make_success();
}

namespace
{
  struct RemoveItemsTest
  {
    RemoveItemsTest(const TransportAPI::Id& id)
      : id_(id)
      , foundBeginning_(false)
    {
    }

    bool operator()(const TAO::DCPS::LinkImpl::IOItem& item)
    {
      if (item.requestId_ == id_)
      {
        if (item.beginning_)
        {
          foundBeginning_ = true;
        }
        return foundBeginning_;
      }
      return false;
    }

    TransportAPI::Id id_;
    bool foundBeginning_;
  };
}

TransportAPI::Status
TAO::DCPS::LinkImpl::recall(
  const TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  bool found = false;
  bool done = false;
  std::deque<TAO::DCPS::LinkImpl::IOItem>::iterator beginIter = queue_.begin();
  std::deque<TAO::DCPS::LinkImpl::IOItem>::iterator endIter = queue_.begin();
  RemoveItemsTest shouldRemove(requestId);

  for (
    std::deque<TAO::DCPS::LinkImpl::IOItem>::iterator iter = queue_.begin();
    iter != queue_.end() && !done;
    ++iter
    )
  {
    if (shouldRemove(*iter))
    {
      if (!found)
      {
        beginIter = iter;
      }
      else
      {
        endIter = iter;
      }
    }
    else if (found)
    {
      done = true;
    }
  }
  if (found)
  {
    ++endIter;
    queue_.erase(beginIter, endIter);
    return TransportAPI::make_success();
  }
  return TransportAPI::make_failure();
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

namespace
{
  void addToAMB(
    void* buffer,
    size_t bufferSize,
    ACE_Message_Block& mb
    )
  {
    std::copy(
      reinterpret_cast<char*>(buffer),
      reinterpret_cast<char*>(buffer) + bufferSize,
      mb.wr_ptr()
      );
    mb.wr_ptr(bufferSize);
  }

  void iovecToAMB(
    const iovec buffers[],
    size_t iovecSize,
    ACE_Message_Block& mb
    )
  {
    for (size_t idx = 0; idx != iovecSize; ++idx)
    {
      addToAMB(buffers[idx].iov_base, buffers[idx].iov_len, mb);
    }
  }

  size_t getTotalSize(
    const iovec buffers[],
    size_t iovecSize
    )
  {
    size_t totalSize = 0;
    for (size_t idx = 0; idx != iovecSize; ++idx)
    {
      totalSize += buffers[idx].iov_len;
    }
    return totalSize;
  }
}

void
TAO::DCPS::LinkImpl::received(const iovec buffers[], size_t iovecSize)
{
  Guard guard(lock_);
  ACE_Message_Block mb(getTotalSize(buffers, iovecSize));
  iovecToAMB(buffers, iovecSize, mb);
  char* buffer = mb.rd_ptr();

  TransportAPI::Id requestId =
    (buffer[0] << 24) +
    (buffer[1] << 16) +
    (buffer[2] << 8) +
    buffer[3];

  size_t sequenceNumber =
    (buffer[4] << 24) +
    (buffer[5] << 16) +
    (buffer[6] << 8) +
    buffer[7];

  bool beginning = (buffer[8] != 0);
  bool ending = (buffer[9] != 0);

  mb.rd_ptr(10);

  IOItem item(
    mb,
    mb.rd_ptr(),
    mb.length(),
    requestId,
    sequenceNumber,
    beginning,
    ending
    );

  if (
    (requestId == lastReceived_.first) &&
    (sequenceNumber == lastReceived_.second + 1)
    )
  {
    if (ending)
    {
      // TBD Deliver
      bufferedData_.clear();
    }
    else
    {
      bufferedData_.push_back(item);
    }
    lastReceived_ = std::make_pair(requestId, sequenceNumber);
  }
  else
  {
    bufferedData_.clear();
    if (beginning)
    {
      if (ending)
      {
        // TBD Deliver
      }
      else
      {
        bufferedData_.push_back(item);
      }
      lastReceived_ = std::make_pair(requestId, sequenceNumber);
    }
  }
  // Read header
  // If block is last block + 1
  //   If block is end of block
  //     Deliver all buffered blocks and the latest
  //     Clear cache
  //   Else
  //     Buffer block
  //   End If
  // Else
  //   Clear cache
  //   If block is beginning
  //     If block is ending
  //       Deliver block
  //     Else
  //       Buffer block
  //     End If
  //   End If
  // End If
}

TransportAPI::Id
TAO::DCPS::LinkImpl::getNextRequestId(
  const Guard&
  )
{
  return ++currentRequestId_;
}

bool
TAO::DCPS::LinkImpl::deliver(
  const Guard&,
  ACE_Message_Block& mb,
  const TransportAPI::Id& requestId
  )
{
  DataView view(mb, max_transport_buffer_size_);
  DataView::View packets;

  view.get(packets);
  size_t sequenceNumber = 0;
  if (packets.empty())
  {
    return true;
  }

  bool enqueued = !queue_.empty();
  DataView::View::iterator first = packets.begin();
  DataView::View::iterator last = packets.end();
  --last;
  for (DataView::View::iterator iter = packets.begin(); iter != packets.end(); ++iter)
  {
    IOItem item(mb, iter->first, iter->second, requestId, sequenceNumber, iter == first, iter == last);
    if (enqueued)
    {
      queue_.push_back(item);
    }
    else if (!trySending(item, true))
    {
      queue_.push_back(item);
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
  typedef unsigned char uchar;

  buffer[0] = uchar(item.requestId_ >> 24);
  buffer[1] = uchar(item.requestId_ >> 16);
  buffer[2] = uchar(item.requestId_ >> 8);
  buffer[3] = uchar(item.requestId_ & 255);
  buffer[4] = (item.sequenceNumber_ >> 24) & 0xff;
  buffer[5] = (item.sequenceNumber_ >> 16) & 0xff;
  buffer[6] = (item.sequenceNumber_ >> 8) & 0xff;
  buffer[7] = item.sequenceNumber_ & 255;
  buffer[8] = (item.beginning_ ? 1 : 0);
  buffer[9] = (item.ending_ ? 1 : 0);

  iovs[0].iov_base = reinterpret_cast<char*>(buffer); // TBD: Marshal a simple header into a buffer
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
  TransportAPI::Status status = link_.send(iovs, 2, item.requestId_);
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
