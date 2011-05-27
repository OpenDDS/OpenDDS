// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPSENDSTRATEGY_H
#define TAO_DCPS_SIMPLETCPSENDSTRATEGY_H

#include  "SimpleTcpConnection_rch.h"
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

        SimpleTcpSendStrategy(SimpleTcpConfiguration* config,
                              SimpleTcpConnection*    connection,
                              SimpleTcpSynchResource* synch_resource);
        virtual ~SimpleTcpSendStrategy();


      protected:

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);


      private:

        SimpleTcpConnection_rch connection_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLETCPSENDSTRATEGY_H */
