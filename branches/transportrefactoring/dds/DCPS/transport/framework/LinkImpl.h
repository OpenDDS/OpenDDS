// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_LINKIMPL_H
#define TAO_DCPS_LINKIMPL_H

#include "dds/DCPS/dcps_export.h"
#include "LinkCallback.h"
#include "Transport.h"
#include <ace/Message_Block.h>
#include <ace/Synch.h>
#include <ace/Task.h>
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
      , public ACE_Task_Base
    {
    public:
      LinkImpl(TransportAPI::Transport::Link& link, size_t max_transport_buffer_size);
      virtual ~LinkImpl();

      /**
       * ACE_Task_Base methods
       */
      //@{
      virtual int open(void* args = 0);
      virtual int close(u_long flags = 0);
      virtual int svc();
      //@}

      TransportAPI::Status connect(TransportAPI::BLOB* endpoint);
      TransportAPI::Status disconnect();

      TransportAPI::Status send(ACE_Message_Block& mb);

      virtual void connected(const TransportAPI::Id& requestId);
      virtual void disconnected(const TransportAPI::failure_reason& reason);

      virtual void sendSucceeded(const TransportAPI::Id& requestId);
      virtual void sendFailed(const TransportAPI::failure_reason& reason);
      virtual void backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason);
      virtual void received(const iovec buffers[], size_t iovecSize);

    private:
      typedef ACE_Guard<ACE_Thread_Mutex> Guard;

      bool enqueue(const Guard&, ACE_Message_Block& mb, const TransportAPI::Id& requestId);
      TransportAPI::Id getNextRequestId(const Guard&);

      struct IOItem
      {
        IOItem();
        IOItem(
          ACE_Message_Block& mb,
          char* data,
          size_t size,
          const TransportAPI::Id& requestIdIn
          );
        IOItem(const IOItem& rhs);
        ~IOItem();

        IOItem& operator=(const IOItem& rhs);

        std::auto_ptr<ACE_Message_Block> mb_;
        char* data_begin_;
        size_t data_size_;
        TransportAPI::Id requestId_;
      };

      TransportAPI::Transport::Link& link_;
      size_t max_transport_buffer_size_;
      ACE_Thread_Mutex lock_;
      ACE_Condition<ACE_Thread_Mutex> condition_;
      ACE_thread_t threadId_;
      TransportAPI::Id currentRequestId_;
      bool running_;
      bool shutdown_;
      bool connected_;
      bool backpressure_;
      std::queue<IOItem> queue_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "LinkImpl.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_LINKIMPL_H */
