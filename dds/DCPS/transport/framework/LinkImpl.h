// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_LINKIMPL_H
#define OPENDDS_DCPS_LINKIMPL_H

#include "dds/DCPS/dcps_export.h"
#include "LinkImplCallback.h"
#include "LinkCallback.h"
#include "Transport.h"
#include <ace/Message_Block.h>
#include <ace/Synch.h>
#include <ace/Task.h>
#include <queue>

namespace OpenDDS
{
  namespace DCPS
  {

    /**
     * This class implements the LinkCallback interface and handles
     * downstream calls that should go out on its Link.
     */
    class OpenDDS_Dcps_Export LinkImpl
      : public TransportAPI::LinkCallback
      , public ACE_Task_Base
    {
    public:
      LinkImpl(TransportAPI::Transport& transport, size_t max_transport_buffer_size);
      virtual ~LinkImpl();

      // Set the callback. LinkImpl does not participate in the ownership of
      // the callback object, and therefore setCallback(0) should be called
      // before the callback's destruction.
      void setCallback(LinkImplCallback* cb);

      /**
       * ACE_Task_Base methods
       */
      //@{
      virtual int open(void* args = 0);
      virtual int close(u_long flags = 0);
      virtual int svc();
      //@}

      // Public for testing
      struct IOItem
      {
        IOItem();
        IOItem(
          ACE_Message_Block& mb,
          char* data,
          size_t size,
          const TransportAPI::Id& requestIdIn,
          size_t sequenceNumber,
          bool ending
          );
        IOItem(const IOItem& rhs);
        ~IOItem();

        IOItem& operator=(const IOItem& rhs);

        std::auto_ptr<ACE_Message_Block> mb_;
        char* data_begin_;
        size_t data_size_;
        TransportAPI::Id requestId_;
        size_t sequenceNumber_;
        bool ending_;
        bool deferred_;
      };

      bool performWork(
        ACE_Thread_Mutex& extLock,
        ACE_Condition<ACE_Thread_Mutex>& extCondition,
        bool& extShutdown,
        std::deque<IOItem>& extQueue,
        bool& extConnected,
        bool& extBackpressure,
        bool& extRunning
        );

      TransportAPI::Status connect(TransportAPI::BLOB* endpoint);
      TransportAPI::Status disconnect();

      TransportAPI::Status send(ACE_Message_Block& mb, TransportAPI::Id& requestId);
      TransportAPI::Status recall(const TransportAPI::Id& requestId);

      virtual void connected(const TransportAPI::Id& requestId);
      virtual void disconnected(const TransportAPI::failure_reason& reason);

      virtual void sendSucceeded(const TransportAPI::Id& requestId);
      virtual void sendFailed(const TransportAPI::failure_reason& reason);
      virtual void backPressureChanged(bool applyBackpressure, const TransportAPI::failure_reason& reason);
      virtual void received(const iovec buffers[], size_t iovecSize);

    private:
      class LinkGuard
      {
      public:
        LinkGuard(TransportAPI::Transport& transport);
        ~LinkGuard();
        LinkGuard& operator=(TransportAPI::Transport::Link* link);
        void reset(TransportAPI::Transport::Link* link = 0);
        TransportAPI::Transport::Link* operator->() const;
        TransportAPI::Transport::Link* get() const;

      private:
        LinkGuard(const LinkGuard& rhs);

        void tryFree();

        TransportAPI::Transport& transport_;
        TransportAPI::Transport::Link* link_;
      };

      typedef ACE_Guard<ACE_Thread_Mutex> Guard;

      void deliver(
        const Guard&,
        ACE_Message_Block& mb,
        const TransportAPI::Id& requestId
        );
      bool trySending(IOItem& item);
      TransportAPI::Id getNextRequestId(const Guard&);

      LinkImplCallback* callback_;
      TransportAPI::Transport& transport_;
      LinkGuard link_;
      size_t max_transport_buffer_size_;
      ACE_Thread_Mutex lock_;
      ACE_Condition<ACE_Thread_Mutex> connectedCondition_;
      ACE_Condition<ACE_Thread_Mutex> condition_;
      ACE_thread_t threadId_;
      TransportAPI::Id currentRequestId_;
      bool running_;
      bool shutdown_;
      bool connected_;
      bool backpressure_;
      bool connectionDeferred_;
      TransportAPI::Status deferredConnectionStatus_;
      std::deque<IOItem> queue_;
      IOItem bufferedData_;
      std::pair<TransportAPI::Id, size_t> lastReceived_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "LinkImpl.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_LINKIMPL_H */
