// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPSOCKET_H
#define TAO_DCPS_SIMPLEUDPSOCKET_H

#include  "SimpleUnreliableDgram_export.h"
#include  "SimpleUnreliableDgramSocket.h"
#include  "ace/INET_Addr.h"
#include  "ace/SOCK_Dgram.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUdpSocket : public SimpleUnreliableDgramSocket
    {
      public:

        SimpleUdpSocket();
        virtual ~SimpleUdpSocket();

        virtual ACE_HANDLE get_handle() const;

         virtual int open_socket (ACE_INET_Addr& local_address,
                 const ACE_INET_Addr& multicast_group_address,
                 bool receiver);

        virtual void close_socket();

        virtual ssize_t send_bytes(const iovec iov[],
                           int   n,
                           const ACE_INET_Addr& remote_address);

        virtual ssize_t receive_bytes(iovec iov[],
                              int   n,
                              ACE_INET_Addr& remote_address);

      private:

        /// The socket
        ACE_SOCK_Dgram socket_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "SimpleUdpSocket.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_SIMPLEUDPSOCKET_H */
