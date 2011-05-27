// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPRECEIVESTRATEGY_H
#define TAO_DCPS_SIMPLETCPRECEIVESTRATEGY_H

#include  "SimpleTcpConnection_rch.h"
#include  "dds/DCPS/transport/framework/DataLink_rch.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleTcpReceiveStrategy : public TransportReceiveStrategy
    {
      public:

        SimpleTcpReceiveStrategy(DataLink*             link,
                                 SimpleTcpConnection*  connection,
                                 TransportReactorTask* task);
        virtual ~SimpleTcpReceiveStrategy();


      protected:

        virtual ssize_t receive_bytes(iovec iov[],
                                      int   n,
                                      ACE_INET_Addr& remote_address);

        virtual void deliver_sample(ReceivedDataSample& sample,
                                    const ACE_INET_Addr& remote_address);

        virtual int start_i();
        virtual void stop_i();

      private:

        DataLink_rch             link_;
        SimpleTcpConnection_rch  connection_;
        TransportReactorTask_rch reactor_task_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLETCPRECEIVESTRATEGY_H */
