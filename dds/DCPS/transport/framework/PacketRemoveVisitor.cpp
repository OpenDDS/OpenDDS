/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PacketRemoveVisitor.h"
#include "TransportRetainedElement.h"
#include "ace/Message_Block.h"

#if !defined (__ACE_INLINE__)
#include "PacketRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

PacketRemoveVisitor::PacketRemoveVisitor(
  const TransportQueueElement::MatchCriteria& match,
  ACE_Message_Block*& unsent_head_block,
  ACE_Message_Block* header_block,
  MessageBlockAllocator& mb_allocator,
  DataBlockAllocator& db_allocator)
  : match_(match)
  , head_(unsent_head_block)
  , header_block_(header_block)
  , status_(REMOVE_NOT_FOUND)
  , current_block_(0)
  , previous_block_(0)
  , replaced_element_mb_allocator_(mb_allocator)
  , replaced_element_db_allocator_(db_allocator)
{
  DBG_ENTRY_LVL("PacketRemoveVisitor", "PacketRemoveVisitor", 6);
}

PacketRemoveVisitor::~PacketRemoveVisitor()
{
  DBG_ENTRY_LVL("PacketRemoveVisitor", "~PacketRemoveVisitor", 6);
}

int
PacketRemoveVisitor::visit_element_ref(TransportQueueElement*& element)
{
  DBG_ENTRY_LVL("PacketRemoveVisitor", "visit_element_ref", 6);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Obtain the element_blocks using element->msg()\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "The element is [%0x]\n",
        element));

  if (element->is_retained_replaced()) {
    status_ = REMOVE_FOUND;
    return 0;
  }

  // These is the head of the chain of "source" blocks from the element
  // currently being visited.
  ACE_Message_Block* element_blocks =
    const_cast<ACE_Message_Block*>(element->msg());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "element_blocks == [%0x]\n", element_blocks));

  // As we visit an element, we also adjust our current_block_ and
  // previous_block_ data members such that we can correlate the current
  // element (being visited) with the message blocks that it contributed
  // to the unsent packet blocks.  Our head_ data member was set to the
  // first unsent block in the chain of blocks that make up the remaining
  // portions of the "packet".

  if (this->current_block_ == 0) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "this->current_block_ == 0.  Visiting first element.\n"));

    // This must be our first visit_element() call.  Set up the
    // current_block_ and previous_block_ data members appropriately.
    this->current_block_ = this->head_;
    this->previous_block_ = 0;

    // If the current_block_ is still zero, there is nothing to do here,
    // so cancel the visitation
    if (this->current_block_ == 0) {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "No blocks to iterate through, ending visitation.\n"));
      return 0;
    }

    // There is a chance that the head_ block (and thus the current_block_)
    // is actually a duplicate of the packet header_block_.  If so, we
    // need to adjust the current_block_ and previous_block_ appropriately.
    if (this->header_block_->base() == this->current_block_->base()) {
      // Yup.  Just what we thought may be the case.
      this->previous_block_ = this->current_block_;
      this->current_block_ = this->previous_block_->cont();
    }

  } else {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "this->current_block_ != 0.  Visiting element other than "
          "the first element.\n"));

    // We are visiting an element that is not the very first element in
    // the packet.

    // Let's get the previous_block_ data member set to point to the
    // block in the packet chain that is the predecessor to the first
    // block in the packet chain that was contributed from the current
    // element.
    this->previous_block_ = this->current_block_;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set previous_block_ to the current_block_ [%0x]\n",
          this->previous_block_));

    // Keep changing the previous_block_ to the next block in the chain
    // until we know that the next block in the chain is a duplicate of
    // the block at the front of the element_blocks chain.
    while (this->previous_block_->cont()->base() != element_blocks->base()) {
      this->previous_block_ = this->previous_block_->cont();

      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Moved previous_block_ to its cont() block [%0x]\n",
            this->previous_block_));
    }

    // At this point, we know that the previous_block_ is the block
    // that immediately precedes the first block contributed by the
    // element that we are currently visiting.  Set the current_block_
    // to point to the first block contributed by the element.
    this->current_block_ = this->previous_block_->cont();

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set current_block_ to the previous_block_->cont() [%0x]\n",
          this->current_block_));
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Does the current element match the sample to be replaced?\n"));

  // Does the current element (being visited) match the sample that we
  // need to remove?
  if (this->match_.matches(*element)) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "YES - The element matches the sample\n"));

    // We get inside here if the element we are currently visiting is
    // the element that "matches" the sample that needs to be removed.

    // At this point, the current_block_ points to the first block in
    // the packet that is known to have been contributed by the
    // element that we are currently visiting.

    // The previous_block_ is either pointing to the block in the packet
    // that immediately precedes the current_block_, or the previous_block_
    // is set to 0 indicating that the current_block_ also happens to
    // be the head_ block in the packet (and the element we are visiting
    // is the first element (remaining) in the packet).

    // Our goal now is to extract the blocks from the packet that have
    // been contributed by the element that we are currently visiting.
    // Then, we will need to replace those blocks in the packet with
    // our own blocks.  How the replacement blocks are created depends
    // upon whether or not we are visiting the first element in the packet.

    // The original_blocks will end up being a chain of blocks
    // extracted from the packet, all of which were contributed by
    // the current element being visited.
    ACE_Message_Block* original_blocks = this->current_block_;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set original_blocks to this->current_block_ [%0x]\n",
          original_blocks));

    // The remaining_chain will end up being the chain of blocks that
    // followed the original blocks in the packet.  There is always
    // the possibility that the remaining chain will be 0, meaning that
    // we are currently visiting (and removing) the last element in the
    // packet.
    ACE_Message_Block* remaining_chain = this->current_block_->cont();

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set remaining_chain to this->current_block_->cont() [%0x]\n",
          remaining_chain));

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set original_blocks->cont(0)\n"));

    // At this point, we only know for sure that one block was
    // contributed by the element currently being visited.
    original_blocks->cont(0);

    unsigned int num_elem_blocks_sent = 0;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set num_elem_blocks_sent to 0\n"));

    // The original_blocks_tail is a pointer to the last block in the
    // chain of blocks contributed by the element currently being visited.
    ACE_Message_Block* original_blocks_tail = original_blocks;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set original_blocks_tail to original_blocks [%0x]\n",
          original_blocks_tail));

    // Find the block in the element_blocks that contributed the
    // block pointed to by the original_blocks_tail.
    ACE_Message_Block* contrib_block = element_blocks;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set contrib_block to element_blocks [%0x]\n",
          contrib_block));

    // Loop through each block in the element_blocks until we either
    // find the contributing element block, or we have checked all of the
    // element_blocks, and never found the contributing element block.
    while (contrib_block != 0) {
      if (contrib_block->base() == original_blocks->base()) {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   "
              "contrib_block->base() == original_blocks->base()\n"));
        // Ok.  We have found the source block.
        break;
      }

      // That wasn't a match.  Try the next contrib_block to see
      // if it is the contributing block for the block at the top of
      // the original_blocks chain (which is a chain of 1 at this point).
      contrib_block = contrib_block->cont();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Move contrib_block to contrib_block->cont() [%0x]\n",
            contrib_block));
      ++num_elem_blocks_sent;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "num_elem_blocks_sent incremented to %d\n",
            num_elem_blocks_sent));
    }

    // Sanity check - make sure that we found the contributing block
    // in the current element (being visited) for the contributed block
    // that is the lone block in the original_blocks chain.
    if (contrib_block == 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Element queue and unsent message block "
                 "chain is out-of-sync. source_block == 0.\n"));

      // Set the status to indicate a fatal error occurred.
      this->status_ = REMOVE_ERROR;

      // Stop vistation now.
      return 0;
    }

    // Now that we have identified the contributing block for the
    // single block in the original_blocks chain, we may need to add
    // more blocks to the original_blocks chain - one more block for
    // each additional block chained to the element's contributing block.
    // Note that this while loop doesn't do anything if the contrib_block
    // is the last contributing block in the element.  In this case, the
    // original_blocks contains the lone block that was contributed by
    // the lone (last) contributing block in the element - and the
    // remaining_chain properly points to the remaining blocks in the
    // packet.
    while (contrib_block->cont() != 0) {
      // The source element block indicates that it has a "next"
      // block that would have also contributed a block to the packet.

      // This means that there is a block at the front of the
      // remaining_chain of blocks that really should be part of the
      // original_blocks.

      // Sanity check - the remaining_chain better not be NULL (0).
      if (remaining_chain == 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Element queue and unsent message block "
                   "chain is out-of-synch. remaining_chain == 0.\n"));
        this->status_ = REMOVE_ERROR;
        return 0;
      }

      // Extract/unchain the first block from the remaining_chain.
      ACE_Message_Block* additional_block = remaining_chain;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Extracted additional_block from remaining_chain [%0x]\n",
            additional_block));

      remaining_chain = remaining_chain->cont();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Move remaining_chain to remaining_chain->cont() [%0x]\n",
            remaining_chain));

      additional_block->cont(0);
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Set additional_block->cont(0)\n"));

      // Attach the block to the end of the original_blocks chain.
      original_blocks_tail->cont(additional_block);
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "original_blocks_tail->cont(additional_block)\n"));

      original_blocks_tail = additional_block;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Set original_blocks_tail to additional_block [%0x]\n",
            original_blocks_tail));

      // Advance to the next contributing block.
      contrib_block = contrib_block->cont();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Move contrib_block to contrib_block->cont() [%0x]\n",
            contrib_block));
    }

    // Finally!At this point we have broken the unsent packet chain of
    // blocks into three seperate chains:
    //
    //   (1) this->previous_block_ is either 0, or it points to the block
    //       (from the unsent packet chain) that immediately preceded the
    //       first block (from the unsent packet chain) that was contributed
    //       by the sample (that we need to replace).
    //       this->previous_block_ is 0 when the contributed blocks from
    //       the sample (that we are replacing) are the first blocks from
    //       the unsent packet chain.
    //
    //       Thus, sub-chain (1) is either an empty chain (when
    //       this->previous_block_ is 0), or it is a chain that starts
    //       with the head_ block (the first block from the unsent packet
    //       chain), and ends with the this->previous_block_.
    //
    //   (2) original_blocks points to the first block (from the unsent
    //       packet chain) that was contributed by the sample (that we
    //       need to replace).
    //
    //       Thus, sub-chain (2) is a chain that starts with the block
    //       pointed to by original_blocks.
    //
    //   (3) remaining_chain points to the first block (from the unsent
    //       packet chain) that followed the last block that was
    //       contributed by the sample (that we need to replace).
    //
    //       Thus, sub-chain (3) is a chain that starts with the block
    //       pointed to by remaining_chain.  Note that this may be 0 if
    //       the sample being replaced is the last sample in the packet.
    //
    // If sub-chains (1), (2), and (3) were chained together (in that
    // order), we would end up with the original unsent packet chain.
    // Whew.

    // Now we can perform our replacement duties.

    // Save off the pointer to the original element
    TransportQueueElement* orig_elem = element;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Create the new TransportReplacedElement using the "
          "orig_elem [%0x]\n",
          orig_elem));

    // Create the replacement element for the original element.
    element = new
      TransportReplacedElement(orig_elem,
                               &this->replaced_element_mb_allocator_,
                               &this->replaced_element_db_allocator_);

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "The new TransportReplacedElement is [%0x]\n",
          element));

    // Now we have to deal with replacing the original_blocks chain
    // with duplicates from the msg() chain of the replacement element.

    ACE_Message_Block* replacement_element_blocks =
      const_cast<ACE_Message_Block*>(element->msg());
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set replacement_element_blocks to the replacement element's "
          "msg() [%0x]\n",
          replacement_element_blocks));

    // Move through the chain to account for the num_elem_blocks_sent
    for (unsigned int i = 0; i < num_elem_blocks_sent; i++) {
      replacement_element_blocks = replacement_element_blocks->cont();

      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Moved replacement_element_blocks to its cont() block "
            "[%0x]\n",
            replacement_element_blocks));
    }

    // Make a duplicate of the replacement_element_blocks chain
    ACE_Message_Block* replacement_blocks =
      replacement_element_blocks->duplicate();

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set replacement_blocks to duplicate of "
          "replacement_element_blocks [%0x]\n",
          replacement_blocks));

    // Now adjust the block at the front of the replacement_blocks chain
    // to match the block at the front of the original_blocks chain -
    // with respect to the difference between the rd_ptr() setting and
    // the base() setting.
    size_t rd_offset = original_blocks->rd_ptr() - original_blocks->base();

    if (rd_offset > 0) {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Call replacement_blocks->rd_ptr(rd_offset) with "
            "rd_offset == [%d]\n",
            rd_offset));
      replacement_blocks->rd_ptr(rd_offset);
    }

    // Find the last block (the tail) in the replacement_blocks chain
    ACE_Message_Block* replacement_blocks_tail = replacement_blocks;

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Set replacement_blocks_tail to replacement_blocks "
          "[%0x]\n",
          replacement_blocks_tail));

    while (replacement_blocks_tail->cont() != 0) {
      replacement_blocks_tail = replacement_blocks_tail->cont();
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Moved replacement_blocks_tail to its cont() block "
            "[%0x]\n",
            replacement_blocks_tail));
    }

    // Now we can stitch the unsent packet chain back together using the
    // replacement blocks instead of the orig_blocks.
    replacement_blocks_tail->cont(remaining_chain);

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Stitched replacement_blocks_tail to remaining_chain.\n"));

    if (this->previous_block_ == 0) {
      // Replacing blocks at the head of the unsent packet chain.
      this->head_ = replacement_blocks;
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Replacing blocks at head of unsent packet chain.\n"));

    } else {
      // Replacing blocks not at the head of the unsent packet chain.
      this->previous_block_->cont(replacement_blocks);
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Replacing blocks not at head of unsent packet chain.\n"));
    }

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Release the original_blocks.\n"));

    // Release the chain of original blocks.
    original_blocks->release();

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "Tell original element that data_dropped().\n"));

    // Tell the original element (that we replaced), data_dropped()
    // by transport.
    // This visitor is used in TransportSendStrategy::do_remove_sample
    // and TransportSendBuffer::retain_all. In former case, the sample
    // is dropped as a result of writer's remove_sample call. In the
    // later case, the dropped_by_transport is not used as the sample
    // is retained sample and no callback is made to writer.
    this->status_ = orig_elem->data_dropped() ? REMOVE_RELEASED : REMOVE_FOUND;

    if (this->status_ == REMOVE_RELEASED || this->match_.unique()) {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Return 0 to halt visitation.\n"));
      // Replace a single sample if one is specified, otherwise visit the
      // entire queue replacing each sample with the specified
      // publication Id value.
      return 0;
    }
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "Return 1 to continue visitation.\n"));

  // Continue visitation.
  return 1;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
