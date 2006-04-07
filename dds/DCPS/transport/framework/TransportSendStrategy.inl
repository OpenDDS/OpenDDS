// -*- C++ -*-
//
// $Id$
#include  "TransportConfiguration.h"
#include  "ThreadSynchStrategy.h"
#include  "ThreadSynch.h"
#include  "ThreadSynchResource.h"
#include  "TransportQueueElement.h"
#include  "BuildChainVisitor.h"
#include  "QueueRemoveVisitor.h"
#include  "PacketRemoveVisitor.h"
#include  "TransportDefs.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "EntryExit.h"


ACE_INLINE int
TAO::DCPS::TransportSendStrategy::start()
{
  DBG_ENTRY("TransportSendStrategy","start");

  GuardType guard(this->lock_);

  // Since we (the TransportSendStrategy object) are a reference-counted
  // object, but the synch_ object doesn't necessarily know this, we need
  // to give a "copy" of a reference to ourselves to the synch_ object here.
  // We will do the reverse when we unregister ourselves (as a worker) from
  // the synch_ object.
//MJM: The synch thingie knows to not "delete" us, right?
  this->_add_ref();
  if (this->synch_->register_worker(this) == -1)
    {
      // Take back our "copy".
      this->_remove_ref();
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportSendStrategy failed to register "
                        "as a worker with the ThreadSynch object.\n"),
                       -1);
    }

  return 0;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::stop()
{
  DBG_ENTRY("TransportSendStrategy","stop");

  GuardType guard(this->lock_);
//MJM: Why are we guarding this?  The refcount decrement is already
//MJM: guarded.  Is unregister guarded, or does it need to be?

  this->synch_->unregister_worker();

  // Since we gave the synch_ a "copy" of a reference to ourselves, we need
  // to take it back now.
  this->_remove_ref();

  // TBD SOON - What about all of the samples that may still be stuck in
  //            our queue_ and/or elems_?
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::send_start()
{
  DBG_ENTRY("TransportSendStrategy","send_start");

  GuardType guard(this->lock_);
  ++this->start_counter_;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::send(TransportQueueElement* element)
{
  DBG_ENTRY("TransportSendStrategy","send");

  GuardType guard(this->lock_);

  size_t element_length = element->msg()->total_length();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Send element msg() has total_length() == [%d].\n",
             element_length));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "this->max_header_size_ == [%d].\n",
             this->max_header_size_));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "this->max_size_ == [%d].\n",
             this->max_size_));

//MJM: Make this an ASSERT so that it gets removed for performance runs.

  // Really an assert.  We can't accept any element that wouldn't fit into
  // a transport packet by itself (ie, it would be the only element in the
  // packet).
  if (this->max_header_size_ + element_length > this->max_size_)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Element too large - won't fit into packet.\n"));
      return;
    }

  // Check the mode_ to see if we simply put the element on the queue.
  if (this->mode_ == MODE_QUEUE)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "this->mode_ == MODE_QUEUE, so queue elem and leave.\n"));

      this->queue_.put(element);
      this->synch_->work_available();
      return;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "this->mode_ == MODE_DIRECT.\n"));

  // We are in the MODE_DIRECT send mode.  When in this mode, the send()
  // calls will "build up" the transport packet to be sent directly when it
  // reaches the optimal size, contains the maximum number of samples, etc.

  // We need to check if the current element (the arg passed-in to this
  // send() method) should be appended to the transport packet, or if the
  // transport packet should be sent (directly) first, dealing with the
  // current element afterwards.

  // We will decide to send the packet as it is now, under two circumstances:
  //
  //    Either:
  //
  //    (1) The current element won't fit into the current packet since it
  //        would violate the max_packet_size_.
  //
  //    -OR-
  //
  //    (2) There is at least one element already in the current packet,
  //        and the current element says that it must be sent in an
  //        exclusive packet (ie, in a packet all by itself).
  //
  int element_requires_exclusive_packet = element->requires_exclusive_packet();

//MJM: This entire conditional needs to be eliminated for performance
//MJM: runs. Conditional compilation section maybe?
  if (element_requires_exclusive_packet)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "The element DOES require an exclusive packet.\n"));
    }
  else
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "The element does NOT require an exclusive packet.\n"));
    }

  if ((this->max_header_size_ +
       this->header_.length_  +
       element_length           > this->max_size_) ||
      ((this->elems_.size() != 0) && (element_requires_exclusive_packet == 1)))
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Element won't fit in current packet - send current "
                 "packet (directly) now.\n"));

      // Send the current packet, and deal with the current element
      // afterwards.
      this->direct_send();

      // Now check to see if we flipped into MODE_QUEUE, which would mean
      // that the direct_send() experienced backpressure, and the
      // packet was only partially sent.  If this has happened, we deal with
      // the current element by placing it on the queue (and then we are done).
      //
      // Otherwise, if the mode_ is still MODE_DIRECT, we can just
      // "drop" through to the next step in the logic where we append the
      // current element to the current packet.
//MJM: But we don't want to append an exclusive thingie to a packet (or
//MJM: a thingie to an exclusive packet either), right?
      if (this->mode_ == MODE_QUEUE)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "We experienced backpressure on that direct send, as "
                     "the mode_ is now MODE_QUEUE.  Queue elem and leave.\n"));
          this->queue_.put(element);
          this->synch_->work_available();
          return;
        }
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Start the 'append elem' to current packet logic.\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Put element into current packet elems_.\n"));

  // Now that we know the current element should go into the current
  // packet, we can just go ahead and "append" the current element to
  // the current packet.

  // Add the current element to the collection of packet elements.
  this->elems_.put(element);
//MJM: I am not sure that this works when either the previous element or
//MJM: the current (new) element is a exclusive one.  This would only
//MJM: happen if the previous packet (if any) was not completely sent
//MJM: just prior to this spot.

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Before, the header_.length_ == [%d].\n",
             this->header_.length_));

  // Adjust the header_.length_ to account for the length of the element.
  this->header_.length_ += element_length;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "After adding element's length, the header_.length_ == [%d].\n",
             this->header_.length_));

  // The current packet now contains the current element.  We need to
  // check to see if the conditions are such that we should go ahead and
  // attempt to send the packet "directly" now, or if we can just leave
  // and send the current packet later (in another send() call or in a
  // send_stop() call).

  // There are three conditions that will cause us to attempt to send the
  // packet (directly) right now.
  //
  //   (1) The current packet has the maximum number of samples per packet.
  //   (2) The current packet's total length exceeds the optimum packet size.
  //   (3) The current element (currently part of the packet elems_)
  //       requires an exclusive packet.
  //
//MJM: Should probably check >= max_samples_ here.  Belts/suspenders thing.
  if ((this->elems_.size() == this->max_samples_)                            ||
      (this->max_header_size_ + this->header_.length_ > this->optimum_size_) ||
      (element_requires_exclusive_packet == 1))
//MJM: I think that I need to think about the exclusive cases here.
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Now the current packet looks full - send it (directly).\n"));
      this->direct_send();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Back from the direct_send() attempt.\n"));
//MJM: This following conditional needs to be lost for performance runs
//MJM: as well.
      if (this->mode_ == MODE_QUEUE)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "And we flipped into MODE_QUEUE as a result of the "
                     "direct_send() call.\n"));
        }
      else
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "And we stayed in the MODE_DIRECT as a result of the "
                     "direct_send() call.\n"));
        }
    }
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::send_stop()
{
  DBG_ENTRY("TransportSendStrategy","send_stop");

  GuardType guard(this->lock_);

//MJM: Make it an assert for the performance runs to lose it.
  if (this->start_counter_ == 0)
    {
      // This is an indication of a logic error.  This is more of an assert.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Received unexpected send_stop() call.\n"));
      return;
    }

  --this->start_counter_;

  if (this->start_counter_ != 0)
    {
      // This wasn't the last send_stop() that we are expecting.  We only
      // really honor the first send_start() and the last send_stop().
      // We can return without doing anything else in this case.
      return;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "This is an 'important' send_stop() event since our "
             "start_counter_ is 0.\n"));

  // We just caused the start_counter_ to become zero.  This
  // means that we aren't expecting another send() or send_stop() at any
  // time in the near future (ie, it isn't imminent).
//MJM: It means that the publisher(s) have indicated a desire to push
//MJM: all the data just sent out to the remote ends.

  // If our mode_ is currently MODE_QUEUE, then we don't have
  // anything to do here because samples have already been going to the
  // queue.  We only need to do something if the mode_ is
  // MODE_DIRECT.  It means that we may have some sample(s) in the
  // current packet that have never been sent.  This is our
  // opportunity to send the current packet directly if this is the case.
  if (this->mode_ == MODE_QUEUE)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "But since we are in MODE_QUEUE, we don't have to do "
                 "anything more in this important send_stop().\n"));
      // We don't do anything if we are in MODE_QUEUE.  Just leave.
      return;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "We are in MODE_DIRECT in an important send_stop() - "
             "header_.length_ == [%d].\n", this->header_.length_));

  // Only attempt to send the current packet (directly) if the current
  // packet actually contains something (it could be empty).
  if (this->header_.length_ > 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "There something in the current packet - attempt to send "
                 "it (directly) now.\n"));
      this->direct_send();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Back from the attempt to send leftover packet directly.\n"));
//MJM: Another conditionally lost conditional, ay.
      if (this->mode_ == MODE_QUEUE)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "But we flipped into MODE_QUEUE as a result.\n"));
        }
      else
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "And we stayed in the MODE_DIRECT.\n"));
        }
    }
}


ACE_INLINE int
TAO::DCPS::TransportSendStrategy::remove_sample
                                    (const DataSampleListElement* sample)
{
  DBG_ENTRY("TransportSendStrategy","remove_sample");

  GuardType guard(this->lock_);

  QueueRemoveVisitor remove_element_visitor(sample->sample_);

  if (this->mode_ == MODE_DIRECT)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The mode is MODE_DIRECT.\n"));

      this->elems_.accept_remove_visitor(remove_element_visitor);

      int status = remove_element_visitor.status();

      if (status == 1)
        {
          // The sample was found (and removed) by the visitor.
          // Adjust our header_.length_ to account for the removed bytes.
          this->header_.length_ -= remove_element_visitor.removed_bytes();
        }

      // Now we can return -1 if status == -1.  Otherwise, we return 0.
      return (status == -1) ? -1 : 0;
    }

  // We now know that this->mode_ == MODE_QUEUE.
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "The mode is MODE_QUEUE.\n"));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Visit the queue_ with the RemoveElementVisitor.\n"));

  // First we will attempt to remove the element from the queue_,
  // in case it is "stuck" in there.
  this->queue_.accept_remove_visitor(remove_element_visitor);

  int status = remove_element_visitor.status();

  if (status == 1)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The sample was removed from the queue_.\n"));
      // This means that the visitor did not encounter any fatal error
      // along the way, *AND* the sample was found in the queue_,
      // and has now been removed.  We are done.
      return 0;
    }

  if (status == -1)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The RemoveElementVisitor encountered a fatal error.\n"));
      // This means that the visitor encountered some fatal error along
      // the way (and it already reported something to the log).
      // Return our failure code.
      return -1;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "The RemoveElementVisitor did not find the sample in queue_.\n"));

  // We get here if the visitor did not encounter any fatal error, but it
  // also didn't find the sample - and hence it didn't perform any
  // "remove sample" logic.

  // Now we need to turn our attention to the current transport packet,
  // since the packet is likely in a "partially sent" state, and the
  // sample may still be contributing unsent bytes in the pkt_chain_.

  // Construct a PacketRemoveVisitor object.
  PacketRemoveVisitor remove_from_packet_visitor(sample->sample_,
                                                 this->pkt_chain_,
                                                 this->header_block_,
                                                 this->replaced_element_allocator_);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Visit our elems_ with the PacketRemoveVisitor.\n"));

  // Let it visit our elems_ collection as a "replace" visitor.
  this->elems_.accept_replace_visitor(remove_from_packet_visitor);

  status = remove_from_packet_visitor.status();

  if (status == -1)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The PacketRemoveVisitor encountered a fatal error.\n"));
    }
  else if (status == 1)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The PacketRemoveVisitor found the sample and removed it.\n"));
    }
  else
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The PacketRemoveVisitor didn't find the sample.\n"));
    }

  // Return -1 only if the visitor's status() returns -1. Otherwise, return 0.
  return (status == -1) ? -1 : 0;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY("TransportSendStrategy","remove_all_control_msgs");

  GuardType guard(this->lock_);

  QueueRemoveVisitor remove_element_visitor(pub_id);

  if (this->mode_ == MODE_DIRECT)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The mode is MODE_DIRECT.\n"));

      this->elems_.accept_remove_visitor(remove_element_visitor);

      int status = remove_element_visitor.status();

      if (status == 1)
        {
          // Adjust our header_.length_ to account for the removed bytes.
          this->header_.length_ -= remove_element_visitor.removed_bytes();
        }

      // Now we can return.
      return;
    }

  // We now know that this->mode_ == MODE_QUEUE.
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "The mode is MODE_QUEUE.\n"));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Visit the queue_ with the RemoveElementVisitor.\n"));

  // First we will attempt to remove the element from the queue_,
  // in case it is "stuck" in there.
  this->queue_.accept_remove_visitor(remove_element_visitor);

  int status = remove_element_visitor.status();

  if (status == -1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: QueueRemoveVisitor encountered fatal error.\n"));
    }

  // Now we need to turn our attention to the current transport packet,
  // since the packet is likely in a "partially sent" state, and there
  // may still be control samples from our pub_id_ that are contributing
  // unsent bytes in the pkt_chain_.

  // Construct a PacketRemoveVisitor object.
  PacketRemoveVisitor remove_from_packet_visitor(pub_id,
                                                 this->pkt_chain_,
                                                 this->header_block_,
                                                 this->replaced_element_allocator_);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Visit our elems_ with the PacketRemoveVisitor.\n"));

  // Let it visit our elems_ collection as a "replace" visitor.
  this->elems_.accept_replace_visitor(remove_from_packet_visitor);

  status = remove_from_packet_visitor.status();

  if (status == -1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: PacketRemoveVisitor encountered fatal error.\n"));
    }
}



ACE_INLINE void
TAO::DCPS::TransportSendStrategy::direct_send()
{
  DBG_ENTRY("TransportSendStrategy","direct_send");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Prepare the current packet for a direct send attempt.\n"));

  // Prepare the packet for sending.
  this->prepare_packet();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Now attempt to send the packet.\n"));

 
  // We will try resend the packet if the send() fails and then connection 
  // is re-established.
  for (CORBA::ULong i = 0; i < 2; ++i)
    {
      // Attempt to send the packet
      SendPacketOutcome outcome = this->send_packet(NOTIFY_IMMEADIATELY);

      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                "The outcome of the send_packet() was %d.\n", outcome));

      if ((outcome == OUTCOME_BACKPRESSURE) ||
          (outcome == OUTCOME_PARTIAL_SEND))
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "The outcome of the send_packet() was either "
                    "OUTCOME_BACKPRESSURE or OUTCOME_PARTIAL_SEND.\n"));

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "Flip into the MODE_QUEUE mode_.\n"));

          // We encountered backpressure, or only sent part of the packet.
          this->mode_ = MODE_QUEUE;
          this->synch_->work_available();
        }
      else if ((outcome == OUTCOME_PEER_LOST) ||
              (outcome == OUTCOME_SEND_ERROR))
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "The outcome of the send_packet() was either "
                    "OUTCOME_PEER_LOST or OUTCOME_SEND_ERROR.\n"));

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "Nothing better to do other than flip to MODE_QUEUE, but "
                    "don't announce work_available() to the synch_ object.\n"));

          if (this->relink () == -1)
            {
              // Either we've lost our connection to the peer (ie, the peer
              // disconnected), or we've encountered some unknown fatal error
              // attempting to send the packet. We tried relink and it failed.
              // We can't do anything other than flip into MODE_QUEUE, and never
              // be able to send anything. The user should stop sending when 
              // receiving the callback.
              this->mode_ = MODE_QUEUE;
            }
          else
            {
              // Try send the packet again if the connection is re-established.
              continue; 
            }
      }
    else
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "The outcome of the send_packet() must have been "
                  "OUTCOME_COMPLETE_SEND.\n"));
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "So, we will just stay in MODE_DIRECT.\n"));
      }
    
    break;
  }

  // We stay in MODE_DIRECT mode if we didn't encounter any backpressure.
  return;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::get_packet_elems_from_queue()
{
  DBG_ENTRY("TransportSendStrategy","get_packet_elems_from_queue");

  TransportQueueElement* element = this->queue_.peek();

  while (element != 0)
    {
      // Total number of bytes in the current element's message block chain.
      size_t element_length = element->msg()->total_length();

      // Flag used to determine if the element requires a packet all to itself.
      int exclusive_packet = element->requires_exclusive_packet();

      // The total size of the current packet (packet header bytes included)
      size_t packet_size = this->max_header_size_ + this->header_.length_;

      if (packet_size + element_length > this->max_size_)
        {
          // The current element won't fit into the current packet.
          break;
        }

      if (exclusive_packet == 1)
        {
          if (this->elems_.size() == 0)
            {
              // The current packet is empty so we won't violate the
              // exclusive_packet requirement by put()'ing the element
              // into the elems_ collection.  We also extract the current
              // element from the queue_ at this time (we've been peek()'ing
              // at the element all this time).
              this->elems_.put(this->queue_.get());
              this->header_.length_ = element_length;
            }

          // Otherwise (when elems_.size() != 0), we don't use the current
          // element as part of the packet.  We know that there is already
          // at least one element in the packet, and the current element
          // is going to need its own (exclusive) packet.  We will just
          // use the packet elems_ as it is now.  Always break once
          // we've encountered and dealt with the exclusive_packet case.
          break;
        }

      // At this point, we have passed all of the pre-conditions and we can
      // now extract the current element from the queue_, put it into the
      // packet elems_, and adjust the packet header_.length_.
      this->elems_.put(this->queue_.get());
      this->header_.length_ += element_length;

      // If the current number of packet elems_ has reached the maximum
      // number of samples per packet, then we are done.
      if (this->elems_.size() == this->max_samples_)
        {
          break;
        }

      // If the current value of the header_.length_ exceeds (or equals)
      // the optimum_size_ for a packet, then we are done.
      if (this->header_.length_ >= this->optimum_size_)
        {
          break;
        }

      // Set up the element to peek() at the front of the queue.
      element = this->queue_.peek();
    }
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::prepare_packet()
{
  DBG_ENTRY("TransportSendStrategy","prepare_packet");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Marshall the packet header.\n"));

  // First make sure that the header_block_ is "reset".
  this->header_block_->rd_ptr(this->header_block_->base());
  this->header_block_->wr_ptr(this->header_block_->base());

  // Marshall the packet header_ into the header_block_
  this->header_block_ << this->header_;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the pkt_chain_ to point to a duplicate of the "
             "(marshalled) packet header block.\n"));

  // Make a duplicate of the header_block_ and make that be the head
  // block in the pkt_chain_.
  this->pkt_chain_ = this->header_block_->duplicate();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use a BuildChainVisitor to visit the packet elems_.\n"));

  // Build up a chain of blocks by duplicating the message block chain
  // held by each element (in elems_), and then chaining the new duplicate
  // blocks together to form one long chain.
  BuildChainVisitor visitor;
  this->elems_.accept_visitor(visitor);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Attach the visitor's chain of blocks to the lone (packet "
             "header) block currently in the pkt_chain_.\n"));

  // Attach the visitor's chain of blocks to the packet header block.
  this->pkt_chain_->cont(visitor.chain());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the header_complete_ flag to false (0).\n"));

  // Don't forget to set the header_complete_ to false (0) to indicate
  // that the first block in the pkt_chain_ is the packet header block
  // (actually a duplicate() of the packet header_block_).
  this->header_complete_ = 0;
}


ACE_INLINE TAO::DCPS::TransportSendStrategy::SendPacketOutcome
TAO::DCPS::TransportSendStrategy::send_packet(UseDelayedNotification delay_notification)
{
  DBG_ENTRY("TransportSendStrategy","send_packet");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Populate the iovec array using the pkt_chain_.\n"));

  iovec iov[MAX_SEND_BLOCKS];

  ACE_Message_Block* block = this->pkt_chain_;

  int num_blocks = 0;

  while (block != 0 && num_blocks < MAX_SEND_BLOCKS)
    {
      iov[num_blocks].iov_len  = block->length();
      iov[num_blocks].iov_base = block->rd_ptr();
      ++num_blocks;
      block = block->cont();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "There are [%d] number of entries in the iovec array.\n",
             num_blocks));

  // Get our subclass to do this next step, since it is the one that knows
  // how to really do this part.
  int bp_flag = 0;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Attempt to send_bytes() now.\n"));

  ssize_t num_bytes_sent = this->send_bytes(iov, num_blocks, bp_flag);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The send_bytes() said that num_bytes_sent == [%d].\n",
             num_bytes_sent));

  if (num_bytes_sent == 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Since num_bytes_sent == 0, return OUTCOME_PEER_LOST.\n"));
      // This means that the peer has disconnected.
      return OUTCOME_PEER_LOST;
    }

  if (num_bytes_sent < 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Since num_bytes_sent < 0, check the backpressure flag.\n"));

      // Check for backpressure...
      if (bp_flag == 1)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Since backpressure flag is true, return "
                     "OUTCOME_BACKPRESSURE.\n"));
          // Ok.  Not really an error - just backpressure.
          return OUTCOME_BACKPRESSURE;
        }

      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Since backpressure flag is false, return "
                 "OUTCOME_SEND_ERROR.\n"));


      // Not backpressure - it's a real error.
      // Note: moved thisto send_bytes so the errno msg could be written.
      //ACE_ERROR((LM_ERROR,
      //           "(%P|%t) ERROR: Call to peer().send() failed with negative "
      //           "return code.\n"));

      return OUTCOME_SEND_ERROR;
    }


  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Since num_bytes_sent > 0, adjust the packet to account for "
             "the bytes that did get sent.\n"));

  // We sent some bytes - adjust the current packet (elems_ and pkt_chain_)
  // to account for the bytes that have been sent.
  int result = this->adjust_packet_after_send(num_bytes_sent, delay_notification);

  if (result == 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "The adjustment logic says that the complete packet was "
                 "sent.  Return OUTCOME_COMPLETE_SEND.\n"));
      return OUTCOME_COMPLETE_SEND;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The adjustment logic says that only a part of the packet was "
             "sent. Return OUTCOME_PARTIAL_SEND.\n"));

  return OUTCOME_PARTIAL_SEND;
}


ACE_INLINE int 
TAO::DCPS::TransportSendStrategy::relink ()
{
  // The subsclass needs implement this function for re-establishing
  // the link upon send failure.
  return -1;
}

