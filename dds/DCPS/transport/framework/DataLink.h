/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATALINK_H
#define OPENDDS_DCPS_DATALINK_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/RcEventHandler.h"
#include "ReceiveListenerSetMap.h"
#include "SendResponseListener.h"
#include "TransportDefs.h"
#include "TransportSendStrategy.h"
#include "TransportSendStrategy_rch.h"
#include "TransportStrategy.h"
#include "TransportStrategy_rch.h"
#include "TransportSendControlElement.h"
#include "TransportSendListener.h"
#include "TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/TimeTypes.h"

#include "ace/Event_Handler.h"
#include "ace/Synch_Traits.h"

#include <utility>

#include <iosfwd> // For operator<<() diagnostic formatter.

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_SOCK;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {


class TransportQueueElement;
class ReceivedDataSample;
class DataSampleElement;
class ThreadPerConnectionSendTask;
class TransportClient;
class TransportImpl;

typedef OPENDDS_MAP_CMP(RepoId, DataLinkSet_rch, GUID_tKeyLessThan) DataLinkSetMap;

typedef WeakRcHandle<TransportSendListener> TransportSendListener_wrch;

/**
 * This class manages the reservations based on the associations between datareader
 * and datawriter. It basically delegate the samples to send strategy for sending and
 * deliver the samples received by receive strategy to the listener.
 *
 * Notes about object ownership:
 * 1) Own the send strategy object and receive strategy object.
 * 2) Own ThreadPerConnectionSendTask object which is used when thread_per_connection
 *    is enabled.
 */
class OpenDDS_Dcps_Export DataLink
: public RcEventHandler {

  friend class DataLinkCleanupTask;

public:

  enum ConnectionNotice {
    DISCONNECTED,
    RECONNECTED,
    LOST
  };

  /// A DataLink object is always created by a TransportImpl object.
  /// Thus, the TransportImpl object passed-in here is the object that
  /// created this DataLink.  The ability to specify a priority
  /// for individual links is included for construction so its
  /// value can be available for activating any threads.
  DataLink(TransportImpl& impl, Priority priority, bool is_loopback, bool is_active);
  virtual ~DataLink();

  /// Reactor invokes this after being notified in schedule_stop or cancel_release
  int handle_exception(ACE_HANDLE /* fd */);

  /// Allows DataLink::stop to be done on the reactor thread so that
  /// this thread avoids possibly deadlocking trying to access reactor
  /// to stop strategies or schedule timers
  void schedule_stop(const MonotonicTimePoint& schedule_to_stop_at);
  /// The stop method is used to stop the DataLink prior to shutdown.
  void stop();

  /// The resume_send is used in the case of reconnection
  /// on the subscriber's side.
  void resume_send();

  /// Only called by our TransportImpl object.
  ///
  /// Return Codes: 0 means successful reservation made.
  ///              -1 means failure.
  virtual int make_reservation(const RepoId& remote_subscription_id,
                               const RepoId& local_publication_id,
                               const TransportSendListener_wrch& send_listener,
                               bool reliable);

  /// Only called by our TransportImpl object.
  ///
  /// Return Codes: 0 means successful reservation made.
  ///              -1 means failure.
  virtual int make_reservation(const RepoId& remote_publication_id,
                               const RepoId& local_subscription_id,
                               const TransportReceiveListener_wrch& receive_listener,
                               bool reliable);

  // ciju: Called by LinkSet with locks held
  /// This will release reservations that were made by one of the
  /// make_reservation() methods.  All we know is that the supplied
  /// RepoId is considered to be a remote id.  It could be a
  /// remote subscriber or a remote publisher.
  void release_reservations(RepoId          remote_id,
                            RepoId          local_id,
                            DataLinkSetMap& released_locals);

  void schedule_delayed_release();

  const TimeDuration& datalink_release_delay() const;

  /// Either send or receive listener for this local_id should be
  /// removed from internal DataLink structures so it no longer
  /// receives events.
  void remove_listener(const RepoId& local_id);

  // ciju: Called by LinkSet with locks held
  /// Called by the TransportClient objects that reference this
  /// DataLink.  Used by the TransportClient to send a sample,
  /// or to send a control message. These functions either give the
  /// request to the PerThreadConnectionSendTask when thread_per_connection
  /// configuration is true or just simply delegate to the send strategy.
  void send_start();
  void send(TransportQueueElement* element);
  void send_stop(RepoId repoId);

  // ciju: Called by LinkSet with locks held
  /// This method is essentially an "undo_send()" method.  It's goal
  /// is to remove all traces of the sample from this DataLink (if
  /// the sample is even known to the DataLink).
  virtual RemoveResult remove_sample(const DataSampleElement* sample);

  // ciju: Called by LinkSet with locks held
  virtual void remove_all_msgs(const RepoId& pub_id);

  /// This is called by our TransportReceiveStrategy object when it
  /// has received a complete data sample.  This method will cause
  /// the appropriate TransportReceiveListener objects to be told
  /// that data_received().
  /// If readerId is not GUID_UNKNOWN, only the TransportReceiveListener
  /// with that ID (if one exists) will receive the data.
  int data_received(ReceivedDataSample& sample,
                    const RepoId& readerId = GUID_UNKNOWN);

  /// Varation of data_received() that allows for excluding a subset of readers
  /// by specifying which readers specifically should receive.
  /// Any reader ID that does not appear in the include set will be skipped.
  void data_received_include(ReceivedDataSample& sample, const RepoIdSet& incl);

  /// Obtain a unique identifier for this DataLink object.
  DataLinkIdType id() const;

  /// Our TransportImpl will inform us if it is being shutdown()
  /// by calling this method.
  void transport_shutdown();

  /// Notify the datawriters and datareaders that the connection is
  /// disconnected, lost, or reconnected. The datareader/datawriter
  /// will notify the corresponding listener.
  void notify(ConnectionNotice notice);

  /// Called before release the datalink or before shutdown to let
  /// the concrete DataLink to do anything necessary.
  virtual void pre_stop_i();

  // Call-back from the concrete transport object.
  // The connection has been broken. No locks are being held.
  // Take a snapshot of current associations which will be removed
  // by DataLinkCleanupTask.
  bool release_resources();

  // Used by to inform the send strategy to clear all unsent samples upon
  // backpressure timed out.
  void terminate_send();
  void terminate_send_if_suspended();

  /// This is called on publisher side to see if this link communicates
  /// with the provided sub.
  bool is_target(const RepoId& remote_sub_id);

  /// This is called by DataLinkCleanupTask thread to remove the associations
  /// based on the snapshot in release_resources().
  void clear_associations();

  int handle_timeout(const ACE_Time_Value& tv, const void* arg);
  int handle_close(ACE_HANDLE h, ACE_Reactor_Mask m);

  // Set the DiffServ codepoint of the socket.  This is a stateless
  // method and is here only because this is a convenient common
  // location that can be reached by client code that needs to
  // perform this behavior.
  void set_dscp_codepoint(int cp, ACE_SOCK& socket);

  /// Accessors for the TRANSPORT_PRIORITY value associated with
  /// this link.
  Priority& transport_priority();
  Priority  transport_priority() const;

  bool& is_loopback();
  bool  is_loopback() const;

  bool& is_active();
  bool  is_active() const;

  bool cancel_release();

  /// This allows a subclass to easily create a transport control
  /// sample to send via send_control.
  ACE_Message_Block* create_control(char submessage_id,
                                    DataSampleHeader& header,
                                    Message_Block_Ptr data);

  /// This allows a subclass to send transport control samples over
  /// this DataLink. This is useful for sending transport-specific
  /// control messages between one or more endpoints under this
  /// DataLink's control.
  SendControlStatus send_control(const DataSampleHeader& header, Message_Block_Ptr data);

  /// For a given publication "pub_id", store the total number of corresponding
  /// subscriptions in "n_subs" and given a set of subscriptions
  /// (the "in" sequence), return the subset of the input set "in" which are
  /// targets of this DataLink (see is_target()).
  GUIDSeq* target_intersection(const RepoId& pub_id, const GUIDSeq& in, size_t& n_subs);

  TransportImpl& impl() const;

  void default_listener(const TransportReceiveListener_wrch& trl);
  TransportReceiveListener_wrch default_listener() const;

  typedef WeakRcHandle<TransportClient> TransportClient_wrch;
  typedef std::pair<TransportClient_wrch, RepoId> OnStartCallback;

  void add_pending_on_start(const RepoId& local, const RepoId& remote);
  bool add_on_start_callback(const TransportClient_wrch& client, const RepoId& remote);
  void remove_on_start_callback(const TransportClient_wrch& client, const RepoId& remote);
  void invoke_on_start_callbacks(bool success);
  void invoke_on_start_callbacks(const RepoId& local, const RepoId& remote, bool success);
  void remove_startup_callbacks(const RepoId& local, const RepoId& remote);

  class Interceptor : public ReactorInterceptor {
  public:
    Interceptor(ACE_Reactor* reactor, ACE_thread_t owner) : ReactorInterceptor(reactor, owner) {}
    bool reactor_is_shut_down() const;
  };

  class ImmediateStart : public ReactorInterceptor::Command {
  public:
    ImmediateStart(RcHandle<DataLink> link, WeakRcHandle<TransportClient> client, const RepoId& remote) : link_(link), client_(client), remote_(remote) {}
    void execute();
  private:
    RcHandle<DataLink> link_;
    WeakRcHandle<TransportClient> client_;
    RepoId remote_;
  };

  void set_scheduling_release(bool scheduling_release);

  virtual void send_final_acks (const RepoId& readerid);

  virtual ICE::Endpoint* get_ice_endpoint() const { return 0; }

protected:

  /// This is how the subclass "announces" to this DataLink base class
  /// that this DataLink has now been "connected" and should start
  /// the supplied strategy objects.  This start method is also
  /// going to keep a "copy" of the references to the strategy objects.
  /// Also note that it is acceptable to pass-in a NULL (0)
  /// TransportReceiveStrategy*, but it is assumed that the
  /// TransportSendStrategy* argument is not NULL.
  ///
  /// If the start() method fails to start either strategy, then a -1
  /// is returned.  Otherwise, a 0 is returned.  In the failure case,
  /// if one of the strategy objects was started successfully, then
  /// it will be stopped before the start() method returns -1.
  int start(const TransportSendStrategy_rch& send_strategy,
            const TransportStrategy_rch& receive_strategy,
            bool invoke_all = true);

  /// This announces the "stop" event to our subclass.  The "stop"
  /// event will occur when this DataLink is handling a
  /// release_reservations() call and determines that it has just
  /// released all of the remaining reservations on this DataLink.
  /// The "stop" event will also occur when the TransportImpl
  /// is being shutdown() - we call stop_i() from our
  /// transport_shutdown() method to handle this case.
  virtual void stop_i();

  /// Used to provide unique Ids to all DataLink methods.
  static ACE_UINT64 get_next_datalink_id();

  /// The transport receive strategy object for this DataLink.
  TransportStrategy_rch receive_strategy_;

  friend class ThreadPerConnectionSendTask;

  /// The implementation of the functions that accomplish the
  /// sample or control message delivery. They just simply
  /// delegate to the send strategy.
  void send_start_i();
  virtual void send_i(TransportQueueElement* element, bool relink = true);
  void send_stop_i(RepoId repoId);

  /// For a given local RepoId (publication or subscription), return the list
  /// of remote peer RepoIds (subscriptions or publications) that this link
  /// knows about due to make_reservation().
  GUIDSeq* peer_ids(const RepoId& local_id) const;

  void network_change() const;

private:

  /// Helper function to output the enum as a string to help debugging.
  const char* connection_notice_as_str(ConnectionNotice notice);

  TransportSendListener_rch send_listener_for(const RepoId& pub_id) const;
  TransportReceiveListener_rch recv_listener_for(const RepoId& sub_id) const;

  /// Save current sub and pub association maps for releasing and create
  /// empty maps for new associations.
  void prepare_release();

  virtual bool handle_send_request_ack(TransportQueueElement* element);

  /// Allow derived classes to provide an alternate "customized" queue element
  /// for this DataLink (not shared with other links in the DataLinkSet).
  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element)
  {
    return element;
  }

  virtual void release_remote_i(const RepoId& /*remote_id*/) {}
  virtual void release_reservations_i(const RepoId& /*remote_id*/,
                                      const RepoId& /*local_id*/) {}

  void data_received_i(ReceivedDataSample& sample,
                       const RepoId& readerId,
                       const RepoIdSet& incl_excl,
                       ReceiveListenerSet::ConstrainReceiveSet constrain);

  void notify_reactor();

  typedef ACE_SYNCH_MUTEX     LockType;

  /// Convenience function for diagnostic information.
#ifndef OPENDDS_SAFETY_PROFILE
  friend OpenDDS_Dcps_Export
  std::ostream& operator<<(std::ostream& str, const DataLink& value);
#endif

  /// A boolean indicating if the DataLink has been stopped. This
  /// value is protected by the strategy_lock_.
  bool stopped_;
  MonotonicTimePoint scheduled_to_stop_at_;

  /// Map publication Id value to TransportSendListener.
  typedef OPENDDS_MAP_CMP(RepoId, TransportSendListener_wrch, GUID_tKeyLessThan) IdToSendListenerMap;
  IdToSendListenerMap send_listeners_;

  /// Map subscription Id value to TransportReceieveListener.
  typedef OPENDDS_MAP_CMP(RepoId, TransportReceiveListener_wrch, GUID_tKeyLessThan) IdToRecvListenerMap;
  IdToRecvListenerMap recv_listeners_;

  /// If default_listener_ is not null and this DataLink receives a sample
  /// from a publication GUID that's not in pub_map_, it will call
  /// data_received() on the default_listener_.
  TransportReceiveListener_wrch default_listener_;

  mutable LockType pub_sub_maps_lock_;

  typedef OPENDDS_MAP_CMP(RepoId, ReceiveListenerSet_rch, GUID_tKeyLessThan) AssocByRemote;
  AssocByRemote assoc_by_remote_;

  struct LocalAssociationInfo {
    bool reliable_;
    RepoIdSet associated_;
  };

  typedef OPENDDS_MAP_CMP(RepoId, LocalAssociationInfo, GUID_tKeyLessThan) AssocByLocal;
  AssocByLocal assoc_by_local_;

  /// A reference to the TransportImpl that created this DataLink.
  TransportImpl& impl_;

  /// The id for this DataLink
  ACE_UINT64 id_;

  /// The task used to do the sending. This ThreadPerConnectionSendTask
  /// object is created when the thread_per_connection configuration is
  /// true. It only dedicate to this datalink.
  unique_ptr<ThreadPerConnectionSendTask> thr_per_con_send_task_;

  // snapshot of associations when the release_resources() is called.
  AssocByLocal assoc_releasing_;

  /// TRANSPORT_PRIORITY value associated with the link.
  Priority transport_priority_;

  bool scheduling_release_;

protected:

  typedef ACE_Guard<LockType> GuardType;

  /// The transport send strategy object for this DataLink.
  TransportSendStrategy_rch send_strategy_;

  LockType strategy_lock_;
  typedef OPENDDS_MAP_CMP(RepoId, TransportClient_wrch, GUID_tKeyLessThan) RepoToClientMap;
  typedef OPENDDS_MAP_CMP(RepoId, RepoToClientMap, GUID_tKeyLessThan) OnStartCallbackMap;
  OnStartCallbackMap on_start_callbacks_;
  typedef OPENDDS_MAP_CMP(RepoId, RepoIdSet, GUID_tKeyLessThan) PendingOnStartsMap;
  PendingOnStartsMap pending_on_starts_;

  /// Configurable delay in milliseconds that the datalink
  /// should be released after all associations are removed.
  TimeDuration datalink_release_delay_;

  /// Allocators for data and message blocks used by transport
  /// control samples when send_control is called.
  unique_ptr<MessageBlockAllocator> mb_allocator_;
  unique_ptr<DataBlockAllocator> db_allocator_;

  /// Is remote attached to same transport ?
  bool is_loopback_;
  /// Is pub or sub ?
  bool is_active_;
  bool started_;

  /// Listener for TransportSendControlElements created in send_control
  SendResponseListener send_response_listener_;

  Interceptor interceptor_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_DATALINK_H */
