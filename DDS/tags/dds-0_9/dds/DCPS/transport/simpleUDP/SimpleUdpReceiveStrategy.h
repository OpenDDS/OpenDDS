// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPRECEIVESTRATEGY_H
#define TAO_DCPS_SIMPLEUDPRECEIVESTRATEGY_H

#include  "SimpleUdp_export.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "SimpleUdpTransport_rch.h"
#include  "SimpleUdpSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUdp_Export SimpleUdpReceiveStrategy 
      : public TransportReceiveStrategy
    {
      public:

        SimpleUdpReceiveStrategy(SimpleUdpTransport*   transport,
                                 SimpleUdpSocket*      socket,
                                 TransportReactorTask* task);
        virtual ~SimpleUdpReceiveStrategy();


      protected:

        virtual ssize_t receive_bytes(iovec          iov[],
                                      int            n,
                                      ACE_INET_Addr& remote_address);

        virtual void deliver_sample(ReceivedDataSample&  sample,
                                    const ACE_INET_Addr& remote_address);

        virtual int start_i();
        virtual void stop_i();


      private:

        SimpleUdpTransport_rch   transport_;
        SimpleUdpSocket_rch      socket_;
        TransportReactorTask_rch task_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUDPRECEIVESTRATEGY_H */
