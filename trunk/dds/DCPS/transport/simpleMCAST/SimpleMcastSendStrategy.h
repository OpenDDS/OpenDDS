// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTSENDSTRATEGY_H
#define TAO_DCPS_SIMPLEMCASTSENDSTRATEGY_H

#include  "SimpleMcast_export.h"
#include  "SimpleMcastSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "ace/INET_Addr.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleMcastConfiguration;

    class SimpleMcast_Export SimpleMcastSendStrategy : public TransportSendStrategy
    {
      public:

        SimpleMcastSendStrategy(SimpleMcastConfiguration* config,
                              const ACE_INET_Addr&    remote_address,
                              SimpleMcastSocket*        socket);
        virtual ~SimpleMcastSendStrategy();

      protected:

        virtual void stop_i();

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);


      private:

        /// The remote address
        ACE_INET_Addr addr_;

        /// The socket
        SimpleMcastSocket_rch socket_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEMCASTSENDSTRATEGY_H */
