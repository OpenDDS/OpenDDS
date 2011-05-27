// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPSOCKET_H
#define TAO_DCPS_SIMPLEUDPSOCKET_H

#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include  "ace/INET_Addr.h"
#include  "ace/Synch.h"
#include  "ace/SOCK_Dgram.h"
#include  "ace/Event_Handler.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUdpSocket : public RcObject<ACE_SYNCH_MUTEX>,
                            public ACE_Event_Handler
    {
      public:

        SimpleUdpSocket();
        virtual ~SimpleUdpSocket();

        virtual ACE_HANDLE get_handle() const;

        virtual int handle_input(ACE_HANDLE);
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

        int open_socket(const ACE_INET_Addr& local_address);
        void close_socket();

        int set_receive_strategy(TransportReceiveStrategy* strategy,
                                 TransportReactorTask*     reactor_task);
        void remove_receive_strategy();

        ssize_t send_bytes(const iovec iov[],
                           int   n,
                           const ACE_INET_Addr& remote_address);

        ssize_t receive_bytes(iovec iov[],
                              int   n,
                              ACE_INET_Addr& remote_address);
        

      private:

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        /// Lock used to protect against sending to the socket_ from
        /// multiple DataLinks.
        LockType lock_;

        /// The local address.
        ACE_INET_Addr local_address_;

        /// The socket
        ACE_SOCK_Dgram socket_;

        /// The reactor task
        TransportReactorTask_rch task_;

        /// The single TransportReceiveStrategy object for this socket.
        TransportReceiveStrategy_rch receive_strategy_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "SimpleUdpSocket.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUDPSOCKET_H */
