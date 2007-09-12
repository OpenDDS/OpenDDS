#ifndef MCASTTRANSPORT_H
#define MCASTTRANSPORT_H

#include "mcast_export.h"
#include "dds/DCPS/transport/UDP/UDPTransport.h"
#include <ace/SOCK_Dgram.h>
#include <ace/Task.h>

/// Implements the TransportAPI::Transport interface using MCast.
class MCast_Export MCastTransport: public UDPTransport
{
public:
  virtual std::pair<TransportAPI::Status, TransportAPI::Transport::Link*> establishLink(
    const TransportAPI::BLOB* endpoint,
    const TransportAPI::Id& requestId,
    TransportAPI::LinkCallback* callback,
    bool active
    );
 
protected:
  class Link: public UDPTransport::Link
  {
  public:
    virtual TransportAPI::Status establish(const TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId, bool active);
  };
};

#endif
