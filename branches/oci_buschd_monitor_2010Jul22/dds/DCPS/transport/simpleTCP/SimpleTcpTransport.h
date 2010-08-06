/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPTRANSPORT_H
#define OPENDDS_DCPS_SIMPLETCPTRANSPORT_H

#include "SimpleTcp_export.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "SimpleTcpConfiguration_rch.h"
#include "SimpleTcpDataLink_rch.h"
#include "SimpleTcpConnection_rch.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "ace/INET_Addr.h"
#include "ace/Hash_Map_Manager.h"
#include "ace/Synch.h"

namespace OpenDDS {
namespace DCPS {

class SimpleTcpAcceptor;
class SimpleTcpConnectionReplaceTask;

/**
 * This class provides the "SimpleTcp" transport specific implementation.
 * It creates the acceptor for listening the incoming requests using
 * TCP and maintains a collection of TCP specific connections/datalinks.
 *
 * Notes about object ownership:
 * 1) Own the datalink objects, passive connection objects, acceptor object
 *    and SimpleTcpConnectionReplaceTask object(used during reconnecting).
 * 2) Reference to TransportReactorTask object owned by base class.
 */
class SimpleTcp_Export SimpleTcpTransport : public TransportImpl {
public:

  SimpleTcpTransport();
  virtual ~SimpleTcpTransport();

  SimpleTcpConfiguration* get_configuration();

  int fresh_link(SimpleTcpConnection_rch connection);

protected:

  /// Either find a suitable DataLink that already exists (and is
  /// connected), or create one, connect it, save it off for reuse,
  /// and return it.
  virtual DataLink* find_or_create_datalink(
    RepoId                  local_id,
    const AssociationData*  remote_association,
    CORBA::Long             priority,
    bool                    active);

  virtual int configure_i(TransportConfiguration* config);

  virtual void shutdown_i();
  virtual void pre_shutdown_i();

  virtual int connection_info_i
  (TransportInterfaceInfo& local_info) const;

  /// Called by the DataLink to release itself.
  virtual void release_datalink_i(DataLink* link,
                                  bool release_pending);

private:

  /// The SimpleTcpConnection is our friend.  It tells us when it
  /// has been created (by our acceptor_), and is seeking the
  /// DataLink that should be (or will be) expecting the passive
  /// connection.
  friend class SimpleTcpConnection;
  friend class SimpleTcpDataLink;

  /// Called by the SimpleTcpConnection object when it has been
  /// created by the acceptor and needs to be attached to a DataLink.
  /// The DataLink may or may not already be created and waiting
  /// for this passive connection to appear.
  /// Note that the SimpleTcpConnection* "ownership" is given away
  /// to the passive_connection() call.
  void passive_connection(const ACE_INET_Addr& remote_address,
                          SimpleTcpConnection* connection);

  /// Called by find_or_create_datalink().
  int make_active_connection(const ACE_INET_Addr& remote_address,
                             SimpleTcpDataLink*   link);

  /// Called by find_or_create_datalink().
  int make_passive_connection(const ACE_INET_Addr& remote_address,
                              SimpleTcpDataLink*   link);

  /// Code common to make_active_connection() and
  /// make_passive_connection().
  int connect_datalink(SimpleTcpDataLink*   link,
                       SimpleTcpConnection* connection);

  void wait_for_connection (const ACE_Time_Value& abs_timeout);

  /// Map Type: (key) PriorityKey to (value) SimpleTcpDataLink_rch
  typedef ACE_Hash_Map_Manager_Ex
  <PriorityKey,
  SimpleTcpDataLink_rch,
  ACE_Hash<PriorityKey>,
  ACE_Equal_To<PriorityKey>,
  ACE_Null_Mutex>              AddrLinkMap;

  typedef std::map<PriorityKey, SimpleTcpDataLink_rch>   LinkMap;
  typedef std::map<PriorityKey, SimpleTcpConnection_rch> ConnectionMap;

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

  typedef ACE_Reverse_Lock<ReservationLockType> Reverse_Lock_t;
  Reverse_Lock_t reverse_reservation_lock_;

// TBD SOON - Something needs to protect the tcp_config_ reference
//            because it gets set in our configure() method, and
//            dropped in our shutdown_i() method.  Maybe we can just
//            assume that configure() can remain unlocked (why not lock it
//            though - it isn't in the send or receive path?)
//            Step back and take a look at when the various locks in here
//            get used - and if not in the direct send or recieve path,
//            maybe we can simplify this at the expense of a little more
//            locking (ie, longer critical sections).  And the base
//            class has a lock that might work for us - check out if
//            our base class should do the locking, and then we can
//            assume it has been done for us (in various situations).

  /// Used to accept passive connections on our local_address_.
  SimpleTcpAcceptor* acceptor_;

  /// Our configuration object, supplied to us in config_i().
  SimpleTcpConfiguration_rch tcp_config_;

  /// This is the map of connected DataLinks.
  AddrLinkMap links_;
  AddrLinkMap pending_release_links_;

  /// This lock is used to protect the links_ data member.
  LockType links_lock_;

  /// Map of passive connection objects that need to be paired
  /// with a DataLink.
  ConnectionMap connections_;

  /// Condition that will be signal()'ed whenever something has been
  /// inserted into connections_.
  ConditionType connections_updated_;

  /// This protects the connections_ and the connections_updated_
  /// data members.
  LockType connections_lock_;

  /// We need the reactor for our Acceptor.
  TransportReactorTask_rch reactor_task_;

  /// This task is used to resolve some deadlock situation
  /// duing reconnecting.
  /// TODO: reuse the reconnect_task in the SimpleTcpConnection
  ///       for new connection checking.
  SimpleTcpConnectionReplaceTask* con_checker_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_SIMPLETCPTRANSPORT_H */
