/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEMCASTSOCKET_H
#define OPENDDS_DCPS_SIMPLEMCASTSOCKET_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramSocket.h"
#include "ace/SOCK_Dgram_Mcast.h"

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleMcastSocket : public SimpleUnreliableDgramSocket {
public:

  SimpleMcastSocket();
  virtual ~SimpleMcastSocket();

  virtual ACE_HANDLE get_handle() const;
  virtual ACE_SOCK& socket();

  virtual int open_socket(ACE_INET_Addr& local_address,
                          const ACE_INET_Addr& multicast_group_address,
                          bool receiver);

  virtual void close_socket();

  virtual ssize_t send_bytes(const iovec iov[],
                             int   n,
                             const ACE_INET_Addr& multicast_group_address);

  virtual ssize_t receive_bytes(iovec iov[],
                                int   n,
                                ACE_INET_Addr& multicast_group_address);

private:

  /// The group address.
  ACE_INET_Addr multicast_group_address_;

  /// The socket
  ACE_SOCK_Dgram_Mcast socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleMcastSocket.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLEMCASTSOCKET_H */
