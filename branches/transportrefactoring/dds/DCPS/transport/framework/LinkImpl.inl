// -*- C++ -*-
//
// $Id$

#include "LinkImpl.h"
#include <stdexcept>

TAO::DCPS::LinkImpl::IOItem::IOItem()
  : data_begin_(0)
  , data_size_(0)
  , requestId_(0)
  , sequenceNumber_(0)
{
}

TAO::DCPS::LinkImpl::IOItem::IOItem(
  ACE_Message_Block& mb,
  char* data,
  size_t size,
  const TransportAPI::Id& requestIdIn,
  size_t sequenceNumber,
  bool ending
  )
  : mb_(mb.duplicate())
  , data_begin_(data)
  , data_size_(size)
  , requestId_(requestIdIn)
  , sequenceNumber_(sequenceNumber)
  , ending_(ending)
  , deferred_(false)
{
}

TAO::DCPS::LinkImpl::IOItem::IOItem(
  const TAO::DCPS::LinkImpl::IOItem& rhs
  )
  : data_begin_(rhs.data_begin_)
  , data_size_(rhs.data_size_)
  , requestId_(rhs.requestId_)
  , sequenceNumber_(rhs.sequenceNumber_)
  , ending_(rhs.ending_)
  , deferred_(rhs.deferred_)
{
  if (rhs.mb_.get() != 0)
  {
    mb_.reset(rhs.mb_->duplicate());
  }
}

TAO::DCPS::LinkImpl::IOItem::~IOItem(
  )
{
}

TAO::DCPS::LinkImpl::IOItem&
TAO::DCPS::LinkImpl::IOItem::operator=(
  const TAO::DCPS::LinkImpl::IOItem& rhs
  )
{
  if (this != &rhs)
  {
    if (rhs.mb_.get() != 0)
    {
      mb_.reset(rhs.mb_->duplicate());
    }
    else
    {
      mb_.reset();
    }
    data_begin_ = rhs.data_begin_;
    data_size_ = rhs.data_size_;
    requestId_ = rhs.requestId_;
    sequenceNumber_ = rhs.sequenceNumber_;
    ending_ = rhs.ending_;
    deferred_ = rhs.deferred_;
  }
  return *this;
}

TAO::DCPS::LinkImpl::LinkImpl(
  TransportAPI::Transport::Link& link,
  size_t max_transport_buffer_size
  )
  : callback_(0)
  , link_(link)
  , max_transport_buffer_size_(max_transport_buffer_size)
  , connectedCondition_(lock_)
  , condition_(lock_)
  , threadId_(0)
  , currentRequestId_(0)
  , running_(false)
  , shutdown_(false)
  , connected_(false)
  , backpressure_(false)
  , connectionDeferred_(false)
  , deferredConnectionStatus_(TransportAPI::make_failure())
  , lastReceived_(std::make_pair(0, 0))
{
  link_.setCallback(this);
}

TAO::DCPS::LinkImpl::~LinkImpl(
  )
{
  link_.setCallback(0);
  close(0);
}
