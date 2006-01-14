// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportSendStrategy.h"


#if !defined (__ACE_INLINE__)
#include "TransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

// I think 2 chunks for the header message block is enough
// - one for the original copy and one for duplicate which 
// occurs every packet and is released after packet is sent.
// The data block only needs 1 chunk since the duplicate()
// just increases the ref count.
TAO::DCPS::TransportSendStrategy::TransportSendStrategy
                                     (TransportConfiguration* config,
                                      ThreadSynchResource*    synch_resource)
  : max_samples_(config->max_samples_per_packet_),
    optimum_size_(config->optimum_packet_size_),
    max_size_(config->max_packet_size_),
    queue_(config->queue_links_per_pool_,config->queue_initial_pools_),
    elems_(1,config->max_samples_per_packet_),
    pkt_chain_(0),
    header_complete_(0),
    start_counter_(0),
    mode_(MODE_DIRECT),
    num_delayed_notifications_(0),
    header_db_allocator_(1),
    header_mb_allocator_(2)
{
  DBG_ENTRY("TransportSendStrategy","TransportSendStrategy");

  // Create a ThreadSynch object just for us.
  this->synch_ = config->send_thread_strategy()->create_synch_object
                                                         (synch_resource);

  // We cache this value in data member since it doesn't change, and we
  // don't want to keep asking for it over and over.
  this->max_header_size_ = this->header_.max_marshaled_size();

  // Create the header_block_ that is used to hold the marshalled
  // transport packet header bytes.
  ACE_NEW_MALLOC (this->header_block_,
                  (ACE_Message_Block*)header_mb_allocator_.malloc (),
                  ACE_Message_Block(this->max_header_size_,
                  ACE_Message_Block::MB_DATA,
                  0,
                  0,
                  0,
                  0,
                  ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                  ACE_Time_Value::zero,
                  ACE_Time_Value::max_time,
                  &header_db_allocator_,
                  &header_mb_allocator_));


  this->delayed_delivered_notification_queue_ = new TransportQueueElement* [max_samples_];
}


TAO::DCPS::TransportSendStrategy::~TransportSendStrategy()
{
  DBG_ENTRY("TransportSendStrategy","~TransportSendStrategy");

  // We created the header_block_ in our ctor, so we should release() it.
//MJM: blech.
  if (this->header_block_)
    {
      this->header_block_->release();
    }

  if (this->synch_)
    {
      delete this->synch_;
//MJM: Or should this be release to be more general?  To let the synch
//MJM: thingie manage itself the way it sees fit.
    }

    delete [] this->delayed_delivered_notification_queue_;

}


TAO::DCPS::TransportSendStrategy::WorkOutcome
TAO::DCPS::TransportSendStrategy::perform_work()
{
  DBG_ENTRY("TransportSendStrategy","perform_work");

  SendPacketOutcome outcome;

  { // scope for the guard(this->lock_);
    GuardType guard(this->lock_);

    // The perform_work() is called by our synch_ object using
    // a thread designated to call this method when it thinks
    // we need to be called in order to "service" the queue_ and/or
    // deal with a partially sent current packet.
    //
    // We will return a 0 if we don't see a need to have our perform_work()
    // called again, and we will return a 1 if we do see the need to have our
    // perform_work() method called again.

    // First, make sure that the mode_ indicates that we are, indeed, in
    // the MODE_QUEUE mode.  If we are not in MODE_QUEUE mode (meaning we are
    // in MODE_DIRECT), then it means we didn't need to have this perform_work()
    // method called - in this case, do nothing other than return
    // WORK_OUTCOME_NO_MORE_TO_DO to tell our caller that we really don't
    // see a need for it to call our perform_work() again (at least not
    // right now).
    if (this->mode_ != MODE_QUEUE)
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Entered perform_work() and mode_ is not MODE_QUEUE - "
                  "just return WORK_OUTCOME_NO_MORE_TO_DO.\n"));
        return WORK_OUTCOME_NO_MORE_TO_DO;
      }

    // Check the "state" of the current packet.  We will either find that the
    // current packet is in a state of being "partially sent", or we will find
    // it in a state of being "empty".  When the current packet is "empty", it
    // means that it is time to build up the current packet using elements
    // extracted from the queue_, and then we will attempt to send the
    // packet.  When we find the current packet in the "partially sent" state,
    // we will not touch the queue_ - we will just try to send the unsent
    // bytes in the current (partially sent) packet.
    if (this->header_.length_ == 0)
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "The current packet doesn't have any unsent bytes - we "
                  "need to 'populate' the current packet with elems from "
                  "the queue.\n"));

        // The current packet is "empty".  Build up the current packet using
        // elements from the queue_, and prepare the current packet to be sent.

        // Before we build the packet from the queue_, let's make sure that
        // there is actually something on the queue_ to build from.
        if (this->queue_.size() == 0)
          {
            VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                      "But the queue is empty.  We have cleared the "
                      "backpressure situation.\n"));
            // We are here because the queue_ is empty, and there isn't
            // any "partial packet" bytes left to send.  We have overcome
            // the backpressure situation and don't have anything to do
            // right now.

            VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                      "Flip mode to MODE_DIRECT, and return "
                      "WORK_OUTCOME_NO_MORE_TO_DO.\n"));

            // Flip the mode back to MODE_DIRECT.
            this->mode_ = MODE_DIRECT;

            // And return WORK_OUTCOME_NO_MORE_TO_DO to tell our caller that
            // perform_work() doesn't need to be called again (at this time).
            return WORK_OUTCOME_NO_MORE_TO_DO;
          }

        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "There is at least one elem in the queue - get the packet "
                  "elems from the queue.\n"));

        // There is stuff in the queue_ if we get to this point in the logic.
        // Build-up the current packet using element(s) from the queue_.
        this->get_packet_elems_from_queue();

        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Prepare the packet from the packet elems_.\n"));

        // Now we can prepare the new packet to be sent.
        this->prepare_packet();

        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Packet has been prepared from packet elems_.\n"));
      }
    else
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "We have a current packet that still has unsent bytes.\n"));
      }

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
              "Attempt to send the current packet.\n"));


    // Now we can attempt to send the current packet - whether it is
    // a "partially sent" packet or one that we just built-up using elements
    // from the queue_ (and subsequently prepared for sending) - it doesn't
    // matter.  Just attempt to send as many of the "unsent" bytes in the
    // packet as possible.
    outcome = this->send_packet(DELAY_NOTIFICATION);

    // If we sent the whole packet (eg, partial_send is false), and the queue_
    // is now empty, then we've cleared the backpressure situation.
    if ((outcome == OUTCOME_COMPLETE_SEND) && (this->queue_.size() == 0))
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Flip the mode to MODE_DIRECT, and then return "
                  "WORK_OUTCOME_NO_MORE_TO_DO.\n"));

        // Revert back to MODE_DIRECT mode.
        this->mode_ = MODE_DIRECT;
      }
  } // End of scope for guard(this->lock_);

  // Notify the Elements that were sent.
  this->send_delayed_notifications();

  // If we sent the whole packet (eg, partial_send is false), and the queue_
  // is now empty, then we've cleared the backpressure situation.
  if ((outcome == OUTCOME_COMPLETE_SEND) && (this->queue_.size() == 0))
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "We sent the whole packet, and there is nothing left on "
                 "the queue now.\n"));

      // Return WORK_OUTCOME_NO_MORE_TO_DO to tell our caller that we
      // don't desire another call to this perform_work() method.
      return WORK_OUTCOME_NO_MORE_TO_DO;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "We still have unsent bytes in the current packet AND/OR there "
             "are still elements in the queue.\n"));

  if ((outcome == OUTCOME_PEER_LOST) || (outcome == OUTCOME_SEND_ERROR))
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "We lost our connection, or had some fatal connection "
                 "error.  Return WORK_OUTCOME_BROKEN_RESOURCE.\n"));

      // Either we've lost our connection to the peer (ie, the peer
      // disconnected), or we've encountered some unknown fatal error
      // attempting to send the packet.  Stay in MODE_QUEUE, but lets
      // return WORK_OUTCOME_BROKEN_RESOURCE.
      return WORK_OUTCOME_BROKEN_RESOURCE;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "We still have an 'unbroken' connection.\n"));

  if (outcome == OUTCOME_BACKPRESSURE)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "We experienced backpressure on our attempt to send the "
                 "packet.  Return WORK_OUTCOME_ClOGGED_RESOURCE.\n"));
      // We have a "clogged resource".
      return WORK_OUTCOME_ClOGGED_RESOURCE;
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "We may have sent the whole current packet, but still have "
             "elements on the queue.\n"));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Or, we may have only partially sent the current packet.\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Either way, we return WORK_OUTCOME_MORE_TO_DO now.\n"));

  // We may have had an OUTCOME_COMPLETE_SEND, but there is still stuff
  // in the queue_ to be sent.  *OR* we have have had an OUTCOME_PARTIAL_SEND,
  // which equates to the same thing - we still have work to do.

  // We are still in MODE_QUEUE mode, thus there is still work to be
  // done to service the queue_ and/or a partially sent current packet.
  // Return WORK_OUTCOME_MORE_TO_DO so that our caller knows that we still
  // want it to call this perform_work() method.
  return WORK_OUTCOME_MORE_TO_DO;
}


// Now we need to "peel off" those message blocks that were fully
// sent, adjust the first message block with an unsent byte to have
// its rd_ptr() pointing to that first unsent byte, and set the
// pkt_chain_ to that first message block with an unsent byte.
// As we "peel off" fully sent message blocks, we need to also deal with
// fully sent elements by removing them from the elems_ and
// calling their data_delivered() method.  In addition, as we peel off
// the message blocks that are fully sent, we need to untie them from
// the chain and release them.
// And finally, don't forget to adjust the header_.length_ to
// account for the num_bytes_sent (beware that some of the num_bytes_sent
// may be packet header bytes and shouldn't affect the header_.length_
// which doesn't include the packet header bytes.
int
TAO::DCPS::TransportSendStrategy::adjust_packet_after_send
                                                   (ssize_t num_bytes_sent,
                                                    UseDelayedNotification delay_notification)
{
  DBG_ENTRY("TransportSendStrategy","adjust_packet_after_send");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Adjusting the current packet because %d bytes of the packet "
             "have been sent.\n", num_bytes_sent));

  ssize_t num_bytes_left = num_bytes_sent;
  ssize_t num_non_header_bytes_sent = 0;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set num_bytes_left to %d.\n", num_bytes_left));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set num_non_header_bytes_sent to %d.\n",
             num_non_header_bytes_sent));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Peek at the element at the front of the packet elems_.\n"));

  // This is the element currently at the front of elems_.
  TransportQueueElement* element = this->elems_.peek();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use the element's msg() to find the last block in "
             "the msg() chain.\n"));

  // Get a pointer to the last message block in the element.
  const ACE_Message_Block* elem_tail_block = element->msg();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Start with tail block == element->msg().\n"));

  while (elem_tail_block->cont() != 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Set tail block to its cont() block (next in chain).\n"));
      elem_tail_block = elem_tail_block->cont();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Tail block now set (because tail block's cont() is 0).\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Start the 'while (num_bytes_left > 0)' loop.\n"));

  while (num_bytes_left > 0)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "At top of 'num bytes left' loop.  num_bytes_left == [%d].\n",
                 num_bytes_left));

      int block_length = this->pkt_chain_->length();

      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "Length of block at front of pkt_chain_ is [%d].\n",
                 block_length));

      if (block_length <= num_bytes_left)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "The whole block at the front of pkt_chain_ was sent.\n"));

          // The entire message block at the front of the chain has been sent.
          // Detach the head message block from the chain and adjust
          // the pkt_chain_ to point to the next block (if any) in
          // the chain.
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Extract the fully sent block from the pkt_chain_.\n"));

          ACE_Message_Block* fully_sent_block = this->pkt_chain_;

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Set pkt_chain_ to pkt_chain_->cont().\n"));

          this->pkt_chain_ = this->pkt_chain_->cont();

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Set the fully sent block's cont() to 0.\n"));

          fully_sent_block->cont(0);

          // Update the num_bytes_left to indicate that we have
          // processed the entire length of the block.
          num_bytes_left -= block_length;

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Updated num_bytes_left to account for fully sent "
                     "block (block_length == [%d]).\n", block_length));
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Now, num_bytes_left == [%d].\n", num_bytes_left));

          if (this->header_complete_ == 0)
            {
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Since the header_complete_ flag is false, it means "
                         "that the packet header block was still in the "
                         "pkt_chain_.\n"));

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Not anymore...  Set the header_complete_ flag "
                         "to true.\n"));

              // That was the packet header block.  And now we know that it
              // has been completely sent.
              this->header_complete_ = 1;

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Release the fully sent block.\n"));

              // Release the fully_sent_block
              fully_sent_block->release();
            }
          else
            {
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Since the header_complete_ flag is true, it means "
                         "that the packet header block was not in the "
                         "pkt_chain_.\n"));
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "So, the fully sent block was part of an element.\n"));

              // That wasn't the packet header block.  It was from the
              // element currently at the front of the elems_
              // collection.  If it was the last block from the
              // element, then we need to extract the element from the
              // elems_ collection and invoke data_delivered() on it.
              num_non_header_bytes_sent += block_length;

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Updated num_non_header_bytes_sent to account for "
                         "fully sent block (block_length == [%d]).\n",
                         block_length));

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Now, num_non_header_bytes_sent == [%d].\n",
                         num_non_header_bytes_sent));

              if (fully_sent_block->base() == elem_tail_block->base())
                {
                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Ok.  The fully sent block was a duplicate of "
                             "the tail block of the element that is at the "
                             "front of the packet elems_.\n"));

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "This means that we have completely sent the "
                             "element at the front of the packet elems_.\n"));

                  // This means that we have completely sent the element
                  // that is currently at the front of the elems_ collection.

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "We can release the fully sent block now.\n"));

                  // Release the fully_sent_block
                  fully_sent_block->release();

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "We can extract the element from the front of "
                             "the packet elems_ (we were just peeking).\n"));

                  // Extract the element from the elems_ collection
                  element = this->elems_.get();

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Tell the element that a decision has been made "
                             "regarding its fate - data_delivered().\n"));

                  // Inform the element that the data has been delivered.
                  if (delay_notification == NOTIFY_IMMEADIATELY)
                    {
                      element->data_delivered();
                    }
                  else if (delay_notification == DELAY_NOTIFICATION)
                    {
                      delayed_delivered_notification_queue_[num_delayed_notifications_++] = element;
                    }

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Peek at the next element in the packet "
                             "elems_.\n"));

                  // Set up for the next element in elems_ by peek()'ing.
                  element = this->elems_.peek();

                  if (element != 0)
                    {
                      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                                 "The is an element still in the packet "
                                 "elems_ (we are peeking at it now).\n"));

                     VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                                "We are going to find the tail block for the "
                                "current element (we are peeking at).\n"));

                      // There was a "next element".  Determine the
                      // elem_tail_block for it.
                      elem_tail_block = element->msg();

                      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                                 "Start w/tail block == element->msg().\n"));

                      while (elem_tail_block->cont() != 0)
                        {
                          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                                     "Set tail block to next in chain.\n"));
                          elem_tail_block = elem_tail_block->cont();
                        }

                      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                                 "Done finding tail block.\n"));
                    }
                }
              else
                {
                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Ok.  The fully sent block is *not* a "
                             "duplicate of the tail block of the element "
                             "at the front of the packet elems_.\n"));

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Thus, we have not completely sent the "
                             "element yet.\n"));

                  // We didn't completely send the element - it has more
                  // message blocks that haven't been sent (that we know of).

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "We can release the fully_sent_block now.\n"));

                  // Release the fully_sent_block
                  fully_sent_block->release();
                }
            }
        }
      else
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Only part of the block at the front of pkt_chain_ "
                     "was sent.\n"));

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Advance the rd_ptr() of the front block (of pkt_chain_) "
                     "by the num_bytes_left (%d).\n", num_bytes_left));

          // Only part of the current block was sent.
          this->pkt_chain_->rd_ptr(num_bytes_left);

          if (this->header_complete_ == 1)
            {
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "And since the packet header block has already been "
                         "completely sent, add num_bytes_left to the "
                         "num_non_header_bytes_sent.\n"));

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "Before, num_non_header_bytes_sent == %d.\n",
                         num_non_header_bytes_sent));

              // We know that the current block isn't the packet header
              // block because the packet header block has already been
              // completely sent.  We need to count these bytes in the
              // num_non_header_bytes_sent.
              num_non_header_bytes_sent += num_bytes_left;

              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                         "After, num_non_header_bytes_sent == %d.\n",
                         num_non_header_bytes_sent));
            }

          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                     "Set the num_bytes_left to 0 now.\n"));

          num_bytes_left = 0;
        }
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The 'num_bytes_left' loop has completed.\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Adjust the header_.length_ to account for the "
             "num_non_header_bytes_sent.\n"));
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Before, header_.length_ == %d.\n",
             this->header_.length_));

  // Adjust the packet header_.length_ to indicate how many non header
  // bytes are left to send.
  this->header_.length_ -= num_non_header_bytes_sent;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "After, header_.length_ == %d.\n",
             this->header_.length_));

  // Returns 0 if the entire packet was sent, and returns 1 otherwise.
  int rc = (this->header_.length_ == 0) ? 0 : 1;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Adjustments all done.  Returning [%d].  0 means entire packet "
             "has been sent.  1 means otherwise.\n",
             rc));

  return rc;
}

void 
TAO::DCPS::TransportSendStrategy::send_delayed_notifications()
{
  for (size_t i = 0; i < num_delayed_notifications_; i++)
  {
    delayed_delivered_notification_queue_[i]->data_delivered();
  }

  num_delayed_notifications_ = 0;
}


