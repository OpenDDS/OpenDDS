// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTRECEIVESTRATEGY_H
#define TAO_DCPS_SIMPLEMCASTRECEIVESTRATEGY_H

#include  "SimpleMcast_export.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "SimpleMcastTransport_rch.h"
#include  "SimpleMcastSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleMcast_Export SimpleMcastReceiveStrategy 
      : public TransportReceiveStrategy
    {
      public:

        SimpleMcastReceiveStrategy(SimpleMcastTransport*   transport,
                                 SimpleMcastSocket*      socket,
                                 TransportReactorTask* task);
        virtual ~SimpleMcastReceiveStrategy();


      protected:

        virtual ssize_t receive_bytes(iovec          iov[],
                                      int            n,
                                      ACE_INET_Addr& remote_address);

        virtual void deliver_sample(ReceivedDataSample&  sample,
                                    const ACE_INET_Addr& remote_address);

        virtual int start_i();
        virtual void stop_i();


      private:

        SimpleMcastTransport_rch   transport_;
        SimpleMcastSocket_rch      socket_;
        TransportReactorTask_rch task_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEMCASTRECEIVESTRATEGY_H */
