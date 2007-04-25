// -*- C++ -*-
//
// $Id$

#include "LinkImpl.h"
#include <stdexcept>

TAO::DCPS::LinkImpl::IOItem::IOItem(
  const TransportAPI::iovec buffersIn[],
  size_t iovecSizeIn,
  const TransportAPI::Id& requestIdIn
  )
  : buffers(&buffersIn[0])
  , iovecSize(iovecSizeIn)
  , requestId(requestIdIn)
{
  // TBD!
}

TAO::DCPS::LinkImpl::IOItem::~IOItem(
  )
{
  delete [] buffers;
}

TAO::DCPS::LinkImpl::LinkImpl(
  TransportAPI::Transport::Link& link
  )
  : link_(link)
  , queueing_(false)
{
  link_.setCallback(this);
}

TAO::DCPS::LinkImpl::~LinkImpl(
  )
{
  link_.setCallback(0);
}
