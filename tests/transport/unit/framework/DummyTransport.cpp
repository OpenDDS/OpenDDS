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
DummyTransport::getBLOB(TransportAPI::BLOB*& endpoint) const
{
}

size_t
DummyTransport::getMaximumBufferSize() const
{
  return 0;
}

TransportAPI::Status
DummyTransport::isCompatibleEndpoint(TransportAPI::BLOB* endpoint) const
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
{
}

TransportAPI::Status
DummyTransport::Link::setCallback(TransportAPI::LinkCallback* callback)
{
  callback_ = callback;
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::establish(TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId)
{
  log_.push_back(logEntry("establish", requestId));
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::shutdown(const TransportAPI::Id& requestId)
{
  log_.push_back(logEntry("shutdown", requestId));
  return TransportAPI::make_success();
}

TransportAPI::Status
DummyTransport::Link::send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId)
{
  log_.push_back(logEntry("send", requestId));
  return TransportAPI::make_success();
}
