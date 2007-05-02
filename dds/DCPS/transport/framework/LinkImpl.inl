// -*- C++ -*-
//
// $Id$

#include "LinkImpl.h"
#include <stdexcept>

TAO::DCPS::LinkImpl::IOItem::IOItem(
  ACE_Message_Block& mb,
  char* data,
  size_t size,
  const TransportAPI::Id& requestIdIn
  )
  : mb_(mb, 0)
  , data_begin_(data)
  , data_size_(size)
  , requestId_(requestIdIn)
{
  // TBD!
}

TAO::DCPS::LinkImpl::IOItem::IOItem(
  const TAO::DCPS::LinkImpl::IOItem::IOItem& rhs
  )
  : mb_(rhs.mb_, 0)
  , data_begin_(rhs.data_begin_)
  , data_size_(rhs.data_size_)
  , requestId_(rhs.requestId_)
{
}

TAO::DCPS::LinkImpl::IOItem::~IOItem(
  )
{
}

TAO::DCPS::LinkImpl::LinkImpl(
  TransportAPI::Transport::Link& link,
  size_t max_transport_buffer_size
  )
  : link_(link)
  , max_transport_buffer_size_(max_transport_buffer_size)
  , currentRequestId_(0)
  , connected_(false)
  , queueing_(false)
{
  link_.setCallback(this);
}

TAO::DCPS::LinkImpl::~LinkImpl(
  )
{
  link_.setCallback(0);
}
