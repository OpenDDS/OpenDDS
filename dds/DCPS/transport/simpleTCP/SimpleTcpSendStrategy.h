// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPSENDSTRATEGY_H
#define TAO_DCPS_SIMPLETCPSENDSTRATEGY_H

#include  "SimpleTcpConnection_rch.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpDataLink_rch.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"


namespace TAO
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
                              SimpleTcpSynchResource* synch_resource);
        virtual ~SimpleTcpSendStrategy();


      protected:

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);

        // Delegate to the connection object to re-establish
        // the connection.
        virtual void relink (bool do_suspend = true);

        virtual void stop_i();

      private:

        SimpleTcpConnection_rch connection_;
        SimpleTcpDataLink_rch   link_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLETCPSENDSTRATEGY_H */
