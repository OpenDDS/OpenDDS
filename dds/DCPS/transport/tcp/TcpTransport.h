/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPTRANSPORT_H
#define OPENDDS_TCPTRANSPORT_H

#include "Tcp_export.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "TcpInst_rch.h"
#include "TcpDataLink_rch.h"
#include "TcpConnection.h"
#include "TcpConnection_rch.h"

#include "dds/DCPS/ReactorTask_rch.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"

#include "ace/INET_Addr.h"
#include "ace/Hash_Map_Manager.h"
#include "ace/Synch_Traits.h"
#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpAcceptor;
class TcpConnectionReplaceTask;

/**
 * This class provides the "Tcp" transport specific implementation.
 * It creates the acceptor for listening the incoming requests using
 * TCP and maintains a collection of TCP specific connections/datalinks.
 *
 * Notes about object ownership:
 * 1) Own the datalink objects, passive connection objects, acceptor object
 *    and TcpConnectionReplaceTask object(used during reconnecting).
 * 2) Reference to TransportReactorTask object owned by base class.
 */

class OpenDDS_Tcp_Export TcpTransport
  : public TransportImpl
{
public:

  explicit TcpTransport(TcpInst& inst);
  virtual ~TcpTransport();

  int fresh_link(TcpConnection_rch connection);

  virtual void unbind_link(DataLink* link);
  TcpInst& config() const;

private:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const RepoId& remote_id);

  virtual bool configure_i(TcpInst& config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& local_info, ConnectionInfoFlags flags) const;

  /// Called by the DataLink to release itself.
  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "tcp"; }

  void async_connect_failed(const PriorityKey& key);

  /// The TcpConnection is our friend.  It tells us when it
  /// has been created (by our acceptor_), and is seeking the
  /// DataLink that should be (or will be) expecting the passive
  /// connection.
  friend class TcpConnection;
  friend class TcpDataLink;

  /// Called by the TcpConnection object when it has been
  /// created by the acceptor and needs to be attached to a DataLink.
  /// The DataLink may or may not already be created and waiting
  /// for this passive connection to appear.
  void passive_connection(const ACE_INET_Addr& remote_address,
                          const TcpConnection_rch& connection);

  bool find_datalink_i(const PriorityKey& key, TcpDataLink_rch& link,
                       const TransportClient_rch& client, const RepoId& remote_id);

  /// Code common to make_active_connection() and
  /// make_passive_connection().
  int connect_tcp_datalink(TcpDataLink& link,
                           const TcpConnection_rch& connection);

  PriorityKey blob_to_key(const TransportBLOB& remote,
                          Priority priority,
                          bool active);

  /// Map Type: (key) PriorityKey to (value) TcpDataLink_rch
  typedef ACE_Hash_Map_Manager_Ex
            <PriorityKey,
            TcpDataLink_rch,
            ACE_Hash<PriorityKey>,
            ACE_Equal_To<PriorityKey>,
            ACE_Null_Mutex>              AddrLinkMap;

  typedef OPENDDS_MAP(PriorityKey, TcpDataLink_rch)   LinkMap;
  typedef OPENDDS_MAP(PriorityKey, TcpConnection_rch) ConnectionMap;

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

// TBD SOON - Something needs to protect the tcp_config_ reference
//            because it gets set in our configure() method, and
//            dropped in our shutdown_i() method.  Maybe we can just
//            assume that configure() can remain unlocked (why not lock it
//            though - it isn't in the send or receive path?)
//            Step back and take a look at when the various locks in here
//            get used - and if not in the direct send or receive path,
//            maybe we can simplify this at the expense of a little more
//            locking (ie, longer critical sections).  And the base
//            class has a lock that might work for us - check out if
//            our base class should do the locking, and then we can
//            assume it has been done for us (in various situations).

  /// Used to accept passive connections on our local_address_.
  unique_ptr<TcpAcceptor> acceptor_;

  class Connector : public ACE_Connector<TcpConnection, ACE_SOCK_Connector>
  {
    virtual int fini();
  };

  /// Open TcpConnections using non-blocking connect.
  Connector connector_;

  /// This is the map of connected DataLinks.
  AddrLinkMap links_;
  AddrLinkMap pending_release_links_;

  /// This lock is used to protect the links_ data member.
  LockType links_lock_;

  /// Map of passive connection objects that need to be paired
  /// with a DataLink.
  ConnectionMap connections_;

  /// This protects the connections_ and the pending_connections_
  /// data members.
  LockType connections_lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_TCPTRANSPORT_H */
