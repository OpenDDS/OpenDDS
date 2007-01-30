// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTSOCKET_H
#define TAO_DCPS_SIMPLEMCASTSOCKET_H

#include  "SimpleMcast_export.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include  "ace/INET_Addr.h"
#include  "ace/Synch.h"
#include  "ace/SOCK_Dgram_Mcast.h"
#include  "ace/Event_Handler.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleMcast_Export SimpleMcastSocket : public RcObject<ACE_SYNCH_MUTEX>, 
                            public ACE_Event_Handler
    {
      public:

        SimpleMcastSocket();
        virtual ~SimpleMcastSocket();

        virtual ACE_HANDLE get_handle() const;

        virtual int handle_input(ACE_HANDLE);
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

        int open(const ACE_INET_Addr& local_address,
                 const ACE_INET_Addr& multicast_group_address,
                 bool receiver);
        void close();

        int set_receive_strategy(TransportReceiveStrategy* strategy,
                                 TransportReactorTask*     reactor_task);
        void remove_receive_strategy();

        ssize_t send_bytes(const iovec iov[],
                           int   n,
                           const ACE_INET_Addr& multicast_group_address);

        ssize_t receive_bytes(iovec iov[],
                              int   n,
                              ACE_INET_Addr& multicast_group_address);

      private:

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        /// Lock used to protect against sending to the socket_ from
        /// multiple DataLinks.
        LockType lock_;

        /// The local address.
        ACE_INET_Addr local_address_;

        /// The group address.
        ACE_INET_Addr multicast_group_address_;

        /// The socket
        ACE_SOCK_Dgram_Mcast socket_;

        /// The reactor task
        TransportReactorTask_rch task_;

        /// The single TransportReceiveStrategy object for this socket.
        TransportReceiveStrategy_rch receive_strategy_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "SimpleMcastSocket.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEMCASTSOCKET_H */
