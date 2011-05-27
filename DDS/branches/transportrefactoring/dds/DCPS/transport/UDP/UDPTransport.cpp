#include "UDPTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"

#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>

namespace
{
  const size_t bufferSize = 8192;
  const std::string BLOBIdentifier = "UDP";
}

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
UDPTransport::getBLOB(const TransportAPI::BLOB*& endpoint) const
{
  endpoint = &endpointConfiguration_;
}

size_t
UDPTransport::getMaximumBufferSize() const
{
  return bufferSize;
}

TransportAPI::Status
UDPTransport::isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const
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
    else if (configuration[i].first == "remoteHostname") {
      remoteHostname_ = configuration[i].second;
    }
    else if (configuration[i].first == "remotePort") {
      remotePort_ = ACE_OS::atoi(configuration[i].second.c_str());
    }
    else if (configuration[i].first == "max_output_pause_period") {
      timeout_ = ACE_OS::strtol(configuration[i].second.c_str(), 0, 10);
    }
  }
  if (hostname_.length() != 0 && port_ != 0 &&
      remoteHostname_.length() != 0 && remotePort_ != 0) {
    endpointConfiguration_ =
      BLOB(hostname_, port_, remoteHostname_, remotePort_,
                      timeout_);
    return TransportAPI::make_success();
  }
  return TransportAPI::make_failure(TransportAPI::failure_reason(
                                      "Configuration requires a hostname "
                                      "and a port number"));
}

std::pair<TransportAPI::Status, TransportAPI::Transport::Link*>
UDPTransport::establishLink(
  const TransportAPI::BLOB* endpoint,
  const TransportAPI::Id& requestId,
  TransportAPI::LinkCallback* callback,
  bool active
  )
{
  TransportAPI::Status status = TransportAPI::make_failure(
    TransportAPI::failure_reason("Invalid BLOB")
    );
  Link* link = 0;
  const UDPTransport::BLOB* blob = dynamic_cast<const UDPTransport::BLOB*>(endpoint);

  if (blob != 0)
  {
    link = new UDPTransport::Link;
    link->setCallback(callback);
    status = link->establish(endpoint, requestId, active);
  }
  return std::make_pair(status, link);
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

UDPTransport::BLOB::BLOB()
  : port_(0),
    remotePort_(0),
    timeout_(0)
{
  setIdentifier(BLOBIdentifier);
}

UDPTransport::BLOB::BLOB(const std::string& hostname,
                         unsigned short port,
                         const std::string& remoteHostname,
                         unsigned short remotePort,
                         unsigned long timeout)
 : hostname_(hostname),
   port_(port),
   remoteHostname_(remoteHostname),
   remotePort_(remotePort),
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

const std::string&
UDPTransport::BLOB::getRemoteHostname() const
{
  return remoteHostname_;
}

unsigned short
UDPTransport::BLOB::getRemotePort() const
{
  return remotePort_;
}

unsigned long
UDPTransport::BLOB::getTimeout() const
{
  return timeout_;
}

UDPTransport::Link::Link()
 : done_(false),
   callback_(0),
   local_(0),
   timeout_(0)
{
}

UDPTransport::Link::~Link()
{
  finish();
  delete local_;
  delete timeout_;
}

TransportAPI::Status
UDPTransport::Link::setCallback(TransportAPI::LinkCallback* callback)
{
  callback_ = callback;
  return TransportAPI::make_success();
}

TransportAPI::Status
UDPTransport::Link::establish(const TransportAPI::BLOB* endpoint,
                              const TransportAPI::Id& requestId,
                              bool active)
{
  // Get down to our BLOB type
  const UDPTransport::BLOB* blob = dynamic_cast<const UDPTransport::BLOB*>(endpoint);
  if (blob == 0) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Endpoint is not a UDP/IP endpoint"));
  }

  // Open a socket on  hostname_:port_
  ACE_SOCK_Dgram::PEER_ADDR addr(blob->getPort(),
                                 blob->getHostname().c_str());
  local_ = new ACE_SOCK_Dgram();
  if (local_->open(addr) == -1) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Unable to connect to " +
                                        blob->getHostname()));
  }
  remote_.set(blob->getRemotePort(),
              blob->getHostname().c_str());

  if (active) {
    unsigned long ms = blob->getTimeout();
    if (ms > 0) {
      timeout_ = new ACE_Time_Value (ms / 1000, ms % 1000 * 1000);
    }

    // Enable asynchronous I/O
    if (local_->enable(ACE_NONBLOCK) == -1) {
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

  return TransportAPI::make_success();
}

TransportAPI::Status
UDPTransport::Link::shutdown(const TransportAPI::Id& requestId)
{
  done_ = true;
  if (local_ == 0 || local_->close() == 0) {
    delete local_;
    local_ = 0;
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
  if (local_ == 0) {
    TransportAPI::failure_reason reason("Link has not yet been established");
    return TransportAPI::make_failure(reason);
  }

  size_t totalSize = 0;
  for (size_t i = 0; i < iovecSize; ++i)
  {
    totalSize += buffers[i].iov_len;
  }

  if (local_->send(buffers,
                   static_cast<int>(iovecSize),
                   remote_,
                   0) != static_cast<ssize_t>(totalSize)) {
    int err = errno;
    TransportAPI::failure_reason
      reason(err == EWOULDBLOCK || err == ETIME ?
             "Sending would the iovec would "
             "take longer than the specified timeout" :
             ACE_OS::strerror(err));
    return TransportAPI::make_failure(reason);
  }

  return TransportAPI::make_success();
}

int
UDPTransport::Link::svc()
{
  if (local_ == 0) {
    // TBD: Log this error
    return 1;
  }

  char buffer[bufferSize];
  const ACE_Time_Value timeout(0, 500000);
  while(done_ == false) {
    ssize_t amount = local_->recv(buffer, bufferSize, remote_,
                                  0, &timeout);

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
      if (errno == EINTR) {
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
  if (local_ != 0) {
    local_->close();
  }
  wait();
}
