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
  DummyTransport();

  /**
   *  @name Configuration methods (synchronous)
   */
  //@{
  virtual void getBLOB(const TransportAPI::BLOB*& endpoint) const;
  virtual size_t getMaximumBufferSize() const;
  virtual TransportAPI::Status isCompatibleEndpoint(const TransportAPI::BLOB* endpoint) const;
  virtual TransportAPI::Status configure(const TransportAPI::NVPList& configuration);
  //@}

  /**
   *  @name Link creation / deletion methods (synchronous)
   */
  //@{
  virtual std::pair<TransportAPI::Status, TransportAPI::Transport::Link*> establishLink(
    const TransportAPI::BLOB* endpoint,
    const TransportAPI::Id& requestId,
    TransportAPI::LinkCallback* callback,
    bool active
    );
  virtual void destroyLink(TransportAPI::Transport::Link* link);
  //@}

  void setFailTimes(unsigned int times) { shouldFailTimes_ = times; }
  void setDeferred(bool defer = true) { defer_ = defer; }

  virtual ~DummyTransport() {}

  Log log_;

protected:
  class Link
    : public TransportAPI::Transport::Link
  {
  public:
    Link(Log& log, unsigned int& shouldFailTimes, bool& defer, TransportAPI::LinkCallback* callback);

    /**
     *  @name Connection methods (asynchronous)
     */
    //@{
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
    unsigned int& shouldFailTimes_;
    bool& defer_;
  };

  unsigned int shouldFailTimes_;
  bool defer_;
};

#endif /* DUMMYTRANSPORT_H */
