#ifndef DUMMYTRANSPORT_H
#define DUMMYTRANSPORT_H

#include "dds/DCPS/transport/framework/Transport.h"
#include <vector>

typedef std::vector<std::pair<std::string, TransportAPI::Id> > Log;

/// Transport
class DummyTransport
  : public TransportAPI::Transport
{
public:
  /**
   *  @name Configuration methods (synchronous)
   */
  //@{
  virtual void getBLOB(TransportAPI::BLOB*& endpoint) const;
  virtual size_t getMaximumBufferSize() const;
  virtual TransportAPI::Status isCompatibleEndpoint(TransportAPI::BLOB* endpoint) const;
  virtual TransportAPI::Status configure(const TransportAPI::NVPList& configuration);
  //@}

  /**
   *  @name Link creation / deletion methods (synchronous)
   */
  //@{
  virtual TransportAPI::Transport::Link* createLink();
  virtual void destroyLink(TransportAPI::Transport::Link* link);
  //@}

  class Link
    : public TransportAPI::Transport::Link
  {
  public:
    Link(Log& log);

    /**
     *  @name Configuration methods (synchronous)
     */
    //@{
    virtual TransportAPI::Status setCallback(TransportAPI::LinkCallback* callback);
    //@}

    /**
     *  @name Connection methods (asynchronous)
     */
    //@{
    virtual TransportAPI::Status establish(TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId);
    virtual TransportAPI::Status shutdown(const TransportAPI::Id& requestId);
    //@}

    /**
     *  @name Data-related methods (asynchronous)
     */
    //@{
    virtual TransportAPI::Status send(const iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);
    //@}

    virtual ~Link() {}

  private:
    TransportAPI::LinkCallback* callback_;
    Log& log_;
  };

  virtual ~DummyTransport() {}

  Log log_;
};

#endif /* DUMMYTRANSPORT_H */
