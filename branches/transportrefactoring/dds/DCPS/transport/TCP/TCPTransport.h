#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "tcp_export.h"
#include "dds/DCPS/transport/framework/Transport.h"
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Stream.h>
#include <ace/Task.h>

/// Implements the TransportAPI::Transport interface using TCP/IP.
class Tcp_Export TCPTransport: public TransportAPI::Transport
{
public:
  TCPTransport();
  virtual ~TCPTransport();

  virtual void getBLOB(const TransportAPI::BLOB*& endpoint) const;
  virtual size_t getMaximumBufferSize() const;
  virtual TransportAPI::Status isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const;
  virtual TransportAPI::Status configure(const TransportAPI::NVPList& configuration);

  virtual TransportAPI::Transport::Link* createLink();
  virtual void destroyLink(TransportAPI::Transport::Link* link);

private:
  class BLOB: public TransportAPI::BLOB
  {
  public:
    BLOB();
    BLOB(const std::string& hostname,
         unsigned short port,
         bool active);
    const std::string& getHostname() const;
    unsigned short getPort() const;
    bool getActive() const;

  private:
    bool active_;
    std::string hostname_;
    unsigned short port_;
  };

  bool active_;
  std::string hostname_;
  unsigned short port_;
  TCPTransport::BLOB endpointConfiguration_;

  class Link: public TransportAPI::Transport::Link,
              public ACE_Task_Base
  {
  public:
    Link();
    virtual ~Link();

    virtual TransportAPI::Status setCallback(TransportAPI::LinkCallback* callback);

    virtual TransportAPI::Status establish(const TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId);
    virtual TransportAPI::Status shutdown(const TransportAPI::Id& requestId);

    virtual TransportAPI::Status send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);

    virtual int svc();

    /// Tell the thread to end itself and wait for it.
    void finish();

  private:
    bool done_;
    TransportAPI::LinkCallback* callback_;
    ACE_SOCK_Stream stream_;
    ACE_SOCK_Stream::PEER_ADDR addr_;
    ACE_SOCK_Acceptor acceptor_;
  };

  // This is necessary for some compilers to access BLOB
  friend class Link;
};

#endif
