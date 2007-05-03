#include "TCPTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"

#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>
#include <ace/SOCK_Connector.h>

static const size_t bufferSize = 8192;
static const std::string BLOBIdentifier = "TCP";

TCPTransport::TCPTransport()
 : active_(false),
   port_(0)
{
}

TCPTransport::~TCPTransport()
{
}

void
TCPTransport::getBLOB(TransportAPI::BLOB*& endpoint) const
{
  endpoint = new BLOB(hostname_, port_, active_);
}

size_t
TCPTransport::getMaximumBufferSize() const
{
  return bufferSize;
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
  for(size_t i = 0; i < configuration.size(); i++) {
    if (configuration[i].first == "hostname") {
      hostname_ = configuration[i].second;
    }
    else if (configuration[i].first == "port") {
      port_ = ACE_OS::atoi(configuration[i].second.c_str());
    }
    else if (configuration[i].first == "active") {
      active_ = ACE_OS::atoi(configuration[i].second.c_str());
    }
  }
  if (hostname_.length() != 0 && port_ != 0) {
    return TransportAPI::make_status();
  }
  return TransportAPI::make_status(false,
                                   TransportAPI::failure_reason(
                                     "Configuration requires a hostname "
                                     "and a port number"));
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
    tlink->finish();
    delete tlink;
  }
//  else {
//    throw something;
//  }
}

TCPTransport::BLOB::BLOB(const std::string& hostname,
                         unsigned short port,
                         bool active)
 : active_(active),
   hostname_(hostname),
   port_(port)
{
  setIdentifier(BLOBIdentifier);
}

const std::string&
TCPTransport::BLOB::getHostname() const
{
  return hostname_;
}

unsigned short
TCPTransport::BLOB::getPort() const
{
  return port_;
}

bool
TCPTransport::BLOB::getActive() const
{
  return active_;
}

TCPTransport::Link::Link()
 : done_(false),
   callback_(0)
{
}

TCPTransport::Link::~Link()
{
  finish();
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

  if (blob->getActive()) {
    // Enable asynchronous I/O
    if (stream_.enable(ACE_NONBLOCK) == -1) {
      return TransportAPI::make_status(false,
                                       TransportAPI::failure_reason(
                                         ACE_OS::strerror(errno)));
    }
  }
  else {
    if (activate() != 0) {
      return TransportAPI::make_status(false,
                                       TransportAPI::failure_reason(
                                         "Unable to activate link"));
    }
  }

  callback_->connected(requestId);
  return TransportAPI::make_status();
}

TransportAPI::Status
TCPTransport::Link::disconnect(const TransportAPI::Id& requestId)
{
  if (stream_.close() == 0) {
    done_ = true;
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
    TransportAPI::failure_reason reason("Unable to send iovec ");
    callback_->sendFailed(reason);
    return TransportAPI::make_status(false, reason);
  }

  callback_->sendSucceeded(requestId);
  return TransportAPI::make_status();
}

int
TCPTransport::Link::svc()
{
  char buffer[bufferSize];
  while(!done_) {
    ssize_t amount = stream_.recv(buffer, bufferSize);
    if (amount == 0) {
      done_ = true;
    }
    else if (amount > 0) {
      // It may be better for the callback to take just
      // a character buffer
      iovec iov[1];
      iov[0].iov_len  = amount;
      iov[0].iov_base = buffer;
      callback_->received(iov, 1);
    }
    else {
      if (errno != EINTR) {
        done_ = true;
        // TBD: Log this error
      }
    }
  }
  return 0;
}

void
TCPTransport::Link::finish()
{
  stream_.close();
  done_ = true;
  wait();
}
