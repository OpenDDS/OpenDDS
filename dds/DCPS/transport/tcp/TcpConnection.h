/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPCONNECTION_H
#define OPENDDS_TCPCONNECTION_H

#include "TcpInst_rch.h"
#ifdef __BORLANDC__
#  include "TcpDataLink.h"
#endif
#include "TcpDataLink_rch.h"
#include "TcpConnection_rch.h"
#include "TcpSendStrategy_rch.h"
#include "TcpReceiveStrategy_rch.h"
#include "TcpReconnectTask.h"
#include "TcpTransport_rch.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/INET_Addr.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpConnection
  : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
  , public RcObject<ACE_SYNCH_MUTEX> {
public:

  /// States are used during reconnecting.
  enum ReconnectState {
    INIT_STATE,
    LOST_STATE,
    RECONNECTED_STATE,
    PASSIVE_WAITING_STATE,
    PASSIVE_TIMEOUT_CALLED_STATE
  };

  /// Passive side constructor (acceptor)
  TcpConnection();

  /// Active side constructor (connector)
  TcpConnection(const ACE_INET_Addr& remote_address,
                Priority priority,
                const TcpInst_rch& config);

  virtual ~TcpConnection();

  std::size_t& id();

  /// Protocol setup (handshake) on the active side.
  /// The local address is sent to the remote (passive) side to
  /// identify ourselves to the remote side.
  int active_open();

  /// This will be called by the DataLink (that "owns" us) when
  /// the TcpTransport has been told to shutdown(), or when
  /// the DataLink finds itself no longer needed, and is
  /// "self-releasing".
  void disconnect();

  // Note that the acceptor or connector that calls the open() method will pass
  // itself in as a void*.
  virtual int open(void* arg);

  void set_receive_strategy(const TcpReceiveStrategy_rch& receive_strategy);

  void remove_receive_strategy();

  /// Give a "copy" of the TcpSendStrategy object to this
  /// connection object.
  void set_send_strategy(const TcpSendStrategy_rch& send_strategy);

  void remove_send_strategy();

  /// We pass this "event" along to the receive_strategy.
  virtual int handle_input(ACE_HANDLE);

  /// Handle back pressure when sending.
  virtual int handle_output(ACE_HANDLE);

  virtual int close(u_long);
  virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

  void set_sock_options(TcpInst* tcp_config);

  int reconnect(bool on_new_association = false);

  /// Return true if the object represents the connector side, otherwise
  /// it's the acceptor side. The acceptor/connector role is not changed
  /// when re-establishing the connection.
  bool is_connector() const;

  /// Return true if connection is connected.
  bool is_connected() const;

  void transfer(TcpConnection* connection);

  int handle_timeout(const ACE_Time_Value &tv, const void *arg);

  /// Cache the reference to the datalink object for lost connection
  /// callbacks.
  void set_datalink(const TcpDataLink_rch& link);

  void notify_lost_on_backpressure_timeout();

  ACE_INET_Addr get_remote_address();

  /// Reconnect initiated by send strategy
  void relink_from_send(bool do_suspend);

  /// Reconnect initiated by receive strategy
  void relink_from_recv(bool do_suspend);

  /// Called by the reconnect task to inform us that the
  /// link & any associated data can be torn down.
  /// This call is done with no DCPS/transport locks held.
  bool tear_link();

  void shutdown();

  /// Access TRANSPORT_PRIORITY.value policy value if set.
  Priority& transport_priority();
  Priority  transport_priority() const;

  virtual ACE_Event_Handler::Reference_Count add_reference();
  virtual ACE_Event_Handler::Reference_Count remove_reference();

  OPENDDS_POOL_ALLOCATION_FWD

private:

  /// Attempt an active connection establishment to the remote address.
  /// The local address is sent to the remote (passive) side to
  /// identify ourselves to the remote side.
  /// Note this method is not thread protected. The caller need acquire
  /// the reconnect_lock_ before calling this function.
  int active_establishment(bool initiate_connect = true);

  int active_reconnect_i();
  int passive_reconnect_i();
  int active_reconnect_on_new_association();

  /// During the connection setup phase, the passive side sets passive_setup_,
  /// redirecting handle_input() events here (there is no recv strategy yet).
  int handle_setup_input(ACE_HANDLE h);

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  /// Lock to avoid the reconnect() called multiple times when
  /// both send() and recv() fail.
  LockType  reconnect_lock_;

  /// Flag indicates if connected or disconneted. It's set to true
  /// when actively connecting or passively acepting succeeds and set
  /// to false whenever the peer stream is closed.
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, bool>  connected_;

  /// Flag indicate this connection object is the connector or acceptor.
  bool is_connector_;

  /// Reference to the receiving strategy.
  TcpReceiveStrategy_rch receive_strategy_;

  /// Reference to the send strategy.
  TcpSendStrategy_rch send_strategy_;

  /// Remote address.
  ACE_INET_Addr remote_address_;

  /// Local address.
  ACE_INET_Addr local_address_;

  /// The configuration used by this connection.
  TcpInst_rch tcp_config_;

  /// Datalink object which is needed for connection lost callback.
  TcpDataLink_rch link_;

  /// The id of the scheduled timer. The timer is scheduled to check if the connection
  /// is re-established during the passive_reconnect_duration_. This id controls
  /// that the timer is just scheduled once when there are multiple threads detect
  /// the lost connection.
  int passive_reconnect_timer_id_;

  /// The task to do the reconnecting.
  /// @todo We might need reuse the PerConnectionSynch thread
  /// to do the reconnecting or create the reconnect task when
  /// we need reconnect.
  TcpReconnectTask reconnect_task_;

  /// The state indicates each step of the reconnecting.
  ReconnectState reconnect_state_;

  /// Last time the connection is re-established.
  ACE_Time_Value last_reconnect_attempted_;

  /// TRANSPORT_PRIORITY.value policy value.
  Priority transport_priority_;

  /// shutdown flag
  bool shutdown_;

  bool passive_setup_;
  ACE_Message_Block passive_setup_buffer_;
  TcpTransport_rch transport_during_setup_;

  /// Small unique identifying value.
  std::size_t id_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpConnection.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPCONNECTION_H */
