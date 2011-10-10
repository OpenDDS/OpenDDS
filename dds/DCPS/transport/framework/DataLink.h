/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATALINK_H
#define OPENDDS_DCPS_DATALINK_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcObject_T.h"
#include "ReceiveListenerSetMap.h"
#include "RepoIdSetMap.h"
#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "TransportSendStrategy.h"
#include "TransportSendStrategy_rch.h"
#include "TransportStrategy.h"
#include "TransportStrategy_rch.h"
#include "TransportSendControlElement.h"
#include "TransportSendListener.h"
#include "TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#include "ace/Synch.h"
#include "ace/Event_Handler.h"

#include <list>
#include <map>

#include <iosfwd> // For operator<<() diagnostic formatter.

class ACE_SOCK;

namespace OpenDDS {
namespace DCPS {

class  TransportReceiveListener;
class  TransportSendListener;
class  TransportQueueElement;
class  DataLinkSetMap;
class  ReceivedDataSample;
struct DataSampleListElement;
class  ThreadPerConnectionSendTask;

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
  : public RcObject<ACE_SYNCH_MUTEX>,
    public ACE_Event_Handler {

  friend class DataLinkCleanupTask;

public:

  enum ConnectionNotice {
    DISCONNECTED,
    RECONNECTED,
    LOST
  };

  /// A DataLink object is always created by a TransportImpl object.
  /// Thus, the TransportImpl object passed-in here is the object that
  /// created this DataLink.  The ability to specifiy a priority
  /// for individual links is included for construction so its
  /// value can be available for activating any threads.
  DataLink(TransportImpl* impl, CORBA::Long priority, bool is_loopback, bool is_active);
  virtual ~DataLink();

  /// The stop method is used to stop the DataLink prior to shutdown.
  void stop();

  /// The resume_send is used in the case of reconnection
  /// on the subscriber's side.
  void resume_send();

  // ciju: Called by TransportImpl
  /// This is for a remote subscriber_id and local publisher_id
  int make_reservation(
    RepoId subscriber_id,
    RepoId publisher_id,
    TransportSendListener* send_listener);

  // ciju: Called by TransportImpl
  /// This is for a remote publisher_id and a local subscriber_id.
  /// The TransportReceiveListener is associated with the local
  /// subscriber_id.
  int make_reservation(RepoId                    publisher_id,
                       RepoId                    subscriber_id,
                       TransportReceiveListener* receive_listener);

  // ciju: Called by LinkSet with locks held
  /// This will release reservations that were made by one of the
  /// make_reservation() methods.  All we know is that the supplied
  /// RepoId is considered to be a remote id.  It could be a
  /// remote subscriber or a remote publisher.
  void release_reservations(RepoId          remote_id,
                            RepoId          local_id,
                            DataLinkSetMap& released_locals);

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
  void send_stop();

  // ciju: Called by LinkSet with locks held
  /// This method is essentially an "undo_send()" method.  It's goal
  /// is to remove all traces of the sample from this DataLink (if
  /// the sample is even known to the DataLink).
  RemoveResult remove_sample(const DataSampleListElement* sample);

  // ciju: Called by LinkSet with locks held
  void remove_all_msgs(RepoId pub_id);

  /// This is called by our TransportReceiveStrategy object when it
  /// has received a complete data sample.  This method will cause
  /// the appropriate TransportReceiveListener objects to be told
  /// that data_received().
  int data_received(ReceivedDataSample& sample);

  /// SAMPLE_ACK message received on this link.
  void ack_received(ReceivedDataSample& sample);

  /// Obtain a unique identifier for this DataLink object.
  DataLinkIdType id() const;

  /// Our TransportImpl will inform us if it is being shutdown()
  /// by calling this method.
  void transport_shutdown();

  /// Notify the datawriters and datareaders that the connection is
  /// disconnected, lost, or reconnected. The datareader/datawriter
  /// will notify the corresponding listener.
  void notify(ConnectionNotice notice);

  void notify_connection_deleted();

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

  /// This is called on publisher side to see if this link communicates
  /// with the provided sub.
  bool is_target(const RepoId& sub_id);

  /// Check if the remote_id/local_id is associated with this link
  /// and if they are the last assoication using this link.
  bool exist(const RepoId& remote_id,
             const RepoId& local_id,
             const bool&   pub_side,
             bool& last);

  /// This is called by DataLinkCleanupTask thread to remove the associations
  /// based on the snapshot in release_resources().
  void clear_associations();

  // The second parameter (zero or non-zero) indicates whether the
  // impl_->release_datalink() needs be called. The impl_->release_datalink()
  // may result in the DataLink object deletion. Any access of DataLink object
  // after that will get access violation. It's ok to release link in
  // remove_association path (release_reservations()), but it would cause
  // access violation during normal transport shutdown(transport_shutdown()).
  int handle_timeout(const ACE_Time_Value &tv,
                     const void * arg = 0);

  // Set the DiffServ codepoint of the socket.  This is a stateless
  // method and is here only because this is a convenient common
  // location that can be reached by client code that needs to
  // perform this behavior.
  void set_dscp_codepoint(int cp, ACE_SOCK& socket);

  /// Accessors for the TRANSPORT_PRIORITY value associated with
  /// this link.
  CORBA::Long& transport_priority();
  CORBA::Long  transport_priority() const;

  bool& is_loopback();
  bool  is_loopback() const;

  bool& is_active();
  bool  is_active() const;

  bool cancel_release();

  /// This allows a subclass to easily create a transport control
  /// sample to send via send_control.
  ACE_Message_Block* create_control(char submessage_id,
                                    DataSampleHeader& header,
                                    ACE_Message_Block* data);

  /// This allows a subclass to send transport control samples over
  /// this DataLink. This is useful for sending transport-specific
  /// control messages between one or more endpoints under this
  /// DataLink's control.
  SendControlStatus send_control(const DataSampleHeader& header, ACE_Message_Block* data);

  /// Return the subset of the input set which are also targets of
  /// this DataLink (see is_target()).
  GUIDSeq* target_intersection(const GUIDSeq& in);

  CORBA::ULong num_targets() const;
  RepoIdSet_rch get_targets() const;

  TransportImpl_rch impl() const;

  /// A second thread may try to use this DataLink before the creating thread
  /// has completed the start() method.  In this case, call wait_for_start()
  /// to block the current thread until start() completes.
  void wait_for_start();
  void unblock_wait_for_start();

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
            const TransportStrategy_rch& receive_strategy);

  /// This announces the "start" event to our subclass.  The "start"
  /// event will occur when this DataLink is handling its first
  /// make_reservation() call.
  virtual int start_i();

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
  /// sample or control message delivery. IThey just simply
  /// delegate to the send strategy.
  void send_start_i();
  void send_i(TransportQueueElement* element, bool relink = true);
  void send_stop_i();

  /// For a given local RepoId (publication or subscription), return the list
  /// of remote peer RepoIds (subscriptions or publications) that this link
  /// knows about due to make_reservation().
  GUIDSeq* peer_ids(const RepoId& local_id) const;

private:

  /// Helper function to output the enum as a string to help debugging.
  const char* connection_notice_as_str(ConnectionNotice notice);

  /// Used by release_reservations() once it has determined that the
  /// remote_id/local_id being released is, in fact,
  /// a remote subscriber id/a local publisher id.
  void release_remote_subscriber(RepoId          subscriber_id,
                                 RepoId          publisher_id,
                                 RepoIdSet_rch&  pubid_set,
                                 DataLinkSetMap& released_publishers);

  /// Used by release_reservations() once it has determined that the
  /// remote_id/local_id being released is, in fact,
  /// a remote publisher id/a local subscriber.
  void release_remote_publisher
  (RepoId               publisher_id,
   RepoId               subscriber_id,
   ReceiveListenerSet_rch& listener_set,
   DataLinkSetMap&      released_subscribers);

  TransportSendListener* send_listener_for(const RepoId& pub_id) const;
  TransportReceiveListener* recv_listener_for(const RepoId& sub_id) const;

  /// Save current sub and pub association maps for releasing and create
  /// empty maps for new associations.
  void prepare_release();

  /// Allow derived classes to provide an alternate "customized" queue element
  /// for this DataLink (not shared with other links in the DataLinkSet).
  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element)
  {
    return element;
  }

  virtual void release_remote_i(const RepoId& /*remote_id*/) {}

  typedef ACE_SYNCH_MUTEX     LockType;

  /// Convenience function for diagnostic information.
  friend OpenDDS_Dcps_Export
  std::ostream& operator<<(std::ostream& str, const DataLink& value);

  /// A boolean indicating if the DataLink has been stopped. This
  /// value is protected by the strategy_lock_.
  bool stopped_;

  /// Map publication Id value to TransportSendListener.
  typedef std::map<RepoId, TransportSendListener*, GUID_tKeyLessThan> IdToSendListenerMap;
  IdToSendListenerMap send_listeners_;

  /// Map subscription Id value to TransportReceieveListener.
  typedef std::map<RepoId, TransportReceiveListener*, GUID_tKeyLessThan> IdToRecvListenerMap;
  IdToRecvListenerMap recv_listeners_;

  /// Map associating each publisher_id with a set of
  /// TransportReceiveListener objects (each with an associated
  /// subscriber_id).  This map is used for delivery of incoming
  /// (aka, received) data samples.
  ReceiveListenerSetMap pub_map_;

  mutable LockType pub_map_lock_;

  /// Map associating each subscriber_id with the set of publisher_ids.
  /// In essence, the pub_map_ and sub_map_ are the "mirror image" of
  /// each other, where we have a many (publishers) to many (subscribers)
  /// association being managed here.
  RepoIdSetMap sub_map_;

  mutable LockType sub_map_lock_;

  /// A (smart) pointer to the TransportImpl that created this DataLink.
  TransportImpl_rch impl_;

  /// The id for this DataLink
  ACE_UINT64 id_;

  /// The task used to do the sending. This ThreadPerConnectionSendTask
  /// object is created when the thread_per_connection configuration is
  /// true. It only dedicate to this datalink.
  ThreadPerConnectionSendTask* thr_per_con_send_task_;

  RepoIdSet released_local_pubs_;
  RepoIdSet released_local_subs_;

  LockType released_local_lock_;

  // snapshot of associations when the release_resource() is called.
  ReceiveListenerSetMap pub_map_releasing_;
  RepoIdSetMap sub_map_releasing_;

  /// TRANSPORT_PRIORITY value associated with the link.
  CORBA::Long transport_priority_;

protected:

  typedef ACE_Guard<LockType> GuardType;

  /// The transport send strategy object for this DataLink.
  TransportSendStrategy_rch send_strategy_;

  LockType strategy_lock_;
  ACE_SYNCH_CONDITION strategy_condition_;

  /// Configurable delay in milliseconds that the datalink
  /// should be released after all associations are removed.
  ACE_Time_Value datalink_release_delay_;

  /// Allocator for TransportSendControlElements created when
  /// send_control is called.
  TransportSendControlElementAllocator* send_control_allocator_;

  /// Allocators for data and message blocks used by transport
  /// control samples when send_control is called.
  MessageBlockAllocator* mb_allocator_;
  DataBlockAllocator* db_allocator_;

  /// Is remote attached to same transport ?
  bool is_loopback_;
  /// Is pub or sub ?
  bool is_active_;
  bool start_failed_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_DATALINK_H */
