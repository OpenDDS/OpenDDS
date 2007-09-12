#ifndef TRANSPORTAPI_TRANSPORT_H
#define TRANSPORTAPI_TRANSPORT_H

#include "TransportTypes.h"

namespace TransportAPI
{

  class LinkCallback;
  class TransportCallback;
  class Transport;

  /// TransportFactory
  class TransportFactory
  {
  public:
    virtual Transport* createTransport(
      TransportCallback* callback
      ) = 0;
    virtual void destroyTransport(
      Transport* transport
      ) = 0;

  protected:
    virtual ~TransportFactory() {}
  };

  /// Transport
  class Transport
  {
  public:
    /// A Link is used to refer to an active connection.
    class Link;

    /**
     *  @name Configuration methods (synchronous)
     */
    //@{
    virtual void getBLOB(const BLOB*& endpoint) const = 0;
    virtual size_t getMaximumBufferSize() const = 0;
    virtual Status isCompatibleEndpoint(const BLOB* endpoint) const = 0;
    virtual Status configure(const NVPList& configuration) = 0;
    //@}

    /**
     *  @name Link creation / deletion methods (synchronous)
     */
    //@{
    virtual std::pair<Status, Link*> establishLink(
      const BLOB* endpoint,
      const Id& requestId,
      LinkCallback* callback,
      bool active
      ) = 0;
    virtual void destroyLink(Link* link) = 0;
    //@}

    class Link
    {
    public:
      /**
       *  @name Connection methods (asynchronous)
       */
      //@{
      virtual Status shutdown(const Id& requestId) = 0;
      //@}

      /**
       *  @name Data-related methods (asynchronous)
       */
      //@{
      virtual Status send(const iovec buffers[], size_t iovecSize, const Id& requestId) = 0;
      //@}

    protected:
      virtual ~Link() {}
    };

  protected:
    virtual ~Transport() {}
  };

  class TransportCallback
  {
  public:
    virtual void linkEstablished(
      const BLOB* endpoint,
      const Id& requestId,
      TransportAPI::Transport::Link* link
      ) = 0;

  protected:
    virtual ~TransportCallback() {}
  };
}

#endif /* TRANSPORTAPI_TRANSPORT_H */
