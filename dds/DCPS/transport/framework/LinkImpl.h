// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_LINKIMPL_H
#define TAO_DCPS_LINKIMPL_H

#include "dds/DCPS/dcps_export.h"
#include "LinkCallback.h"
#include "Transport.h"
#include "ace/Synch.h"
#include <queue>

namespace TAO
{
  namespace DCPS
  {

    /**
     * This class implements the LinkCallback interface and handles
     * downstream calls that should go out on its Link.
     */
    class TAO_DdsDcps_Export LinkImpl
      : public TransportAPI::LinkCallback
    {
    public:
      LinkImpl(TransportAPI::Transport::Link& link);
      virtual ~LinkImpl();

      TransportAPI::Status connect(TransportAPI::BLOB* endpoint, const TransportAPI::Id& requestId);
      TransportAPI::Status disconnect(const TransportAPI::Id& requestId);

      TransportAPI::Status send(const TransportAPI::iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);

      virtual void connected(const TransportAPI::Id& requestId);
      virtual void disconnected(const TransportAPI::failure_reason& reason);

      virtual void sendSucceeded(const TransportAPI::Id& requestId);
      virtual void sendFailed(const TransportAPI::failure_reason& reason);
      virtual void backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason);
      virtual void received(const TransportAPI::iovec buffers[], size_t iovecSize);

    private:
      typedef ACE_Guard<ACE_Thread_Mutex> Guard;

      void enqueue(const Guard&, const TransportAPI::iovec buffers[], size_t iovecSize, const TransportAPI::Id& requestId);

      struct IOItem
      {
        IOItem(const TransportAPI::iovec buffersIn[], size_t iovecSizeIn, const TransportAPI::Id& requestIdIn);
        ~IOItem();

        const TransportAPI::iovec* buffers;
        size_t iovecSize;
        TransportAPI::Id requestId;
      };

      TransportAPI::Transport::Link& link_;
      ACE_Thread_Mutex lock_;
      bool queueing_;
      std::queue<IOItem> queue_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "LinkImpl.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_LINKIMPL_H */
