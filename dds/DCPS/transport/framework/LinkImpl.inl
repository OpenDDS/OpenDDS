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
  size_t sequenceNumber
  )
  : mb_(new ACE_Message_Block(mb, 0))
  , data_begin_(data)
  , data_size_(size)
  , requestId_(requestIdIn)
  , sequenceNumber_(sequenceNumber)
{
  // TBD!
}

TAO::DCPS::LinkImpl::IOItem::IOItem(
  const TAO::DCPS::LinkImpl::IOItem::IOItem& rhs
  )
  : mb_(new ACE_Message_Block(*rhs.mb_, 0))
  , data_begin_(rhs.data_begin_)
  , data_size_(rhs.data_size_)
  , requestId_(rhs.requestId_)
  , sequenceNumber_(rhs.sequenceNumber_)
{
}

TAO::DCPS::LinkImpl::IOItem::~IOItem(
  )
{
}

TAO::DCPS::LinkImpl::IOItem&
TAO::DCPS::LinkImpl::IOItem::operator=(
  const TAO::DCPS::LinkImpl::IOItem::IOItem& rhs
  )
{
  if (this != &rhs)
  {
    mb_.reset(new ACE_Message_Block(*rhs.mb_, 0));
    data_begin_ = rhs.data_begin_;
    data_size_ = rhs.data_size_;
    requestId_ = rhs.requestId_;
    sequenceNumber_ = rhs.sequenceNumber_;
  }
  return *this;
}

TAO::DCPS::LinkImpl::LinkImpl(
  TransportAPI::Transport::Link& link,
  size_t max_transport_buffer_size
  )
  : link_(link)
  , max_transport_buffer_size_(max_transport_buffer_size)
  , condition_(lock_)
  , threadId_(0)
  , currentRequestId_(0)
  , running_(false)
  , shutdown_(false)
  , connected_(false)
  , backpressure_(false)
  , deferred_(false)
  , deferredStatus_(TransportAPI::make_failure())
{
  link_.setCallback(this);
}

TAO::DCPS::LinkImpl::~LinkImpl(
  )
{
  link_.setCallback(0);
}
