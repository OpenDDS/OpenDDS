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

void
OpenDDS::DCPS::LinkImpl::setCallback(
  LinkImplCallback* cb
  )
{
  Guard guard(lock_);
  callback_ = cb;
}

namespace
{
  unsigned int RESERVED_ID = 0;

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
OpenDDS::DCPS::LinkImpl::open(void* args)
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
OpenDDS::DCPS::LinkImpl::close(u_long flags)
{
  {
    Guard guard(lock_);
    if (!running_ || shutdown_)
    {
      return -1;
    }
    shutdown_ = true;
    condition_.signal();
  }
  if (threadId_ != ACE_OS::thr_self())
  {
    wait();
  }
  return 0;
}

int
OpenDDS::DCPS::LinkImpl::svc()
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
OpenDDS::DCPS::LinkImpl::performWork(
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
    bool haveEntry = false;
    {
      Guard guard(extLock);
      if (extract)
      {
        entry = extQueue.front();
        extract = false;
        haveEntry = true;
      }
      connected = extConnected;
      backpressure = extBackpressure;
    }
    if (haveEntry && connected && !backpressure && !entry.deferred_)
    {
      extract = trySending(entry);
      if (extract)
      {
        Guard guard(extLock);
        extQueue.pop_front();
        queueEmpty = extQueue.empty();
      }
      else if (entry.deferred_)
      {
        Guard guard(extLock);
        extQueue.front().deferred_ = true;
      }
    }
    else
    {
      return true;
    }
  }
  return true;
}

TransportAPI::Status
OpenDDS::DCPS::LinkImpl::connect(
  TransportAPI::BLOB* endpoint
  )
{
  Guard guard(lock_);
  if (!connected_)
  {
    TransportAPI::Id requestId(getNextRequestId(guard));
    deferredConnectionStatus_ = TransportAPI::make_deferred();
    connectionDeferred_ = true;
    std::pair<TransportAPI::Status, TransportAPI::Transport::Link*> result =
      transport_.establishLink(endpoint, requestId, this, true /* TBD: active? */ );
    TransportAPI::Status& status = result.first;
    if (status.first == TransportAPI::DEFERRED)
    {
      // Wait for deferred resolution
      status = handleDeferredResolution(connectionDeferred_, connectedCondition_, deferredConnectionStatus_);
    }
    else
    {
      connectionDeferred_ = false;
      connected_ = (status.first == TransportAPI::SUCCESS);
      if (connected_)
      {
        link_.reset(result.second);
      }
    }
    return status;
  }
  return TransportAPI::make_success();
}

TransportAPI::Status
OpenDDS::DCPS::LinkImpl::disconnect(
  )
{
  Guard guard(lock_);
  if (connected_)
  {
    TransportAPI::Id requestId(getNextRequestId(guard));
    deferredConnectionStatus_ = TransportAPI::make_deferred();
    connectionDeferred_ = true;
    TransportAPI::Status status = link_->shutdown(requestId);
    if (status.first == TransportAPI::DEFERRED)
    {
      // Wait for deferred resolution
      status = handleDeferredResolution(connectionDeferred_, connectedCondition_, deferredConnectionStatus_);
    }
    else
    {
      connectionDeferred_ = false;
    }
    transport_.destroyLink(link_.get());
    link_.reset();
    return status;
  }
  return TransportAPI::make_success();
}

TransportAPI::Status
OpenDDS::DCPS::LinkImpl::send(
  ACE_Message_Block& mb,
  TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  requestId = getNextRequestId(guard);
  ACE_Message_Block copy(mb, 1);
  deliver(guard, copy, requestId);
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

    bool operator()(const OpenDDS::DCPS::LinkImpl::IOItem& item)
    {
      if (item.requestId_ == id_)
      {
        if (item.sequenceNumber_ == 0)
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
OpenDDS::DCPS::LinkImpl::recall(
  const TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  bool found = false;
  bool done = false;
  std::deque<OpenDDS::DCPS::LinkImpl::IOItem>::iterator beginIter = queue_.begin();
  std::deque<OpenDDS::DCPS::LinkImpl::IOItem>::iterator endIter = queue_.begin();
  RemoveItemsTest shouldRemove(requestId);

  for (
    std::deque<OpenDDS::DCPS::LinkImpl::IOItem>::iterator iter = queue_.begin();
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
OpenDDS::DCPS::LinkImpl::connected(const TransportAPI::Id& requestId)
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
OpenDDS::DCPS::LinkImpl::disconnected(const TransportAPI::failure_reason& reason)
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
OpenDDS::DCPS::LinkImpl::sendSucceeded(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
  // TBD: check requestId w/front of queue
  bool done = false;
  for (
    std::deque<OpenDDS::DCPS::LinkImpl::IOItem>::iterator iter = queue_.begin();
    iter != queue_.end() && !done;
    ++iter
    )
  {
    // Remove the deferred item
    if ((iter->requestId_ == requestId) && (iter->deferred_))
    {
      queue_.erase(iter);
      done = true;
    }
  }
  condition_.signal();
}

void
OpenDDS::DCPS::LinkImpl::sendFailed(const TransportAPI::failure_reason& reason)
{
 
  Guard guard(lock_);
  // TBD: check requestId w/front of queue
  bool done = false;
  for (
    std::deque<OpenDDS::DCPS::LinkImpl::IOItem>::iterator iter = queue_.begin();
    iter != queue_.end() && !done;
    ++iter
    )
  {
    // Remove the deferred item
    if ((iter->requestId_ == reason.id()) && (iter->deferred_))
    {
      iter->deferred_ = false;
      done = true;
    }
  }
  condition_.signal();
}

void
OpenDDS::DCPS::LinkImpl::backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason)
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

  void
  receivedData(
    const ACE_Message_Block& mb,
    OpenDDS::DCPS::LinkImplCallback* callback
    )
  {
    if (callback != 0)
    {
      callback->receivedData(mb);
    }
  }

  void
  receivedData(
    const OpenDDS::DCPS::LinkImpl::IOItem& item,
    OpenDDS::DCPS::LinkImplCallback* callback
    )
  {
    if (item.mb_.get() != 0)
    {
      receivedData(*(item.mb_.get()), callback);
    }
  }

  void
  clear(OpenDDS::DCPS::LinkImpl::IOItem& cache)
  {
    cache = OpenDDS::DCPS::LinkImpl::IOItem();
  }

  void
  append(
    OpenDDS::DCPS::LinkImpl::IOItem& cache,
    OpenDDS::DCPS::LinkImpl::IOItem& item
    )
  {
    if (cache.mb_.get() == 0)
    {
      cache = item;
    }
    else
    {
      ACE_Message_Block* current = cache.mb_.get();
      while (current->cont() != 0)
      {
        current = current->cont();
      }
      current->cont(item.mb_->duplicate());
    }
  }

  bool inRange(
    const TransportAPI::Id& currentRequestId,
    size_t currentSequenceNumber,
    const TransportAPI::Id& lastRequestId,
    size_t lastSequenceNumber
    )
  {
    if (lastRequestId == RESERVED_ID)
    {
      return true;
    }
    if (currentRequestId > lastRequestId)
    {
      return true;
    }
    else if (currentRequestId < 0x1fffffff && lastRequestId > 0xf0000000)
    {
      return true;
    }
    return false;
  }
}

void
OpenDDS::DCPS::LinkImpl::received(const iovec buffers[], size_t iovecSize)
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

  bool beginning = (sequenceNumber == 0);
  bool ending = (buffer[8] != 0);

  mb.rd_ptr(10);

  IOItem item(
    mb,
    mb.rd_ptr(),
    mb.length(),
    requestId,
    sequenceNumber,
    ending
    );

  if (
    (requestId == lastReceived_.first) &&
    (sequenceNumber == lastReceived_.second + 1)
    )
  {
    append(bufferedData_, item);
    if (ending)
    {
      receivedData(bufferedData_, callback_);
      clear(bufferedData_);
    }
    lastReceived_ = std::make_pair(requestId, sequenceNumber);
  }
  else if (inRange(requestId, sequenceNumber, lastReceived_.first, lastReceived_.second))
  {
    clear(bufferedData_);
    if (beginning)
    {
      if (ending)
      {
        receivedData(mb, callback_);
      }
      else
      {
        append(bufferedData_, item);
      }
      lastReceived_ = std::make_pair(requestId, sequenceNumber);
    }
  }
  // Read header
  // If block is last block + 1
  //   Buffer block
  //   If block is end of block
  //     Deliver all buffered blocks
  //     Clear cache
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
OpenDDS::DCPS::LinkImpl::getNextRequestId(
  const Guard&
  )
{
  ++currentRequestId_;
  if (currentRequestId_ == RESERVED_ID)
  {
    ++currentRequestId_;
  }
  return currentRequestId_;
}

void
OpenDDS::DCPS::LinkImpl::deliver(
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
    return;
  }

  bool enqueued = !queue_.empty();
  DataView::View::iterator first = packets.begin();
  DataView::View::iterator last = packets.end();
  --last;
  for (
    DataView::View::iterator iter = packets.begin();
    iter != packets.end();
    ++iter, ++sequenceNumber
    )
  {
    IOItem item(mb, iter->first, iter->second, requestId, sequenceNumber, iter == last);
    if (
      enqueued ||
      backpressure_ ||
      !connected_ ||
      !trySending(item)
      )
    {
      queue_.push_back(item);
      enqueued = true;
    }
  }
  if (enqueued)
  {
    condition_.signal();
  }
}

bool
OpenDDS::DCPS::LinkImpl::trySending(
  IOItem& item
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
  buffer[8] = (item.ending_ ? 1 : 0);
  buffer[9] = 0; // reserved

  iovs[0].iov_base = reinterpret_cast<char*>(buffer); // TBD: Marshal a simple header into a buffer
  iovs[0].iov_len = sizeof(buffer);
  iovs[1].iov_base = item.data_begin_;
  iovs[1].iov_len = item.data_size_;

  item.deferred_ = true;
  TransportAPI::Status status = link_->send(iovs, 2, item.requestId_);
  item.deferred_ = (status.first == TransportAPI::DEFERRED);
  return status.first == TransportAPI::SUCCESS;
}
