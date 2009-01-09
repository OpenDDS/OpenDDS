// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTSENDSTRATEGY_H
#define OPENDDS_DCPS_TRANSPORTSENDSTRATEGY_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "ThreadSynchWorker.h"
#include "TransportDefs.h"
#include "BasicQueue_T.h"
#include "TransportHeader.h"
#include "TransportReplacedElement.h"
#include "TransportConfiguration_rch.h"

#include "ace/Synch.h"

namespace OpenDDS
{

  namespace DCPS
  {

    class ThreadSynch;
    class ThreadSynchResource;
    class TransportQueueElement;
    struct DataSampleListElement;
    class QueueRemoveVisitor;
    class PacketRemoveVisitor;

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
      : public RcObject<ACE_SYNCH_MUTEX>,
        public ThreadSynchWorker
    {
      public:

        virtual ~TransportSendStrategy();

        /// Start the TransportSendStrategy.  This happens once, when
        /// the DataLink that "owns" this strategy object has established
        /// a connection.
        int start();

        /// Stop the TransportSendStrategy.  This happens once, when the
        /// DataLink that "owns" this strategy object is going away.
        void stop();

        /// Invoked prior to one or more send() invocations from a particular
        /// TransportInterface.
        void send_start();

        /// Our DataLink has been requested by some particular
        /// TransportInterface to send the element.
        void send(TransportQueueElement* element, bool relink = true);

        /// Invoked after one or more send() invocations from a particular
        /// TransportInterface.
        void send_stop();

        /// Our DataLink has been requested by some particular
        /// TransportInterface to remove the supplied sample
        /// (basically, an "unsend" attempt) from this strategy object.
        /// A -1 is returned if some fatal error was encountered while
        /// attempting to remove the sample.  Otherwise, a 0 is returned.
        /// Note that a 0 return value does not imply that the sample was
        /// actually found and removed - it could mean that, or it could
        /// mean that the sample has already been fully sent, and there
        /// is no trace of it in this strategy object.
        int remove_sample(const DataSampleListElement* sample);

        void remove_all_control_msgs(RepoId pub_id);

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
        virtual void relink (bool do_suspend = true);

        /// This is called when first time reconnect is attempted. The send mode
        /// is set to MODE_SUSPEND. Messages are queued at this state.
        void suspend_send ();

        /// This is called when connection is lost and reconnect succeeds.
        /// The send mode is set to the mode before suspend which is either MODE_QUEUE
        /// or MODE_DIRECT.
        void resume_send ();

        /// This is called whenver the connection is lost and reconnect fails.
        /// It removes all samples in the backpressure queue and packet queue.
        void terminate_send (bool graceful_disconnecting = false);

      // Moved clear() declaration below as Enums can't be foward declared.

        /// Let the subclass stop.
        virtual void stop_i() = 0;

        void link_released (bool flag);

        bool isDirectMode();

      protected:

        TransportSendStrategy(TransportConfiguration* config,
                              ThreadSynchResource*    synch_resource,
                              CORBA::Long             priority);

        // Only our subclass knows how to do this.
        // Third arg is the "back-pressure" flag.  If send_bytes() returns
        // -1 and the bp == 1, then it isn't really an error - it is
        // backpressure.
        virtual ssize_t send_bytes(const iovec iov[], int n, int& bp) = 0;

        virtual ssize_t non_blocking_send (const iovec iov[], int n, int& bp);

        virtual ACE_HANDLE get_handle () = 0;
        virtual ssize_t send_bytes_i (const iovec iov[], int n) = 0;

//MJM: Hmmm...  Shouldn't we just return success with a backpreassure
//MJM: flag to be checked?  This means that the bp needs to be checked
//MJM: each time on success, instead of checking the bp flag only on
//MJM: failure.
//MJM: Oh.  Nevermind.

      private:

        enum SendPacketOutcome
        {
          OUTCOME_COMPLETE_SEND,
          OUTCOME_PARTIAL_SEND,
          OUTCOME_BACKPRESSURE,
          OUTCOME_PEER_LOST,
          OUTCOME_SEND_ERROR
        };

        enum UseDelayedNotification
        {
          NOTIFY_IMMEADIATELY,
          DELAY_NOTIFICATION
        };

      int remove_sample_i (QueueRemoveVisitor& simple_rem_vis,
         PacketRemoveVisitor& pac_rem_vis);

        /// Called from send() when it is time to attempt to send our
        /// current packet to the socket while in MODE_DIRECT mode_.
        /// If backpressure occurs, our current packet will be adjusted
        /// to account for bytes that were sent, and the mode will be
        /// changed to MODE_QUEUE.
        /// If no backpressure occurs (ie, the entire packet is sent), then
        /// our current packet will be "reset" to be an empty packet following
        /// the send.
        void direct_send( bool relink );

        /// This method is used while in MODE_QUEUE mode, and a new packet
        /// needs to be formulated using elements from the queue_.  This is
        /// the first step of formulating the new packet.  It will extract
        /// elements from the queue_ and insert those elements into the
        /// pkt_elems_ collection.
        ///
        /// After this step has been done, the prepare_packet() step can
        /// be performed, followed by the actual send_packet() call.
        void get_packet_elems_from_queue();

        /// This method is responsible for actually "creating" the current
        /// send packet using the packet header and the collection of
        /// packet elements that are to make-up the packet's contents.
        void prepare_packet();

        /// This is called to send the current packet.  The current packet
        /// will either be a "partially sent" packet, or a packet that has
        /// just been prepared via a call to prepare_packet().
        SendPacketOutcome send_packet(UseDelayedNotification delay_notification);

        /// This is called from the send_packet() method after it has
        /// sent at least one byte from the current packet.  This method
        /// will update the current packet appropriately, as well as deal
        /// with all of the release()'ing of fully sent ACE_Message_Blocks,
        /// and the data_delivered() calls on the fully sent elements.
        /// Returns 0 if the entire packet was sent, and returns 1 if
        /// the entire packet was not sent.
        int adjust_packet_after_send(ssize_t num_bytes_sent,
                                     UseDelayedNotification delay_notification);

        /// This is called by perform_work() after it has sent
        void send_delayed_notifications();

        typedef BasicQueue<TransportQueueElement> QueueType;

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        enum SendMode{
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

    public:
      /// Clear queued messages and messages in current packet.
      // The API now has a defaulted mode (the default is the same as
      // as the earlier hard-coded value).
      // Clear locks the local mutex. In certain situations its
      // important to set the new mode in the clear itself.
      // Since the default is the earlier hard-coded value, this
      // shouldn't have any impact.
      void clear (SendMode mode = MODE_DIRECT);

    private:

        /// Helper function to debugging.
        static const char* mode_as_str (SendMode mode);

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
        QueueType* queue_;

      /// This queue holds elements which haven't yet become part of
      /// a packet.
      //QueueType* not_yet_pac_q_;
      //size_t not_yet_pac_q_len_;

        /// Current transport packet header.
        TransportHeader header_;

        /// Maximum marshalled size of the transport packet header.
        size_t max_header_size_;

        /// Current transport packet header, marshalled.
        ACE_Message_Block* header_block_;
//MJM: Why not hold this as a member rather than a reference?  That is
//MJM: have a message/data/buffer complex holding a header that can be
//MJM: remarshaled for each packet sent.  That way there will be no
//MJM: allocations required.  You may need to mark it as being on the
//MJM: stack, even though it may not be, in order to not worry if it
//MJM: participates in the normal memory management regimine.

        /// Current elements that have contributed blocks to the current
        /// transport packet.
        QueueType* elems_;

        /// Current (head of chain) block containing unsent bytes for the
        /// current transport packet.
        ACE_Message_Block* pkt_chain_;

        /// Set to 0 (false) when the packet header hasn't been fully sent.
        /// Set to 1 (true) once the packet header has been fully sent.
        int header_complete_;

        /// Counter that, when greater than zero, indicates that we still
        /// expect to receive a send_stop() event.
        /// Incremented once for each call to our send_start() method,
        /// and decremented once for each call to our send_stop() method.
        /// We only care about the transitions of the start_counter_
        /// value from 0 to 1, and from 1 to 0.  This accomodates the
        /// case where more than one TransportInterface is sending to
        /// us at the same time.  We use this counter to enable a
        /// "composite" send_start() and send_stop().
        unsigned start_counter_;
//MJM: Um.  I am not sure that we want to allow the packets from
//MJM: different interfaces to interleave.  I may need to think about
//MJM: this for a bit.
//MJM: Nevermind.  Just thought about it some.  The transport packets
//MJM: are not special at the higher layersa and so there does not need
//MJM: to be any restriction.

        /// This mode determines how send() calls will be handled.
        SendMode mode_;

        /// This mode remembers the mode before send is suspended and is
        /// used after the send is resumed because the connection is
        /// re-established.
        SendMode mode_before_suspend_;

        /// Used for delayed notifications when performing work.
        TransportQueueElement** delayed_delivered_notification_queue_;
        SendMode* delayed_notification_mode_;
        size_t num_delayed_notifications_;

        /// Allocator for header data block.
        TransportMessageBlockAllocator header_mb_allocator_;

        /// Allocator for header message block.
        TransportDataBlockAllocator header_db_allocator_;

        /// The thread synch object.
        ThreadSynch* synch_;

        /// This lock will protect critical sections of code that play a
        /// role in the sending of data.
        LockType lock_;

        /// Cached allocator for TransportReplaceElement.
        TransportReplacedElementAllocator replaced_element_allocator_;

        TransportConfiguration_rch config_;

        bool graceful_disconnecting_;

        bool link_released_;

      //remove these are only for debugging: DUMP_FOR_PACKET_INFO
        protected:
    ACE_Message_Block*    dup_pkt_chain;
    ACE_Message_Block*    act_pkt_chain_ptr;
    void* act_elems_head_ptr;
    ACE_Message_Block*    dup_elems_msg;
    ACE_Message_Block*    act_elems_msg_ptr;
    TransportHeader       dup_presend_header;
    const char * called_from;
    const char * completely_filled;

    };

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "TransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTSENDSTRATEGY_H */
