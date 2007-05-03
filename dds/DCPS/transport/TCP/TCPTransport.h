#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "dds/DCPS/transport/framework/Transport.h"
#include <ace/SOCK_Stream.h>
#include <ace/Task.h>

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
  bool active_;
  std::string hostname_;
  unsigned short port_;

  class BLOB: public TransportAPI::BLOB
  {
  public:
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

  class Link: public TransportAPI::Transport::Link,
              public ACE_Task_Base
  {
  public:
    Link();
    virtual ~Link();

    virtual TransportAPI::Status setCallback(TransportAPI::LinkCallback* callback);

    virtual TransportAPI::Status connect(TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId);
    virtual TransportAPI::Status disconnect(const TransportAPI::Id& requestId);

    virtual TransportAPI::Status send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);

    virtual int svc();

    /// Tell the thread to end itself and wait for it.
    void finish();

  private:
    bool done_;
    TransportAPI::LinkCallback* callback_;
    ACE_SOCK_Stream stream_;
  };

  // This is necessary for some compilers to access BLOB
  friend class Link;
};

#endif
