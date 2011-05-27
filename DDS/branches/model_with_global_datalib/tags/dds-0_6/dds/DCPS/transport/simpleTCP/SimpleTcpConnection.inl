// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpAcceptor.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "ace/SOCK_Connector.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpConnection::SimpleTcpConnection()
{
  DBG_ENTRY("SimpleTcpConnection","SimpleTcpConnection");
}


ACE_INLINE int
TAO::DCPS::SimpleTcpConnection::active_establishment
                                    (const ACE_INET_Addr& remote_address,
                                     const ACE_INET_Addr& local_address)
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

  set_buffer_size();

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


ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::disconnect()
{
  DBG_ENTRY("SimpleTcpConnection","disconnect");
  this->peer().close();
}




ACE_INLINE void
TAO::DCPS::SimpleTcpConnection::remove_receive_strategy()
{
  DBG_ENTRY("SimpleTcpConnection","remove_receive_strategy");

  this->receive_strategy_ = 0;
}

