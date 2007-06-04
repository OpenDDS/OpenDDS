#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include "udp_export.h"
#include "dds/DCPS/transport/framework/Transport.h"
#include <ace/SOCK_Dgram.h>
#include <ace/Task.h>

/// Implements the TransportAPI::Transport interface using UDP.
class Udp_Export UDPTransport: public TransportAPI::Transport
{
public:
  UDPTransport();
  virtual ~UDPTransport();

  virtual void getBLOB(const TransportAPI::BLOB*& endpoint) const;
  virtual size_t getMaximumBufferSize() const;
  virtual TransportAPI::Status isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const;
  virtual TransportAPI::Status configure(const TransportAPI::NVPList& configuration);

  virtual TransportAPI::Transport::Link* createLink();
  virtual void destroyLink(TransportAPI::Transport::Link* link);

public:
  class BLOB: public TransportAPI::BLOB
  {
  public:
    BLOB();
    BLOB(const std::string& hostname,
         unsigned short port,
         const std::string& remoteHostname,
         unsigned short remotePort,
         bool active,
         unsigned long);
    const std::string& getHostname() const;
    unsigned short getPort() const;
    const std::string& getRemoteHostname() const;
    unsigned short getRemotePort() const;
    bool getActive() const;
    unsigned long getTimeout() const;

  private:
    bool active_;
    std::string hostname_;
    unsigned short port_;
    std::string remoteHostname_;
    unsigned short remotePort_;
    unsigned long timeout_;
  };

private:
  bool active_;
  std::string hostname_;
  unsigned short port_;
  std::string remoteHostname_;
  unsigned short remotePort_;
  unsigned long timeout_;
  UDPTransport::BLOB endpointConfiguration_;

public:
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

  protected:
    ACE_Atomic_Op<ACE_SYNCH_MUTEX, bool> done_;
    TransportAPI::LinkCallback* callback_;
    ACE_SOCK_Dgram* local_;
    ACE_SOCK_Dgram::PEER_ADDR remote_;
    ACE_Time_Value* timeout_;
  };

  // This is necessary for some compilers to access BLOB
  friend class Link;
};

#endif
