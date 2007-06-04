#include "MCastTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"
#include <ace/SOCK_Dgram_Mcast.h>

TransportAPI::Transport::Link*
MCastTransport::createLink()
{
  return new Link();
}

TransportAPI::Status
MCastTransport::Link::establish(const TransportAPI::BLOB* endpoint,
                                const TransportAPI::Id& requestId)
{
  // Get down to our BLOB type
  const UDPTransport::BLOB* blob = dynamic_cast<const UDPTransport::BLOB*>(endpoint);
  if (blob == 0) {
    return TransportAPI::make_failure(TransportAPI::failure_reason(
                                        "Endpoint is not a MCast/IP endpoint"));
  }

  // Open a socket on  hostname_:port_
  ACE_SOCK_Dgram::PEER_ADDR addr(blob->getPort(),
                                 blob->getHostname().c_str());
  remote_.set(blob->getRemotePort(),
              blob->getHostname().c_str());

  if (blob->getActive()) {
    ACE_SOCK_Dgram* sock = new ACE_SOCK_Dgram();
    if (sock == 0) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          ACE_OS::strerror(errno)));
    }
    local_ = sock;
    if (sock->open(ACE_Addr::sap_any) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to open sap_any "));
    }
    int mcast_ttl_optval = 1;
    sock->set_option(IPPROTO_IP,
                     IP_MULTICAST_TTL,
                     &mcast_ttl_optval,
                     sizeof (mcast_ttl_optval));

    unsigned long ms = blob->getTimeout();
    if (ms > 0) {
      timeout_ = new ACE_Time_Value (ms / 1000, ms % 1000 * 1000);
    }

    // Enable asynchronous I/O
    if (sock->enable(ACE_NONBLOCK) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          ACE_OS::strerror(errno)));
    }
  }
  else {
    ACE_SOCK_Dgram_Mcast* sock = new ACE_SOCK_Dgram_Mcast();
    if (sock == 0) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          ACE_OS::strerror(errno)));
    }
    local_ = sock;
    if (sock->join(addr) == -1) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to join to " +
                                          blob->getHostname()));
    }
    if (activate() != 0) {
      return TransportAPI::make_failure(TransportAPI::failure_reason(
                                          "Unable to activate link"));
    }
  }

  return TransportAPI::make_success();
}

