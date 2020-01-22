/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPDATALINK_H
#define OPENDDS_TCPDATALINK_H

#include "TcpConnection_rch.h"
#include "TcpTransport.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpSendStrategy;
class TcpReceiveStrategy;

class TcpDataLink : public DataLink {
public:

  TcpDataLink(const ACE_INET_Addr& remote_address,
                    TcpTransport&  transport_impl,
                    Priority           priority,
                    bool               is_loopback,
                    bool               is_active);
  virtual ~TcpDataLink();

  /// Accessor for the remote address.
  const ACE_INET_Addr& remote_address() const;

  /// Called when an established connection object is available
  /// for this TcpDataLink.  Called by the TcpTransport's
  /// connect_datalink() method.
  int connect(const TcpConnection_rch& connection,
              const RcHandle<TcpSendStrategy>& send_strategy,
              const RcHandle<TcpReceiveStrategy>& receive_strategy);

  int reuse_existing_connection(const TcpConnection_rch& connection);
  int reconnect(const TcpConnection_rch& connection);

  TcpConnection_rch get_connection();

  virtual void pre_stop_i();

  /// Set release pending flag.
  void set_release_pending(bool flag);
  /// Get release pending flag.
  bool is_release_pending() const;

  void ack_received(const ReceivedDataSample& sample);
  void request_ack_received(const ReceivedDataSample& sample);
  void drop_pending_request_acks();

  TcpSendStrategy_rch send_strategy();
  TcpReceiveStrategy_rch receive_strategy();

  int make_reservation(const RepoId& remote_subscription_id,
                       const RepoId& local_publication_id,
                       const TransportSendListener_wrch& send_listener,
                       bool reliable);

  int make_reservation(const RepoId& remote_publication_id,
                       const RepoId& local_subscription_id,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

protected:

  /// Called when the DataLink is self-releasing because all of its
  /// reservations have been released, or when the TransportImpl is
  /// handling a shutdown() call.
  virtual void stop_i();

private:
  bool handle_send_request_ack(TransportQueueElement* element);
  void send_graceful_disconnect_message();
  void do_association_actions();
  void send_association_msg(const RepoId& local, const RepoId& remote);

  ACE_INET_Addr           remote_address_;
  WeakRcHandle<TcpConnection> connection_;
  bool graceful_disconnect_sent_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> release_is_pending_;
  typedef OPENDDS_VECTOR(TransportQueueElement*) PendingRequestAcks;
  ACE_SYNCH_MUTEX pending_request_acks_lock_;
  PendingRequestAcks pending_request_acks_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpDataLink.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPDATALINK_H */
