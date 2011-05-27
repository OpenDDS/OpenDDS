#include "TCPTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"

#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>
#include <ace/SOCK_Connector.h>

namespace
{
  const size_t bufferSize = 8192;
  const std::string BLOBIdentifier = "TCP";
}

TCPTransport::TCPTransport()
 : active_(false),
   port_(0)
{
}

TCPTransport::~TCPTransport()
{
}

void
TCPTransport::getBLOB(const TransportAPI::BLOB*& endpoint) const
{
  endpoint = &endpointConfiguration_;
}

size_t
TCPTransport::getMaximumBufferSize() const
{
  return bufferSize;
}

TransportAPI::Status
TCPTransport::isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const
{
  if (endpoint != 0 && endpoint->getIdentifier() == BLOBIdentifier) {
    return TransportAPI::make_success();
  }
  return TransportAPI::make_failure(TransportAPI::failure_reason(
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
    endpointConfiguration_ = BLOB(hostname_, port_, active_);
    return TransportAPI::make_success();
  }
  return TransportAPI::make_failure(TransportAPI::failure_reason(
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

TCPTransport::BLOB::BLOB()
  : active_(false),
    port_(0)
{
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
  return TransportAPI::make_success();
}

TransportAPI::Status
TCPTransport::Link::establish(const TransportAPI::BLOB* endpoint,
                              const TransportAPI::Id& requestId)
{
  // Get down to our BLOB type
  const TCPTransport::BLOB* blob = dynamic_cast<const TCPTransport::BLOB*>(endpoint);
  if (blob == 0) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Endpoint is not a TCP/IP endpoint"));
  }

  ACE_SOCK_Stream::PEER_ADDR addr(blob->getPort(),
                                  blob->getHostname().c_str());
  if (blob->getActive()) {
    // Connect to the hostname_:port_
    ACE_SOCK_Connector connector;
    if (connector.connect(stream_, addr) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to connect to " +
                                          blob->getHostname()));
    }

    // Enable asynchronous I/O
    if (stream_.enable(ACE_NONBLOCK) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          ACE_OS::strerror(errno)));
    }
  }
  else {
    addr_ = addr;
    if (activate() != 0) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to activate link"));
    }
  }

  return TransportAPI::make_success();
}

TransportAPI::Status
TCPTransport::Link::shutdown(const TransportAPI::Id& requestId)
{
  done_ = true;
  if (stream_.close() == 0) {
    return TransportAPI::make_success();
  }

  return TransportAPI::make_failure(TransportAPI::failure_reason(
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
    return TransportAPI::make_failure(reason);
  }

  return TransportAPI::make_success();
}

int
TCPTransport::Link::svc()
{
  if (acceptor_.open(addr_) == 0 && acceptor_.accept(stream_) == 0) {
    char buffer[bufferSize];
    while(!done_) {
      ssize_t amount = stream_.recv(buffer, bufferSize);
      if (amount == 0) {
        done_ = true;
        callback_->disconnected(TransportAPI::failure_reason());
      }
      else if (amount > 0) {
        iovec iov[1];
        iov[0].iov_len  = amount;
        iov[0].iov_base = buffer;
        try {
          callback_->received(iov, 1);
        }
        catch(const std::exception& ex) {
          // TBD: Log this exception
        }
        catch(...) {
          // TBD: Log this exception
        }
      }
      else {
        if (errno != EINTR) {
          done_ = true;
          callback_->disconnected(TransportAPI::failure_reason(
                                    ACE_OS::strerror(errno)));
          // TBD: Log this error
        }
      }
    }
    acceptor_.close();
  }
  else {
    // TBD: Log this error
  }
  return 0;
}

void
TCPTransport::Link::finish()
{
  done_ = true;
  stream_.close();
  wait();
}
