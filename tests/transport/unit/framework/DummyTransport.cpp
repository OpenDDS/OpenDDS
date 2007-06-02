#include "DummyTransport.h"

namespace
{
  Log::value_type
  logEntry(const std::string& string, const TransportAPI::Id& id = 0)
  {
    return std::make_pair(string, id);
  }
}

void
DummyTransport::getBLOB(const TransportAPI::BLOB*& endpoint) const
{
}

size_t
DummyTransport::getMaximumBufferSize() const
{
  return 0;
}

TransportAPI::Status
DummyTransport::isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const
{
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::configure(const TransportAPI::NVPList& configuration)
{
  return TransportAPI::make_success();
}

TransportAPI::Transport::Link*
DummyTransport::createLink()
{
  return new DummyTransport::Link(log_);
}

void
DummyTransport::destroyLink(TransportAPI::Transport::Link* link)
{
  DummyTransport::Link* myLink = dynamic_cast<DummyTransport::Link*>(link);
  delete myLink;
}

DummyTransport::Link::Link(Log& log)
  : callback_(0)
  , log_(log)
  , shouldFailTimes_(0)
  , defer_(false)
{
}

TransportAPI::Status
DummyTransport::Link::setCallback(TransportAPI::LinkCallback* callback)
{
  callback_ = callback;
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::establish(const TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId)
{
  log_.push_back(logEntry("establish", requestId));
  if (defer_)
  {
    return TransportAPI::make_deferred();
  }
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::shutdown(const TransportAPI::Id& requestId)
{
  log_.push_back(logEntry("shutdown", requestId));
  if (defer_)
  {
    return TransportAPI::make_deferred();
  }
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId)
{
  if (shouldFailTimes_ > 0)
  {
    --shouldFailTimes_;
    return TransportAPI::make_failure();
  }
  log_.push_back(logEntry("send", requestId));
  if (defer_)
  {
    return TransportAPI::make_deferred();
  }
  return TransportAPI::make_success();
}
