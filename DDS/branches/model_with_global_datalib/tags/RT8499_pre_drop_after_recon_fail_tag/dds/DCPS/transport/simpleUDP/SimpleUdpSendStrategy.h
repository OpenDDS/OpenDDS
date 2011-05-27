// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPSENDSTRATEGY_H
#define TAO_DCPS_SIMPLEUDPSENDSTRATEGY_H

#include  "SimpleUdp_export.h"
#include  "SimpleUdpSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "ace/INET_Addr.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUdpConfiguration;

    class SimpleUdp_Export SimpleUdpSendStrategy : public TransportSendStrategy
    {
      public:

        SimpleUdpSendStrategy(SimpleUdpConfiguration* config,
                              const ACE_INET_Addr&    remote_address,
                              SimpleUdpSocket*        socket);
        virtual ~SimpleUdpSendStrategy();


      protected:

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);


      private:

        /// The remote address
        ACE_INET_Addr addr_;

        /// The socket
        SimpleUdpSocket_rch socket_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUDPSENDSTRATEGY_H */
