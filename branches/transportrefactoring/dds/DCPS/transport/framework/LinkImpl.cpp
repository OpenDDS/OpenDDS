// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "LinkImpl.h"

#if !defined (__ACE_INLINE__)
#include "LinkImpl.inl"
#endif /* __ACE_INLINE__ */

#include "ace/Guard_T.h"

TransportAPI::Status
TAO::DCPS::LinkImpl::connect(
  TransportAPI::BLOB* endpoint,
  const TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  return link_.connect(endpoint, requestId);
}

TransportAPI::Status
TAO::DCPS::LinkImpl::disconnect(
  const TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  return link_.disconnect(requestId);
}

TransportAPI::Status
TAO::DCPS::LinkImpl::send(
  const TransportAPI::iovec buffers[],
  size_t iovecSize,
  const TransportAPI::Id& requestId
  )
{
  Guard guard(lock_);
  if (!queueing_ && queue_.empty())
  {
    return link_.send(buffers, iovecSize, requestId);
  }
  else
  {
    enqueue(guard, buffers, iovecSize, requestId);
  }
}

void
TAO::DCPS::LinkImpl::connected(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
}

void
TAO::DCPS::LinkImpl::disconnected(const TransportAPI::failure_reason& reason)
{
  Guard guard(lock_);
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
  queueing_ = applyBackpressure;
}

void
TAO::DCPS::LinkImpl::received(const TransportAPI::iovec buffers[], size_t iovecSize)
{
  Guard guard(lock_);
}

void
TAO::DCPS::LinkImpl::enqueue(
  const Guard&,
  const TransportAPI::iovec buffers[],
  size_t iovecSize,
  const TransportAPI::Id& requestId
  )
{
  IOItem item(buffers, iovecSize, requestId);
  queue_.push(item);
}
