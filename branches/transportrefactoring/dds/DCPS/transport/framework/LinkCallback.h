#ifndef TRANSPORTAPI_LINKCALLBACK_H
#define TRANSPORTAPI_LINKCALLBACK_H

#include "TransportTypes.h"

namespace TransportAPI
{
  /// LinkCallback
  class LinkCallback
  {
  public:
    /**
     *  @name Connection callbacks
     */
    //@{
    virtual void connected(const Id& requestId) = 0;
    virtual void disconnected(const failure_reason& reason) = 0;
    //@}

    /**
     *  @name Data-related callbacks
     */
    //@{
    virtual void sendSucceeded(const Id& requestId) = 0;
    virtual void sendFailed(const failure_reason& reason) = 0;
    virtual void backPressureChanged(bool applyBackpressure, const failure_reason& reason) = 0;
    virtual void received(const iovec buffers[], size_t iovecSize) = 0;
    //@}

  protected:
    virtual ~LinkCallback() {}
  };
}

#endif /* TRANSPORTAPI_LINKCALLBACK_H */
