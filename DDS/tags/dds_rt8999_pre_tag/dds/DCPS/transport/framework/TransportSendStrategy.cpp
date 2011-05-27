// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportSendStrategy.h"
#include  "RemoveAllVisitor.h"
#include  "TransportConfiguration.h"
#include  "ThreadSynchStrategy.h"
#include  "ThreadSynchResource.h"
#include  "TransportQueueElement.h"
#include  "BuildChainVisitor.h"
#include  "QueueRemoveVisitor.h"
#include  "PacketRemoveVisitor.h"
#include  "TransportDefs.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

//TBD: The number of chunks of the replace element allocator
//     is hard coded for now. This will be configurable when
//     we implement the dds configurations. This value should
//     be the number of marshalled DataSampleHeader that a 
//     packet could contain.
#define NUM_REPLACED_ELEMENT_CHUNKS 20

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
    queue_(new QueueType (config->queue_messages_per_pool_,config->queue_initial_pools_)),
    elems_(new QueueType (1,config->max_samples_per_packet_)),
    pkt_chain_(0),
    header_complete_(0),
    start_counter_(0),
    mode_(MODE_DIRECT),
    mode_before_suspend_(MODE_NOT_SET),
    num_delayed_notifications_(0),
    header_db_allocator_(1),
    header_mb_allocator_(2),
    replaced_element_allocator_(NUM_REPLACED_ELEMENT_CHUNKS)
{
  DBG_ENTRY("TransportSendStrategy","TransportSendStrategy");

  config->_add_ref ();
  this->config_ = config;

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

  if (this->elems_)
    delete this->elems_;
  if (this->queue_)
    delete this->queue_;
}


TAO::DCPS::TransportSendStrategy::WorkOutcome
TAO::DCPS::TransportSendStrategy::perform_work()
{
  DBG_ENTRY("TransportSendStrategy","perform_work");

  SendPacketOutcome outcome;

  { // scope for the guard(this->lock_);
    GuardType guard(this->lock_);

    if (this->mode_ == MODE_TERMINATED)
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Entered perform_work() and mode_ is MODE_TERMINATED - "
                  "we lost connection and could not reconnect, just return "
                  "WORK_OUTCOME_BROKEN_RESOURCE.\n"));
        return WORK_OUTCOME_BROKEN_RESOURCE;
      }

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
    if (this->mode_ != MODE_QUEUE && this->mode_ != MODE_SUSPEND) 
      {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                  "Entered perform_work() and mode_ is %s - just return "
                  "WORK_OUTCOME_NO_MORE_TO_DO.\n", mode_as_str (this->mode_)));
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
        if (this->queue_->size() == 0)
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
    if ((outcome == OUTCOME_COMPLETE_SEND) && (this->queue_->size() == 0))
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
  if ((outcome == OUTCOME_COMPLETE_SEND) && (this->queue_->size() == 0))
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
          
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "Now flip to MODE_SUSPEND before we try to reconnect.\n"));

      bool do_suspend = true;
      this->relink (do_suspend);

      if (this->mode_ == MODE_SUSPEND)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                    "The reconnect has not done yet and we are still in MODE_SUSPEND. "
                    "Return WORK_OUTCOME_ClOGGED_RESOURCE.\n"));
          // Return WORK_OUTCOME_NO_MORE_TO_DO to tell our caller that we
          // don't desire another call to this perform_work() method.
          return WORK_OUTCOME_NO_MORE_TO_DO;
        }
      else if (this->mode_ == MODE_TERMINATED)
        {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Reconnect failed, now we are in MODE_TERMINATED\n"));
          return WORK_OUTCOME_BROKEN_RESOURCE;
        }
      else
        {
          // If the datalink is re-established then notify the synch
          // thread to perform work.
          this->synch_->work_available();
        }
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
  TransportQueueElement* element = this->elems_->peek();

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
                  element = this->elems_->get();

                  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                             "Tell the element that a decision has been made "
                             "regarding its fate - data_delivered().\n"));

                  // Inform the element that the data has been delivered.
                  if (delay_notification == NOTIFY_IMMEADIATELY)
                    {
                      if (this->mode_ == MODE_TERMINATED)
                        {
                          bool dropped_by_transport = true;
                          element->data_dropped (dropped_by_transport);
                        }
                      else
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
                  element = this->elems_->peek();

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


/// Remove all samples in the backpressure queue and packet queue.
void
TAO::DCPS::TransportSendStrategy::terminate_send ()
{
  DBG_ENTRY("TransportSendStrategy","terminate_send");

  QueueType* elems;
  QueueType* queue;

  {
    GuardType guard(this->lock_);
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
              "Reconnect failed. Now flip to MODE_TERMINATED.\n"));

    this->mode_ = MODE_TERMINATED;

    if (this->header_.length_ > 0)
      {
        // Clear the messages in the pkt_chain_ that is partially sent.
        // We just reuse these functions for normal partial send except actual sending.
        int num_bytes_left = this->pkt_chain_->total_length ();
        int result = this->adjust_packet_after_send(num_bytes_left, NOTIFY_IMMEADIATELY);
        if (result == 0)
          {
            VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                      "The adjustment logic says that the packet is cleared.\n"));
          }
        else
          {
            ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: terminate_send  adjust_packet_after_send()"
              " should not return partial send.\n"));
          }
      }

    elems = this->elems_;
    this->elems_ = new QueueType (this->config_->queue_messages_per_pool_,this->config_->queue_initial_pools_);
    queue = this->queue_;
    this->queue_ = new QueueType (1,this->config_->max_samples_per_packet_);
  }

  // We need remove the queued elements outside the lock,
  // otherwise we have a deadlock situation when remove vistor
  // calls the data_droped on each dropped elements.
  
  // Clear all samples in queue.
  RemoveAllVisitor remove_all_visitor;

  elems->accept_remove_visitor(remove_all_visitor);
  queue->accept_remove_visitor(remove_all_visitor);

  delete elems;
  delete queue;
}



void
TAO::DCPS::TransportSendStrategy::send(TransportQueueElement* element)
{
  DBG_ENTRY("TransportSendStrategy","send");

  GuardType guard(this->lock_);

  if (this->mode_ == MODE_TERMINATED)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "TransportSendStrategy::send: this->mode_ == MODE_TERMINATED, so discard message.\n"));
      bool dropped_by_transport = true;
      element->data_dropped (dropped_by_transport);
      return;
    }

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
  if (this->mode_ == MODE_QUEUE || this->mode_ == MODE_SUSPEND)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "this->mode_ == %s, so queue elem and leave.\n", mode_as_str (this->mode_)));

      this->queue_->put(element);
      if (this->mode_ != MODE_SUSPEND)
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
      ((this->elems_->size() != 0) && (element_requires_exclusive_packet == 1)))
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
                     "the mode_ is now MODE_QUEUE or MODE_SUSPEND.  Queue elem and leave.\n"));
          this->queue_->put(element);
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
  this->elems_->put(element);
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
  if ((this->elems_->size() == this->max_samples_)                            ||
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


void
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

  if (this->mode_ == MODE_TERMINATED)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "TransportSendStrategy::send_stop: dont try to send current packet "
        "since this->mode_ == MODE_TERMINATED\n"));
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

  // If our mode_ is currently MODE_QUEUE or MODE_SUSPEND, then we don't have
  // anything to do here because samples have already been going to the
  // queue.  We only need to do something if the mode_ is
  // MODE_DIRECT.  It means that we may have some sample(s) in the
  // current packet that have never been sent.  This is our
  // opportunity to send the current packet directly if this is the case.
  if (this->mode_ == MODE_QUEUE || this->mode_ == MODE_SUSPEND)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                 "But since we are in %s, we don't have to do "
                 "anything more in this important send_stop().\n",
                 mode_as_str(this->mode_)));
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


int
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

      this->elems_->accept_remove_visitor(remove_element_visitor);

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
  this->queue_->accept_remove_visitor(remove_element_visitor);

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
  this->elems_->accept_replace_visitor(remove_from_packet_visitor);

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


void
TAO::DCPS::TransportSendStrategy::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY("TransportSendStrategy","remove_all_control_msgs");

  GuardType guard(this->lock_);

  QueueRemoveVisitor remove_element_visitor(pub_id);

  if (this->mode_ == MODE_DIRECT)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "The mode is MODE_DIRECT.\n"));

      this->elems_->accept_remove_visitor(remove_element_visitor);

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
  this->queue_->accept_remove_visitor(remove_element_visitor);

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
  this->elems_->accept_replace_visitor(remove_from_packet_visitor);

  status = remove_from_packet_visitor.status();

  if (status == -1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: PacketRemoveVisitor encountered fatal error.\n"));
    }
}



void
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
  while (1)
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
                    "Now flip to MODE_SUSPEND before we try to reconnect.\n"));

          if (this->mode_ != MODE_SUSPEND)
            {
              this->mode_before_suspend_ = this->mode_;
              this->mode_ = MODE_SUSPEND;
            }
          bool do_suspend = false;
          this->relink (do_suspend);

          if (this->mode_ == MODE_SUSPEND)
            {
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                        "The reconnect has not done yet and we are still in MODE_SUSPEND.\n"));
            }
          else if (this->mode_ == MODE_TERMINATED)
            {
              VDBG((LM_DEBUG, "(%P|%t) DBG:   "
                "Reconnect failed, we are in MODE_TERMINATED\n"));
              break;
            }
          else
            {
              // Try send the packet again since the connection is re-established.
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


void
TAO::DCPS::TransportSendStrategy::get_packet_elems_from_queue()
{
  DBG_ENTRY("TransportSendStrategy","get_packet_elems_from_queue");

  TransportQueueElement* element = this->queue_->peek();

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
          if (this->elems_->size() == 0)
            {
              // The current packet is empty so we won't violate the
              // exclusive_packet requirement by put()'ing the element
              // into the elems_ collection.  We also extract the current
              // element from the queue_ at this time (we've been peek()'ing
              // at the element all this time).
              this->elems_->put(this->queue_->get());
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
      this->elems_->put(this->queue_->get());
      this->header_.length_ += element_length;

      // If the current number of packet elems_ has reached the maximum
      // number of samples per packet, then we are done.
      if (this->elems_->size() == this->max_samples_)
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
      element = this->queue_->peek();
    }
}


void
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
  this->elems_->accept_visitor(visitor);

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


TAO::DCPS::TransportSendStrategy::SendPacketOutcome
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


