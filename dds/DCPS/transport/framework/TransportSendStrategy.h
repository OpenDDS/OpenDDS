/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSENDSTRATEGY_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSENDSTRATEGY_H

#include "BasicQueue_T.h"
#include "ThreadSynchStrategy_rch.h"
#include "ThreadSynchWorker.h"
#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "TransportHeader.h"
#include "TransportReplacedElement.h"
#include "TransportRetainedElement.h"

#include <dds/DCPS/dcps_export.h>
#include "dds/DCPS/Atomic.h"
#include <dds/DCPS/DataBlockLockPool.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/Dynamic_Cached_Allocator_With_Overflow_T.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/RcObject.h>

#if defined(OPENDDS_SECURITY)
#include <dds/DdsSecurityCoreC.h>
#include <dds/DCPS/security/framework/SecurityConfig.h>
#include <dds/DCPS/security/framework/SecurityConfig_rch.h>
#endif

#include <ace/Synch_Traits.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ThreadSynch;
class ThreadSynchResource;
class TransportQueueElement;
class TransportSendElement;
class TransportSendBuffer;
class DataSampleElement;
class QueueRemoveVisitor;
class PacketRemoveVisitor;
class TransportImpl;

/**
 * This class provides methods to fill packets with samples for sending
 * and handles backpressure. It maintains the list of samples in current
 * packets and also the list of samples queued during backpressure. A thread
 * per connection is created to handle the queued samples.
 *
 * Notes for the object ownership:
 * 1) Owns ThreadSynch object, list of samples in current packet and list
 *    of samples in queue.
 */
class OpenDDS_Dcps_Export TransportSendStrategy
  : public ThreadSynchWorker {
public:
  virtual ~TransportSendStrategy();

  /// Assigns an optional send buffer.
  void send_buffer(TransportSendBuffer* send_buffer);

  /// Start the TransportSendStrategy.  This happens once, when
  /// the DataLink that "owns" this strategy object has established
  /// a connection.
  int start();

  /// Stop the TransportSendStrategy.  This happens once, when the
  /// DataLink that "owns" this strategy object is going away.
  void stop();

  /// Invoked prior to one or more send() invocations from a particular
  /// TransportClient.
  void send_start();

  /// Our DataLink has been requested by some particular
  /// TransportClient to send the element.
  void send(TransportQueueElement* element, bool relink = true);

  /// Invoked after one or more send() invocations from a particular
  /// TransportClient.
  void send_stop(GUID_t repoId);

  /// Our DataLink has been requested by some particular
  /// TransportClient to remove the supplied sample
  /// (basically, an "unsend" attempt) from this strategy object.
  RemoveResult remove_sample(const DataSampleElement* sample);

  void remove_all_msgs(const GUID_t& pub_id);

  /// Called by our ThreadSynch object when we should be able to
  /// start sending any partial packet bytes and/or compose a new
  /// packet using elements from the queue_.
  ///
  /// Returns 0 to indicate that the ThreadSynch object doesn't need
  /// to call perform_work() again since the queue (and any unsent
  /// packet bytes) has been drained, and the mode_ has been switched
  /// to MODE_DIRECT.
  ///
  /// Returns 1 to indicate that there is more work to do, and the
  /// ThreadSynch object should have this perform_work() method
  /// called again.
  virtual WorkOutcome perform_work();

  /// The subclass needs to provide the implementation
  /// for re-establishing the datalink. This is called
  /// when send returns an error.
  virtual void relink(bool do_suspend = true);

  /// This is called when first time reconnect is attempted. The send mode
  /// is set to MODE_SUSPEND. Messages are queued at this state.
  void suspend_send();

  /// This is called when connection is lost and reconnect succeeds.
  /// The send mode is set to the mode before suspend which is either MODE_QUEUE
  /// or MODE_DIRECT.
  void resume_send();

  /// This is called whenver the connection is lost and reconnect fails.
  /// It removes all samples in the backpressure queue and packet queue.
  void terminate_send(bool graceful_disconnecting = false);
  virtual void terminate_send_if_suspended();

  // Moved clear() declaration below as Enums can't be forward declared.

  /// Let the subclass stop.
  virtual void stop_i() = 0;

  /// Let the subclass start.
  virtual bool start_i() { return true; }

  void link_released(bool flag);

  bool isDirectMode();

  typedef BasicQueue<TransportQueueElement> QueueType;

  /// Convert ACE_Message_Block chain into iovec[] entries for send(),
  /// returns number of iovec[] entries used (up to MAX_SEND_BLOCKS).
  /// Precondition: iov must be an iovec[] of size MAX_SEND_BLOCKS or greater.
  static int mb_to_iov(const ACE_Message_Block& msg, iovec* iov);

  // Subclasses which make use of acceptors should override
  // this method and return the peer handle.
  virtual ACE_HANDLE get_handle();

  void deliver_ack_request(TransportQueueElement* element);

  /// Put the maximum UDP payload size here so that it can be shared by all
  /// UDP-based transports.  This is the worst-case (conservative) value for
  /// UDP/IPv4.  If there are no IP options, or if IPv6 is used, it could
  /// actually be a little larger.
  static const size_t UDP_MAX_MESSAGE_SIZE = 65466;

  /// Alternative to TransportSendStrategy::send for fragmentation
  ///
  /// @param original_element data sample to send, may be larger than max msg size
  /// @param elements_to_send populated by this method with either original_element
  ///                         or fragments created from it.  Elements need to be
  ///                         cleaned up by the caller using data_delivered or
  ///                         data_dropped.
  /// @return operation succeeded
  bool fragmentation_helper(
    TransportQueueElement* original_element, TqeVector& elements_to_send);

protected:

  TransportSendStrategy(std::size_t id,
                        const TransportImpl_rch& transport,
                        ThreadSynchResource* synch_resource,
                        Priority priority,
                        const ThreadSynchStrategy_rch& thread_sync_strategy);


  // Only our subclass knows how to do this.
  // Third arg is the "back-pressure" flag.  If send_bytes() returns
  // -1 and the bp == 1, then it isn't really an error - it is
  // backpressure.
  virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);

  virtual ssize_t non_blocking_send(const iovec iov[], int n, int& bp);

  virtual ssize_t send_bytes_i(const iovec iov[], int n) = 0;

  /// Specific implementation processing of prepared packet header.
  virtual void prepare_header_i();

  /// Specific implementation processing of prepared packet.
  virtual void prepare_packet_i();

  TransportQueueElement* current_packet_first_element() const;

  /// The maximum size of a message allowed by the this TransportImpl, or 0
  /// if there is no such limit.  This is expected to be a constant, for example
  /// UDP/IPv4 can send messages of up to 65466 bytes.
  /// The transport framework will use the returned value (if > 0) to
  /// fragment larger messages.  This fragmentation and
  /// reassembly will be transparent to the user.
  virtual size_t max_message_size() const;

  /// Set graceful disconnecting flag.
  void set_graceful_disconnecting(bool flag);

  virtual void add_delayed_notification(TransportQueueElement* element);

  /// If delayed notifications were queued up, issue those callbacks here.
  /// The default match is "match all", otherwise match can be used to specify
  /// either a certain individual packet or a publication id.
  /// Returns true if anything in the delayed notification list matched.
  bool send_delayed_notifications(const TransportQueueElement::MatchCriteria* match = 0);

#ifdef OPENDDS_SECURITY
  virtual Security::SecurityConfig_rch security_config() const { return Security::SecurityConfig_rch(); }
#endif

private:

  enum SendPacketOutcome {
    OUTCOME_COMPLETE_SEND,
    OUTCOME_PARTIAL_SEND,
    OUTCOME_BACKPRESSURE,
    OUTCOME_PEER_LOST,
    OUTCOME_SEND_ERROR
  };

  /// Called from send() when it is time to attempt to send our
  /// current packet to the socket while in MODE_DIRECT mode_.
  /// If backpressure occurs, our current packet will be adjusted
  /// to account for bytes that were sent, and the mode will be
  /// changed to MODE_QUEUE.
  /// If no backpressure occurs (ie, the entire packet is sent), then
  /// our current packet will be "reset" to be an empty packet following
  /// the send.
  void direct_send(bool relink);

  /// This method is used while in MODE_QUEUE mode, and a new packet
  /// needs to be formulated using elements from the queue_.  This is
  /// the first step of formulating the new packet.  It will extract
  /// elements from the queue_ and insert those elements into the
  /// pkt_elems_ collection.
  ///
  /// After this step has been done, the prepare_packet() step can
  /// be performed, followed by the actual send_packet() call.
  void get_packet_elems_from_queue();

  /// This method is responsible for updating the packet header.
  /// Called exclusively by prepare_packet.
  void prepare_header();

  /// This method is responsible for actually "creating" the current
  /// send packet using the packet header and the collection of
  /// packet elements that are to make-up the packet's contents.
  void prepare_packet();

  /// This is called to send the current packet.  The current packet
  /// will either be a "partially sent" packet, or a packet that has
  /// just been prepared via a call to prepare_packet().
  SendPacketOutcome send_packet();

  /// Form an IOV and call the send_bytes() template method.
  ssize_t do_send_packet(const ACE_Message_Block* packet, int& bp);

#ifdef OPENDDS_SECURITY
  /// Derived classes can override to transform the data right before it's
  /// sent.  If the returned value is non-NULL it will be sent instead of
  /// sending the parameter.  If the returned value is NULL the original
  /// message will be dropped.
  virtual ACE_Message_Block* pre_send_packet(const ACE_Message_Block* m)
  {
    return m->duplicate();
  }
#endif

  /// This is called from the send_packet() method after it has
  /// sent at least one byte from the current packet.  This method
  /// will update the current packet appropriately, as well as deal
  /// with all of the release()'ing of fully sent ACE_Message_Blocks,
  /// and the data_delivered() calls on the fully sent elements.
  /// Returns 0 if the entire packet was sent, and returns 1 if
  /// the entire packet was not sent.
  int adjust_packet_after_send(ssize_t num_bytes_sent);

  /**
   * How much space is available in packet with a given used space before we
   * reach one of the limits: max_message_size() [transport's inherent
   * limitation] or max_size_ [user's configured limit]
   */
  size_t space_available(size_t already_used = 0) const;

  /**
   * Like above, but use the current packet.
   */
  size_t current_space_available() const;

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

public:
  enum SendMode {
    // MODE_NOT_SET is used as the initial value of mode_before_suspend_ so
    // we can check if the resume_send is paired with suspend_send.
    MODE_NOT_SET,
    // Send out the sample with current packet.
    MODE_DIRECT,
    // The samples need be queued because of the backpressure or partial send.
    MODE_QUEUE,
    // The samples need be queued because the connection is lost and we are
    // trying to reconnect.
    MODE_SUSPEND,
    // The samples need be dropped since we lost connection and could not
    // reconnect.
    MODE_TERMINATED
  };

  /// Clear queued messages and messages in current packet and set the
  /// current mode to new_mod if the current mode equals old_mode or
  /// old_mode is MODE_NOT_SET.

  void clear(SendMode new_mode, SendMode old_mode = MODE_NOT_SET);

  /// Access the current sending mode.
  SendMode mode() const;

  bool is_sending(const GUID_t& guid) const
  {
    GuardType g(is_sending_lock_);
    return is_sending_ == guid;
  }

 protected:
  /// Implement framework chain visitations to remove a sample.
  virtual RemoveResult do_remove_sample(const GUID_t& pub_id,
    const TransportQueueElement::MatchCriteria& criteria, bool remove_all = false);

private:

  virtual bool marshal_transport_header(ACE_Message_Block* mb);

  /// Helper function to debugging.
  static const char* mode_as_str(SendMode mode);

  /// Configuration - max number of samples per transport packet
  size_t max_samples_;

  /// Configuration - optimum transport packet size (bytes)
  ACE_UINT32 optimum_size_;

  /// Configuration - max transport packet size (bytes)
  ACE_UINT32 max_size_;

  /// Used during backpressure situations to hold samples that have
  /// not yet been made to be part of a transport packet, and are
  /// completely unsent.
  /// Also used as a bucket for packets which still have to become
  /// part of a packet.
  QueueType queue_;

  /// Maximum marshalled size of the transport packet header.
  size_t max_header_size_;

  /// Current transport packet header, marshalled.
  ACE_Message_Block* header_block_;

  /// Current transport header sequence number.
  SequenceNumber header_sequence_;

  /// Current elements that have contributed blocks to the current
  /// transport packet.
  QueueType elems_;

  /// Current (head of chain) block containing unsent bytes for the
  /// current transport packet.
  ACE_Message_Block* pkt_chain_;

  /// Set to false when the packet header hasn't been fully sent.
  /// Set to true once the packet header has been fully sent.
  bool header_complete_;

  /// Counter that, when greater than zero, indicates that we still
  /// expect to receive a send_stop() event.
  /// Incremented once for each call to our send_start() method,
  /// and decremented once for each call to our send_stop() method.
  /// We only care about the transitions of the start_counter_
  /// value from 0 to 1, and from 1 to 0.  This accommodates the
  /// case where more than one TransportClient is sending to
  /// us at the same time.  We use this counter to enable a
  /// "composite" send_start() and send_stop().
  unsigned start_counter_;

  /// This mode determines how send() calls will be handled.
  Atomic<SendMode> mode_;

  /// This mode remembers the mode before send is suspended and is
  /// used after the send is resumed because the connection is
  /// re-established.
  SendMode mode_before_suspend_;

  /// Used for delayed notifications when performing work.
  typedef std::pair<TransportQueueElement*, SendMode> TQESendModePair;
  OPENDDS_VECTOR(TQESendModePair) delayed_delivered_notification_queue_;

  /// Allocator for header data block.
  unique_ptr<TransportMessageBlockAllocator> header_mb_allocator_;

  /// Allocator for header message block.
  unique_ptr<TransportDataBlockAllocator> header_db_allocator_;

  /// DataBlockLockPool
  unique_ptr<DataBlockLockPool> header_db_lock_pool_;

  /// Allocator for data buffers.
  typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> DataAllocator;
  unique_ptr<DataAllocator> header_data_allocator_;

  /// The thread synch object.
  unique_ptr<ThreadSynch> synch_;

  /// This lock will protect critical sections of code that play a
  /// role in the sending of data.
  LockType lock_;

  /// Cached allocator for TransportReplaceElement.
  MessageBlockAllocator replaced_element_mb_allocator_;
  DataBlockAllocator replaced_element_db_allocator_;

  WeakRcHandle<TransportImpl> transport_;

  bool graceful_disconnecting_;

  bool link_released_;

  TransportSendBuffer* send_buffer_;

  // N.B. The behavior present in TransortSendBuffer should be
  // refactored into the TransportSendStrategy eventually; a good
  // amount of private state is shared between both classes.
  friend class TransportSendBuffer;

  /// Current transport packet header.
  TransportHeader header_;

  struct BeginEndSend {
    BeginEndSend(TransportSendStrategy& tss,
                 const GUID_t& guid)
      : tss_(tss)
    {
      GuardType g(tss_.is_sending_lock_);
      tss_.is_sending_ = guid;
    }

    ~BeginEndSend()
    {
      GuardType g(tss_.is_sending_lock_);
      tss_.is_sending_ = GUID_UNKNOWN;
    }

    TransportSendStrategy& tss_;
  };
  mutable LockType is_sending_lock_;
  GUID_t is_sending_;

protected:
  ThreadSynch* synch() const;

  void set_header_source(ACE_INT64 source);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTSENDSTRATEGY_H */
