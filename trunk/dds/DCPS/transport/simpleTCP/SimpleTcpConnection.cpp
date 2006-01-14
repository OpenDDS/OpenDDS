// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConfiguration.h"
#include  "ace/os_include/netinet/os_tcp.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpConnection.inl"
#endif /* __ACE_INLINE__ */

#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

TAO::DCPS::SimpleTcpConnection::~SimpleTcpConnection()
{
  DBG_ENTRY("SimpleTcpConnection","~SimpleTcpConnection");
}


// This can not be inlined due to circular dependencies disallowing
// visibility into the receive strategy to call add_ref().  Oh well.
void
TAO::DCPS::SimpleTcpConnection::set_receive_strategy
                                 (TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY("SimpleTcpConnection","set_receive_strategy");

  // Make a "copy" for ourselves
  receive_strategy->_add_ref();
  this->receive_strategy_ = receive_strategy;
}

int
TAO::DCPS::SimpleTcpConnection::open(void* arg)
{
  DBG_ENTRY("SimpleTcpConnection","open");

  // The passed-in arg is really the acceptor object that created this
  // SimpleTcpConnection object, and is also the caller of this open()
  // method.  We need to cast the arg to the SimpleTcpAcceptor* type.
  SimpleTcpAcceptor* acceptor = ACE_static_cast(SimpleTcpAcceptor*,arg);

  if (acceptor == 0)
    {
      // The cast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to cast void* arg to "
                        "SimpleTcpAcceptor* type.\n"),
                       -1);
    }

  // Now we need to ask the SimpleTcpAcceptor object to provide us with
  // a pointer to the SimpleTcpTransport object that "owns" the acceptor.
  SimpleTcpTransport_rch transport = acceptor->transport();

  if (transport.is_nil())
    {
      // The acceptor gave us a nil transport (smart) pointer.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Acceptor's transport is nil.\n"),
                       -1);
    }

  SimpleTcpConfiguration* tcp_config = acceptor->get_configuration();
  set_sock_options(tcp_config);

  // We expect that the active side of the connection (the remote side
  // in this case) will supply its listening ACE_INET_Addr as the first
  // message it sends to the socket.  This is a one-way connection
  // establishment protocol message.
  NetworkAddress network_order_address;

  if (this->peer().recv_n((char*)(&network_order_address),
                          sizeof(network_order_address)) == -1)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to receive the remote_address "
                        "from the remote (active) side of the connection."
                        " %p\n", "recv_n"),
                       -1);
    }

  ACE_INET_Addr remote_address;
  network_order_address.to_addr(remote_address);

//MJM: vvv CONNECTION ESTABLISHMENT CHANGES vvv

//MJM: Add code to send a response to the other side that the
//MJM: connection is ready to receive at this point.  It may be
//MJM: necessary to do this higher in the layers to make sure that we
//MJM: really are ready to receive.

//MJM: This is the only really tricky bit, since I have not really
//MJM: investigated enough to know where the connection is considered
//MJM: complete on this end.  I think that it will be when the datalink
//MJM: is placed in all the containers.

//MJM: This is also where this end needs to call back the
//MJM: TransportInterface method that will eventually signal() the
//MJM: wait()ing add_associations() call.  It may not be necessary on
//MJM: this (passive) to actually perform the wait() and signal()
//MJM: operations.  There is enough information in the
//MJM: add_associations() call to differentiate the cases.

//MJM: ^^^ CONNECTION ESTABLISHMENT CHANGES ^^^

  // Now it is time to announce (and give) ourselves to the
  // SimpleTcpTransport object.
  transport->passive_connection(remote_address,this);

  return 0;
}


int
TAO::DCPS::SimpleTcpConnection::handle_input(ACE_HANDLE)
{
  DBG_ENTRY("SimpleTcpConnection","handle_input");

  TransportReceiveStrategy_rch rs = this->receive_strategy_;

  if (rs.is_nil())
    {
      return 0;
    }

  return rs->handle_input();
}


int
TAO::DCPS::SimpleTcpConnection::close(u_long)
{
  DBG_ENTRY("SimpleTcpConnection","close");

  // TBD SOON - Find out exactly when close() is called.
  //            I have no clue when and who might call this.

  this->peer().close();
  return 0;
}


int
TAO::DCPS::SimpleTcpConnection::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY("SimpleTcpConnection","handle_close");

  // TBD SOON - Find out exactly when handle_close() is called.
  //            My guess is that it happens if the reactor is closed
  //            while we are still registered with the reactor.  Right?

  this->peer().close();
  return 0;
}

void
TAO::DCPS::SimpleTcpConnection::set_sock_options (SimpleTcpConfiguration* tcp_config)
{
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  //ACE_SOCK_Stream sock = ACE_static_cast(ACE_SOCK_Stream, this->peer() );
#if !defined (ACE_LACKS_SOCKET_BUFSIZ)

  // A little screwy double negative logic: disabling nagle involves
  // enabling TCP_NODELAY
  int opt = (tcp_config->enable_nagle_algorithm_ == false);
  if (this->peer().set_option (IPPROTO_TCP, TCP_NODELAY, &opt, sizeof (opt)) == -1) {
    ACE_ERROR((LM_ERROR, "Failed to set TCP_NODELAY\n"));
  }

 if (this->peer().set_option (SOL_SOCKET,
                          SO_SNDBUF,
                          (void *) &snd_size,
                          sizeof (snd_size)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
      "(%P|%t) SimpleTcpConnection failed to set the send buffer size to %d errno %m\n",
      snd_size));
    return;
  }

  if (this->peer().set_option (SOL_SOCKET,
                          SO_RCVBUF,
                          (void *) &rcv_size,
                          sizeof (int)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
      "(%P|%t) SimpleTcpConnection failed to set the receive buffer size to %d errno %m \n",
      rcv_size));
    return;
  }
#else
   ACE_UNUSED_ARG (snd_size);
   ACE_UNUSED_ARG (rcv_size);
#endif /* !ACE_LACKS_SOCKET_BUFSIZ */

#else 
   ACE_UNUSED_ARG (snd_size);
   ACE_UNUSED_ARG (rcv_size);
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */
}


int
TAO::DCPS::SimpleTcpConnection::active_establishment
                                    (const ACE_INET_Addr& remote_address,
                                     const ACE_INET_Addr& local_address,
                                     SimpleTcpConfiguration* tcp_config)
{
  DBG_ENTRY("SimpleTcpConnection","active_establishment");

  // Now use a connector object to establish the connection.
  ACE_SOCK_Connector connector;

  if (connector.connect(this->peer(), remote_address) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to connect.\n"),
                       -1);
    }

  set_sock_options(tcp_config);

  // In order to complete the connection establishment from the active
  // side, we need to tell the remote side about our local_address.
  // It will use that as an "identifier" of sorts.  To the other
  // (passive) side, our local_address that we send here will be known
  // as the remote_address.
  NetworkAddress network_order_address(local_address);

  if (this->peer().send_n((char*)(&network_order_address),
                          sizeof(network_order_address)) == -1)
    {
      // TBD later - Anything we are supposed to do to close the connection.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to send our local_address_ to "
                        "the passive side to complete the active connection "
                        "establishment.\n"),
                       -1);
    }

//MJM: vvv CONNECTION ESTABLISHMENT CHANGES vvv

//MJM: Add code to receive a response from the other side that the
//MJM: connection is ready to receive at this point.  Block until it is
//MJM: received.  Then call the method in the TransportInterface that
//MJM: the add_associations() call is wait()ing on.

//MJM: ^^^ CONNECTION ESTABLISHMENT CHANGES ^^^

  return 0;
}


