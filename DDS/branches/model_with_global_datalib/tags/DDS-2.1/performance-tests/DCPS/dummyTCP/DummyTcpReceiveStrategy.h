// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DUMMYTCPRECEIVESTRATEGY_H
#define OPENDDS_DCPS_DUMMYTCPRECEIVESTRATEGY_H

#include "DummyTcpConnection_rch.h"
#include "DummyTcpConnection.h"
#include "DummyTcpDataLink_rch.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"


namespace OpenDDS
{

  namespace DCPS
  {

    class DummyTcpReceiveStrategy : public TransportReceiveStrategy
    {
      public:

        DummyTcpReceiveStrategy(DummyTcpDataLink*    link,
                                 DummyTcpConnection*  connection,
                                 TransportReactorTask* task);
        virtual ~DummyTcpReceiveStrategy();

        int reset(DummyTcpConnection* connection);

        ACE_Reactor* get_reactor();

        bool gracefully_disconnected ();

      protected:

        virtual ssize_t receive_bytes(iovec iov[],
                                      int   n,
                                      ACE_INET_Addr& remote_address);

        virtual void deliver_sample(ReceivedDataSample& sample,
                                    const ACE_INET_Addr& remote_address);

        virtual int start_i();
        virtual void stop_i();

        // Delegate to the connection object to re-establishment
        // the connection.
        virtual void relink (bool do_suspend = true);

      private:

        DummyTcpDataLink_rch    link_;
        DummyTcpConnection_rch  connection_;
        TransportReactorTask_rch reactor_task_;
    };

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "DummyTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DUMMYTCPRECEIVESTRATEGY_H */
