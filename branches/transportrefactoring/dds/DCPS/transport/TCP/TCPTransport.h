#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "dds/DCPS/transport/framework/Transport.h"
#include <ace/SOCK_Stream.h>

/// Implements the TransportAPI::Transport interface using TCP/IP.
class TCPTransport: public TransportAPI::Transport
{
public:
  TCPTransport();
  virtual ~TCPTransport();

  virtual void getBLOB(TransportAPI::BLOB*& endpoint) const;
  virtual size_t getMaximumBufferSize() const;
  virtual TransportAPI::Status isCompatibleEndpoint(TransportAPI::BLOB* endpoint) const;
  virtual TransportAPI::Status configure(const TransportAPI::NVPList& configuration);

  virtual TransportAPI::Transport::Link* createLink();
  virtual void destroyLink(TransportAPI::Transport::Link* link);

private:
  std::string hostname_;
  std::string port_;

  class BLOB: public TransportAPI::BLOB
  {
  public:
    BLOB(const std::string& hostname, const std::string& port);
    const std::string& getHostname() const;
    unsigned short getPort() const;
  };

  class Link: public TransportAPI::Transport::Link
  {
  public:
    Link();
    virtual ~Link();

    virtual TransportAPI::Status setCallback(TransportAPI::LinkCallback* callback);

    virtual TransportAPI::Status connect(TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId);
    virtual TransportAPI::Status disconnect(const TransportAPI::Id& requestId);

    virtual TransportAPI::Status send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);

  private:
    TransportAPI::LinkCallback* callback_;
    ACE_SOCK_Stream stream_;
  };

  // This is necessary for some compilers to access BLOB
  friend class Link;
};

#endif
