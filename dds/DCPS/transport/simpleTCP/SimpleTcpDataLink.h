/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPDATALINK_H
#define OPENDDS_DCPS_SIMPLETCPDATALINK_H

#include "SimpleTcpConnection_rch.h"
#include "SimpleTcpTransport_rch.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "ace/INET_Addr.h"

namespace OpenDDS {
namespace DCPS {

class TransportSendStrategy;
class TransportReceiveStrategy;

class SimpleTcpDataLink : public DataLink {
public:

  SimpleTcpDataLink(const ACE_INET_Addr& remote_address,
                    SimpleTcpTransport*  transport_impl,
                    CORBA::Long          priority,
                    bool                 is_loopback,
                    bool                 is_active);
  virtual ~SimpleTcpDataLink();

  /// Accessor for the remote address.
  const ACE_INET_Addr& remote_address() const;

  /// Called when an established connection object is available
  /// for this SimpleTcpDataLink.  Called by the SimpleTcpTransport's
  /// connect_datalink() method.
  int connect(SimpleTcpConnection*      connection,
              TransportSendStrategy*    send_strategy,
              TransportReceiveStrategy* receive_strategy);

  int reconnect(SimpleTcpConnection* connection);

  SimpleTcpConnection_rch get_connection();
  SimpleTcpTransport_rch get_transport_impl();

  virtual void pre_stop_i();

  /// Called on subscriber side to send the fully association
  /// message to the publisher.
  virtual void fully_associated();

  /// Set release pending flag.
  void set_release_pending (bool flag);
  /// Get release pending flag.
  bool is_release_pending () const;

protected:

  /// Called when the DataLink is self-releasing because all of its
  /// reservations have been released, or when the TransportImpl is
  /// handling a shutdown() call.
  virtual void stop_i();

private:

  void send_graceful_disconnect_message();

  ACE_INET_Addr           remote_address_;
  SimpleTcpConnection_rch connection_;
  SimpleTcpTransport_rch  transport_;
  bool graceful_disconnect_sent_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> release_is_pending_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleTcpDataLink.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLETCPDATALINK_H */
