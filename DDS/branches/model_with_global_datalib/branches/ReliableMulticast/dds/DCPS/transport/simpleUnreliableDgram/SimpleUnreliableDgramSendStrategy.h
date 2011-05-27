// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUNRELIABLEDGRAMSENDSTRATEGY_H
#define TAO_DCPS_SIMPLEUNRELIABLEDGRAMSENDSTRATEGY_H

#include  "SimpleUnreliableDgram_export.h"
#include  "SimpleUnreliableDgramSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "ace/INET_Addr.h"


namespace TAO
{

  namespace DCPS
  {

    class TransportConfiguration;
    class SimpleUnreliableDgramSynchResource;

    class SimpleUnreliableDgram_Export SimpleUnreliableDgramSendStrategy : public TransportSendStrategy
    {
      public:

        SimpleUnreliableDgramSendStrategy(TransportConfiguration* config,
                              const ACE_INET_Addr&    remote_address,
                              SimpleUnreliableDgramSocket*        socket,
                              SimpleUnreliableDgramSynchResource* resource);
        virtual ~SimpleUnreliableDgramSendStrategy();

      protected:

        virtual void stop_i();

        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);
        virtual ACE_HANDLE get_handle ();
        virtual ssize_t send_bytes_i (const iovec iov[], int n);


      private:

        /// The remote address
        ACE_INET_Addr remote_address_;

        /// The socket
        SimpleUnreliableDgramSocket_rch socket_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUNRELIABLEDGRAMSENDSTRATEGY_H */
