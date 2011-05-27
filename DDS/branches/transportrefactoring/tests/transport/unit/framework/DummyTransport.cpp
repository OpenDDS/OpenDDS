#include "DummyTransport.h"

namespace
{
  Log::value_type
  logEntry(const std::string& string, const TransportAPI::Id& id = 0)
  {
    return std::make_pair(string, id);
  }
}

DummyTransport::DummyTransport()
  : shouldFailTimes_(0)
  , defer_(false)
{
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

std::pair<TransportAPI::Status, TransportAPI::Transport::Link*>
DummyTransport::establishLink(
  const TransportAPI::BLOB* endpoint,
  const TransportAPI::Id& requestId,
  TransportAPI::LinkCallback* callback,
  bool active
  )
{
  log_.push_back(logEntry("establishLink", requestId));
  TransportAPI::Transport::Link* link = new DummyTransport::Link(log_, shouldFailTimes_, defer_, callback);
  if (defer_)
  {
    return std::make_pair(TransportAPI::make_deferred(), link);
  }
  return std::make_pair(TransportAPI::make_success(), link);
}

void
DummyTransport::destroyLink(TransportAPI::Transport::Link* link)
{
  DummyTransport::Link* myLink = dynamic_cast<DummyTransport::Link*>(link);
  delete myLink;
}

DummyTransport::Link::Link(Log& log, unsigned int& shouldFailTimes, bool& defer, TransportAPI::LinkCallback* callback)
  : callback_(callback)
  , log_(log)
  , shouldFailTimes_(shouldFailTimes)
  , defer_(defer)
{
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
