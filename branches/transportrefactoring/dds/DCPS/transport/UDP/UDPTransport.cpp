#include "UDPTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"

#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>

static const size_t bufferSize = 8192;
static const std::string BLOBIdentifier = "UDP";

UDPTransport::UDPTransport()
 : active_(false),
   port_(0),
   timeout_(0)
{
}

UDPTransport::~UDPTransport()
{
}

void
UDPTransport::getBLOB(TransportAPI::BLOB*& endpoint) const
{
  endpoint = new BLOB(hostname_, port_, active_, timeout_);
}

size_t
UDPTransport::getMaximumBufferSize() const
{
  return bufferSize;
}

TransportAPI::Status
UDPTransport::isCompatibleEndpoint(TransportAPI::BLOB* endpoint) const
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
UDPTransport::configure(const TransportAPI::NVPList& configuration)
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
    else if (configuration[i].first == "max_output_pause_period") {
      timeout_ = ACE_OS::strtol(configuration[i].second.c_str(), 0, 10);
    }
  }
  if (hostname_.length() != 0 && port_ != 0) {
    return TransportAPI::make_success();
  }
  return TransportAPI::make_failure(TransportAPI::failure_reason(
                                      "Configuration requires a hostname "
                                      "and a port number"));
}

TransportAPI::Transport::Link*
UDPTransport::createLink()
{
  return new Link();
}

void
UDPTransport::destroyLink(TransportAPI::Transport::Link* link)
{
  Link* tlink = dynamic_cast<UDPTransport::Link*>(link);
  if (tlink != 0) {
    tlink->finish();
    delete tlink;
  }
//  else {
//    throw something;
//  }
}

UDPTransport::BLOB::BLOB(const std::string& hostname,
                         unsigned short port,
                         bool active,
                         unsigned long timeout)
 : active_(active),
   hostname_(hostname),
   port_(port),
   timeout_(timeout)
{
  setIdentifier(BLOBIdentifier);
}

const std::string&
UDPTransport::BLOB::getHostname() const
{
  return hostname_;
}

unsigned short
UDPTransport::BLOB::getPort() const
{
  return port_;
}

bool
UDPTransport::BLOB::getActive() const
{
  return active_;
}

unsigned long
UDPTransport::BLOB::getTimeout() const
{
  return timeout_;
}

UDPTransport::Link::Link()
 : done_(false),
   callback_(0),
   timeout_(0)
{
}

UDPTransport::Link::~Link()
{
  finish();
  delete timeout_;
}

TransportAPI::Status
UDPTransport::Link::setCallback(TransportAPI::LinkCallback* callback)
{
  callback_ = callback;
  return TransportAPI::make_success();
}

TransportAPI::Status
UDPTransport::Link::establish(TransportAPI::BLOB* endpoint,
                              const TransportAPI::Id& requestId)
{
  // Get down to our BLOB type
  UDPTransport::BLOB* blob = dynamic_cast<UDPTransport::BLOB*>(endpoint);
  if (blob == 0) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Endpoint is not a UDP/IP endpoint"));
  }

  // Connect to the hostname_:port_
  ACE_INET_Addr addr(blob->getPort(), blob->getHostname().c_str());
  if (local_.open(addr) == -1) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Unable to connect to " +
                                        blob->getHostname()));
  }

  if (blob->getActive()) {
    unsigned long ms = blob->getTimeout();
    if (ms > 0) {
      timeout_ = new ACE_Time_Value (ms / 1000, ms % 1000 * 1000);
    }

    // Enable asynchronous I/O
    if (local_.enable(ACE_NONBLOCK) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          ACE_OS::strerror(errno)));
    }
  }
  else {
    if (activate() != 0) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to activate link"));
    }
  }

  callback_->connected(requestId);
  return TransportAPI::make_success();
}

TransportAPI::Status
UDPTransport::Link::shutdown(const TransportAPI::Id& requestId)
{
  done_ = true;
  if (local_.close() == 0) {
    return TransportAPI::make_success();
  }

  return TransportAPI::make_failure(TransportAPI::failure_reason(
                                      ACE_OS::strerror(errno)));
}

TransportAPI::Status
UDPTransport::Link::send(const iovec buffers[],
                         size_t iovecSize,
                         const TransportAPI::Id& requestId)
{
  for(size_t i = 0; i < iovecSize; i++) {
    // Send with built-in ACE retry
    if (local_.send(buffers[i].iov_base,
                    buffers[i].iov_len,
                    remote_,
                    0,
                    timeout_) != static_cast<ssize_t> (buffers[i].iov_len)) {
      int err = errno;
      TransportAPI::failure_reason
        reason(err == EWOULDBLOCK ?
               "Sending would the iovec would "
               "take longer than the specified timeout" :
               ACE_OS::strerror(err));
      callback_->sendFailed(reason);
      if (err == EWOULDBLOCK) {
        return TransportAPI::make_deferred(reason);
      }
      else {
        return TransportAPI::make_failure(reason);
      }
    }
  }

  callback_->sendSucceeded(requestId);
  return TransportAPI::make_success();
}

int
UDPTransport::Link::svc()
{
  char buffer[bufferSize];
  while(!done_) {
    ssize_t amount = local_.recv(buffer, bufferSize, remote_);
    if (amount == 0) {
      done_ = true;
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
        // TBD: Log this error
      }
    }
  }

  return 0;
}

void
UDPTransport::Link::finish()
{
  done_ = true;
  local_.close();
  wait();
}
