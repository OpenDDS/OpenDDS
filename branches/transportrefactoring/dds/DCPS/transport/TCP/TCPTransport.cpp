#include "TCPTransport.h"
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>
#include <ace/SOCK_Connector.h>

static const std::string BLOBIdentifier = "TCP";

TCPTransport::TCPTransport()
{
}

TCPTransport::~TCPTransport()
{
}

void
TCPTransport::getBLOB(TransportAPI::BLOB*& endpoint) const
{
  endpoint = new BLOB(hostname_, port_);
}

size_t
TCPTransport::getMaximumBufferSize() const
{
  // TBD
  return 0;
}

TransportAPI::Status
TCPTransport::isCompatibleEndpoint(TransportAPI::BLOB* endpoint) const
{
  if (endpoint != 0 && endpoint->getIdentifier() == BLOBIdentifier) {
    return TransportAPI::make_status();
  }
  return TransportAPI::make_status(false,
                                   TransportAPI::failure_reason(
                                     "Identifier: " +
                                     endpoint->getIdentifier() +
                                     " does not match " + BLOBIdentifier));
}

TransportAPI::Status
TCPTransport::configure(const TransportAPI::NVPList& configuration)
{
  // TBD
  for(size_t i = 0; i < configuration.size(); i++) {
    if (configuration[i].first == "hostname") {
      hostname_ = configuration[i].second;
    }
    else if (configuration[i].first == "port") {
      port_ = configuration[i].second;
    }
  }
}

TransportAPI::Transport::Link*
TCPTransport::createLink()
{
  return new Link();
}

void
TCPTransport::destroyLink(TransportAPI::Transport::Link* link)
{
  Link* tlink = dynamic_cast<TCPTransport::Link*>(link);
  if (tlink != 0) {
    delete tlink;
  }
//  else {
//    throw something;
//  }
}

TCPTransport::BLOB::BLOB(const std::string& hostname,
                         const std::string& port)
{
  setIdentifier(BLOBIdentifier);
  TransportAPI::NVPList& nvplist = getParameters();
  nvplist.push_back(TransportAPI::NVP("hostname", hostname));
  nvplist.push_back(TransportAPI::NVP("port", port));
}

const std::string&
TCPTransport::BLOB::getHostname() const
{
  const TransportAPI::NVPList& nvplist = getParameters();
  for(size_t i = 0; i < nvplist.size(); i++) {
    if (nvplist[i].first == "hostname") {
      return nvplist[i].second;
    }
  }
  static const std::string empty;
  return empty;
}

unsigned short
TCPTransport::BLOB::getPort() const
{
  const TransportAPI::NVPList& nvplist = getParameters();
  for(size_t i = 0; i < nvplist.size(); i++) {
    if (nvplist[i].first == "port") {
      return ACE_OS::atoi(nvplist[i].second.c_str());
    }
  }
  return 0;
}

TCPTransport::Link::Link()
 : callback_(0)
{
}

TCPTransport::Link::~Link()
{
}

TransportAPI::Status
TCPTransport::Link::setCallback(TransportAPI::LinkCallback* callback)
{
  callback_ = callback;
  return TransportAPI::make_status();
}

TransportAPI::Status
TCPTransport::Link::connect(TransportAPI::BLOB* endpoint,
                            const TransportAPI::Id& requestId)
{
  // Get down to our BLOB type
  TCPTransport::BLOB* blob = dynamic_cast<TCPTransport::BLOB*>(endpoint);
  if (blob == 0) {
    return TransportAPI::make_status(false,
                                     TransportAPI::failure_reason(
                                       "Endpoint is not a TCP/IP endpoint"));
  }

  // Connect to the hostname_:port_
  ACE_INET_Addr addr(blob->getPort(), blob->getHostname().c_str());
  ACE_SOCK_Connector connector;
  if (connector.connect(stream_, addr) == -1) {
    return TransportAPI::make_status(false,
                                     TransportAPI::failure_reason(
                                       "Unable to connect to " +
                                       blob->getHostname()));
  }

  // Enable asynchronous I/O
  if (stream_.enable(ACE_NONBLOCK) == -1) {
    return TransportAPI::make_status(false,
                                     TransportAPI::failure_reason(
                                       ACE_OS::strerror(errno)));
  }    

  return TransportAPI::make_status();
}

TransportAPI::Status
TCPTransport::Link::disconnect(const TransportAPI::Id& requestId)
{
  if (stream_.close() == 0) {
    return TransportAPI::make_status();
  }
  return TransportAPI::make_status(false,
                                   TransportAPI::failure_reason(
                                     ACE_OS::strerror(errno)));
}

TransportAPI::Status
TCPTransport::Link::send(const iovec buffers[],
                         size_t iovecSize,
                         const TransportAPI::Id& requestId)
{
  // Count up the total bytes
  ssize_t total = 0;
  for(size_t i = 0; i < iovecSize; i++) {
    total += buffers[i].iov_len;
  }

  // Send with built-in ACE retry 
  if (stream_.sendv_n(buffers, iovecSize) != total) {
    return TransportAPI::make_status(false,
                                     TransportAPI::failure_reason(
                                       "Unable to send iovec "));
  }

  return TransportAPI::make_status();
}
