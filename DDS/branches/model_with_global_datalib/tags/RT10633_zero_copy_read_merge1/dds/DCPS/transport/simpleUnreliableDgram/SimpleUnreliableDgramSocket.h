// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUNRELIABLEDGRAMSOCKET_H
#define TAO_DCPS_SIMPLEUNRELIABLEDGRAMSOCKET_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "ace/INET_Addr.h"
#include "ace/Synch.h"
#include "ace/SOCK_Dgram.h"
#include "ace/Event_Handler.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUnreliableDgramSocket : public RcObject<ACE_SYNCH_MUTEX>, 
                            public ACE_Event_Handler
    {
      public:

        SimpleUnreliableDgramSocket();
        virtual ~SimpleUnreliableDgramSocket();

        virtual ACE_HANDLE get_handle() const = 0;

        virtual int handle_input(ACE_HANDLE);
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

        int set_receive_strategy(TransportReceiveStrategy* strategy,
                                 TransportReactorTask*     reactor_task);
        void remove_receive_strategy();

        virtual int open_socket (ACE_INET_Addr& local_address,
                 const ACE_INET_Addr& multicast_group_address = ACE_INET_Addr(),
                 bool receiver = false) = 0;

        virtual void close_socket() = 0;

        virtual ssize_t send_bytes(const iovec iov[],
                           int   n,
                           const ACE_INET_Addr& remote_address) = 0;

        virtual ssize_t receive_bytes(iovec iov[],
                              int   n,
                              ACE_INET_Addr& remote_address) = 0;

      protected:
        
        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        /// Lock used to protect against sending to the socket_ from
        /// multiple DataLinks.
        LockType lock_;

        /// The reactor task
        TransportReactorTask_rch task_;

        /// The single TransportReceiveStrategy object for this socket.
        TransportReceiveStrategy_rch receive_strategy_;

        /// The local address.
        ACE_INET_Addr local_address_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramSocket.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUNRELIABLEDGRAMSOCKET_H */
