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
#include "TcpTransport_rch.h"

#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "dds/DCPS/TimeTypes.h"

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/INET_Addr.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpConnection
  : public RcObject
  , public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> {
public:

  /// States are used during reconnecting.
  enum ReconnectState {
    INIT_STATE,
    LOST_STATE,
    RECONNECTED_STATE,
    ACTIVE_RECONNECTING_STATE,
    ACTIVE_WAITING_STATE,
    PASSIVE_WAITING_STATE,
    PASSIVE_TIMEOUT_CALLED_STATE
  };

  /// Passive side constructor (acceptor)
  TcpConnection();

  /// Active side constructor (connector)
  TcpConnection(const ACE_INET_Addr& remote_address,
                Priority priority,
                const TcpInst& config);

  virtual ~TcpConnection();

  std::size_t& id();

  /// Protocol setup (handshake) on the active side.
  /// The local address is sent to the remote (passive) side to
  /// identify ourselves to the remote side.
  int active_open();
  int active_reconnect_open();

  int passive_open(void*);

  /// This will be called by the DataLink (that "owns" us) when
  /// the TcpTransport has been told to shutdown(), or when
  /// the DataLink finds itself no longer needed, and is
  /// "self-releasing".
  void disconnect();

  // Note that the acceptor or connector that calls the open() method will pass
  // itself in as a void*.
  virtual int open(void* arg);

  TcpSendStrategy_rch send_strategy();
  TcpReceiveStrategy_rch receive_strategy();

  /// We pass this "event" along to the receive_strategy.
  virtual int handle_input(ACE_HANDLE);

  /// Handle back pressure when sending.
  virtual int handle_output(ACE_HANDLE);

  virtual int close(u_long);
  virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);

  void set_sock_options(const TcpInst* tcp_config);

  /// Return true if the object represents the connector side, otherwise
  /// it's the acceptor side. The acceptor/connector role is not changed
  /// when re-establishing the connection.
  bool is_connector() const;

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

  TcpTransport_rch impl() { return impl_; }

  /// Access TRANSPORT_PRIORITY.value policy value if set.
  Priority& transport_priority();
  Priority  transport_priority() const;

  virtual ACE_Event_Handler::Reference_Count add_reference();
  virtual ACE_Event_Handler::Reference_Count remove_reference();

  OPENDDS_POOL_ALLOCATION_FWD

private:

  /// Handle the logic after an active connection has been established
  int on_active_connection_established();

  void active_reconnect_i();
  void passive_reconnect_i();

  void notify_connection_lost();
  void handle_stop_reconnecting();

  /// During the connection setup phase, the passive side sets passive_setup_,
  /// redirecting handle_input() events here (there is no recv strategy yet).
  int handle_setup_input(ACE_HANDLE h);

  const std::string& config_name() const;

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  /// Lock to synchronize state between reactor and non-reactor threads.
  LockType reconnect_lock_;

  /// Flag indicate this connection object is the connector or acceptor.
  bool is_connector_;

  /// Remote address.
  ACE_INET_Addr remote_address_;

  /// Local address.
  ACE_INET_Addr local_address_;

  /// The configuration used by this connection.
  const TcpInst* tcp_config_;

  /// Datalink object which is needed for connection lost callback.
  TcpDataLink_rch link_;

  /// Impl object which is needed for connection objects and reconnect task
  TcpTransport_rch impl_;

  /// The state indicates each step of the reconnecting.
  ReconnectState reconnect_state_;

  /// TRANSPORT_PRIORITY.value policy value.
  Priority transport_priority_;

  /// shutdown flag
  bool shutdown_;

  bool passive_setup_;
  ACE_Message_Block passive_setup_buffer_;
  TcpTransport* transport_during_setup_;

  /// Small unique identifying value.
  std::size_t id_;
  int conn_retry_counter_;

  /// Get name of the current reconnect state as a string.
  const char* reconnect_state_string() const;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpConnection.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPCONNECTION_H */
