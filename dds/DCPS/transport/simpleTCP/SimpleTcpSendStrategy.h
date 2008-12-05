// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_SIMPLETCPSENDSTRATEGY_H
#define OPENDDS_DCPS_SIMPLETCPSENDSTRATEGY_H

#include "SimpleTcpConnection_rch.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpDataLink_rch.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"


namespace OpenDDS
{

  namespace DCPS
  {

    class SimpleTcpConfiguration;
    class SimpleTcpSynchResource;


    class SimpleTcpSendStrategy : public TransportSendStrategy
    {
      public:

        SimpleTcpSendStrategy(SimpleTcpDataLink*      link,
                              SimpleTcpConfiguration* config,
                              SimpleTcpConnection*    connection,
                              SimpleTcpSynchResource* synch_resource,
                              TransportReactorTask*   task,
                              CORBA::Long             priority);
        virtual ~SimpleTcpSendStrategy();

        /// This is called by the datalink object to associate with the "new" connection object.
        /// The "old" connection object is unregistered with the reactor and the "new" connection
        /// object is registered for sending. The implementation of this method is borrowed from
        /// the ReceiveStrategy.
        int reset(SimpleTcpConnection* connection);

      protected:

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);
        virtual ACE_HANDLE get_handle ();
        virtual ssize_t send_bytes_i (const iovec iov[], int n);

        /// Delegate to the connection object to re-establish
        /// the connection.
        virtual void relink (bool do_suspend = true);

        virtual void stop_i();

      private:

        SimpleTcpConnection_rch connection_;
        SimpleTcpDataLink_rch   link_;
        TransportReactorTask_rch reactor_task_;
    };

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_SIMPLETCPSENDSTRATEGY_H */
