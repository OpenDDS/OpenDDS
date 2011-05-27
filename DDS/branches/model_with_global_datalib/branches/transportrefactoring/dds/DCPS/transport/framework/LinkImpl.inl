// -*- C++ -*-
//
// $Id$

#include "LinkImpl.h"
#include <stdexcept>

ACE_INLINE
OpenDDS::DCPS::LinkImpl::IOItem::IOItem()
  : data_begin_(0)
  , data_size_(0)
  , requestId_(0)
  , sequenceNumber_(0)
{
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::IOItem::IOItem(
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

ACE_INLINE
OpenDDS::DCPS::LinkImpl::IOItem::IOItem(
  const OpenDDS::DCPS::LinkImpl::IOItem& rhs
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

ACE_INLINE
OpenDDS::DCPS::LinkImpl::IOItem::~IOItem(
  )
{
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::IOItem&
OpenDDS::DCPS::LinkImpl::IOItem::operator=(
  const OpenDDS::DCPS::LinkImpl::IOItem& rhs
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

ACE_INLINE
OpenDDS::DCPS::LinkImpl::LinkImpl(
  TransportAPI::Transport& transport,
  size_t max_transport_buffer_size
  )
  : callback_(0)
  , transport_(transport)
  , link_(transport_)
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
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::~LinkImpl(
  )
{
  close(0);
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::LinkGuard::LinkGuard(TransportAPI::Transport& transport)
  : transport_(transport)
  , link_(0)
{
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::LinkGuard::~LinkGuard()
{
  tryFree();
}

ACE_INLINE
OpenDDS::DCPS::LinkImpl::LinkGuard&
OpenDDS::DCPS::LinkImpl::LinkGuard::operator=(TransportAPI::Transport::Link* link)
{
  reset(link);
  return *this;
}

ACE_INLINE
void
OpenDDS::DCPS::LinkImpl::LinkGuard::reset(TransportAPI::Transport::Link* link)
{
  tryFree();
  link_ = link;
}

ACE_INLINE
TransportAPI::Transport::Link*
OpenDDS::DCPS::LinkImpl::LinkGuard::operator->() const
{
  return get();
}

ACE_INLINE
TransportAPI::Transport::Link*
OpenDDS::DCPS::LinkImpl::LinkGuard::get() const
{
  return link_;
}

ACE_INLINE
void
OpenDDS::DCPS::LinkImpl::LinkGuard::tryFree()
{
  if (link_ != 0)
  {
    transport_.destroyLink(link_);
    link_ = 0;
  }
}
