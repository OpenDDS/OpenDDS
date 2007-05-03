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
  return TransportAPI::make_status();
}

void
TAO::DCPS::LinkImpl::connected(const TransportAPI::Id& requestId)
{
  Guard guard(lock_);
  connected_ = true;
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
  queueing_ = applyBackpressure;
}

void
TAO::DCPS::LinkImpl::received(const iovec buffers[], size_t iovecSize)
{
  Guard guard(lock_);
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
  return true;
}
