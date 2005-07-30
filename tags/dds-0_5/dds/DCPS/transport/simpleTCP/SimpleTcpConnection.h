// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPCONNECTION_H
#define TAO_DCPS_SIMPLETCPCONNECTION_H

#include  "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "ace/SOCK_Stream.h"
#include  "ace/Svc_Handler.h"
#include  "ace/INET_Addr.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleTcpConnection
                    : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>,
                      public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        SimpleTcpConnection();
        virtual ~SimpleTcpConnection();

        /// Attempt an active connection establishment to the remote address.
        /// The local address is sent to the remote (passive) side to
        /// identify ourselves to the remote side.
        int active_establishment(const ACE_INET_Addr& remote_address,
                                 const ACE_INET_Addr& local_address);

        /// This will be called by the DataLink (that "owns" us) when
        /// the SimpleTcpTransport has been told to shutdown(), or when
        /// the DataLink finds itself no longer needed, and is
        /// "self-releasing".
        void disconnect();

        // Note that the acceptor that calls the open() method will pass
        // itself in as a void*.
        virtual int open(void* arg);

        void set_receive_strategy(TransportReceiveStrategy* receive_strategy);
        void remove_receive_strategy();

        /// We pass this "event" along to the receive_strategy.
        virtual int handle_input(ACE_HANDLE);

        virtual int close(u_long);
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

        void set_buffer_size ();

      private:

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        TransportReceiveStrategy_rch receive_strategy_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "SimpleTcpConnection.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLETCPCONNECTION_H */
