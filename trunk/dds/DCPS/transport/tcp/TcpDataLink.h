/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPDATALINK_H
#define OPENDDS_TCPDATALINK_H

#include "TcpConnection_rch.h"
#include "TcpTransport_rch.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "ace/INET_Addr.h"

namespace OpenDDS {
namespace DCPS {

class TransportSendStrategy;
class TransportStrategy;

class TcpDataLink : public DataLink {
public:

  TcpDataLink(const ACE_INET_Addr& remote_address,
                    TcpTransport*  transport_impl,
                    CORBA::Long          priority,
                    bool                 is_loopback,
                    bool                 is_active);
  virtual ~TcpDataLink();

  /// Accessor for the remote address.
  const ACE_INET_Addr& remote_address() const;

  /// Called when an established connection object is available
  /// for this TcpDataLink.  Called by the TcpTransport's
  /// connect_datalink() method.
  int connect(const TcpConnection_rch& connection,
              const TransportSendStrategy_rch& send_strategy,
              const TransportStrategy_rch& receive_strategy);

  int reconnect(TcpConnection* connection);

  TcpConnection_rch get_connection();
  TcpTransport_rch get_transport_impl();

  virtual void pre_stop_i();

  /// Set release pending flag.
  void set_release_pending(bool flag);
  /// Get release pending flag.
  bool is_release_pending() const;

protected:

  /// Called when the DataLink is self-releasing because all of its
  /// reservations have been released, or when the TransportImpl is
  /// handling a shutdown() call.
  virtual void stop_i();

private:

  void send_graceful_disconnect_message();

  ACE_INET_Addr           remote_address_;
  TcpConnection_rch connection_;
  TcpTransport_rch  transport_;
  bool graceful_disconnect_sent_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> release_is_pending_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TcpDataLink.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPDATALINK_H */
