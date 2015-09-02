#include "TcpSubscriber.h"
#include "TestData.h"
#include <ace/Message_Block.h>

TestStats*     TcpSubscriber::stats_      = 0;
ACE_Reactor*   TcpSubscriber::r_          = 0;
TcpSubscriber::TestAllocator*
               TcpSubscriber::allocator_  = 0;
uintptr_t      TcpSubscriber::block_size_ = 0;


TcpSubscriber::TcpSubscriber()
  : packet_count_(0)
{
  remainder_ = new ACE_Message_Block(TcpSubscriber::block_size_,
                                     ACE_Message_Block::MB_DATA,
                                     0,
                                     0,
                                     TcpSubscriber::allocator_);
}


TcpSubscriber::~TcpSubscriber()
{
  delete remainder_;
}


void
TcpSubscriber::initSubscriber(TestStats*     stats,
                              ACE_Reactor*   r,
                              TestAllocator* allocator,
                              unsigned       block_size)
{
  TcpSubscriber::stats_      = stats;
  TcpSubscriber::r_          = r;
  TcpSubscriber::allocator_  = allocator;
  TcpSubscriber::block_size_ = block_size;
}


int
TcpSubscriber::open(void*)
{
  ACE_INET_Addr addr;

  if (this->peer().get_remote_addr(addr) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) TcpSubscriber can't retrieve client address\n"),
                     -1);
  }

  if (r_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) TcpSubscriber has a null ptr to the reactor\n"),
                     -1);
  }

  // Register ourselves with the reactor using the READ_MASK.
  if (r_->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) TcpSubscriber can't register with reactor\n"),
                     -1);
  }

  return 0;
}


int
TcpSubscriber::close(u_long)
{
  destroy();
  return 0;
}


void
TcpSubscriber::destroy()
{
  // Remove ourselves from the reactor
  r_->remove_handler
                (this,
                 ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);

  // Shut down the connection to the client.
  peer().close();
}


int
TcpSubscriber::handle_input(ACE_HANDLE)
{
  ACE_Message_Block* raw_block = remainder_->duplicate();

  // Don't do anything here if the remainder_'s read pointer is
  // set to be at the very beginning of the underlying buffer.
  if (remainder_->base() != remainder_->rd_ptr()) {
    if (remainder_->rd_ptr() == remainder_->wr_ptr()) {
      // There aren't any remainder bytes.  Just reset the raw_block's
      // read and write pointers to the beginning of the underlying buffer.
      raw_block->rd_ptr(raw_block->base());
      raw_block->wr_ptr(raw_block->base());
    }
    else {
      // We need to check to see just how many bytes are left in the
      // underlying buffer.  If more than half of the underlying buffer
      // is past the end of the remainder_'s write pointer, then we
      // don't do a thing - and simply leave the raw_block's read and
      // write pointers alone (they equal the remainder_'s pointers).
      // However, if there is less than half of the buffer following
      // the remainder bytes, we need to move the remainder_ content
      // to the beginning of the underlying buffer - adjusting the
      // raw_block accordingly.
      uintptr_t bytes_used = remainder_->wr_ptr() - remainder_->base();
      if (bytes_used > (TcpSubscriber::block_size_ / 2)) {
        raw_block->rd_ptr(raw_block->base());
        raw_block->wr_ptr(raw_block->base());
        raw_block->copy(remainder_->rd_ptr(),
                        remainder_->wr_ptr() - remainder_->rd_ptr());
      }
    }
  }

  // We can release the remainder now.
  remainder_->release();
  remainder_ = 0;

  // We need to receive as many bytes as possible into the raw block.
  uintptr_t num_bytes_left = TcpSubscriber::block_size_ -
                            (raw_block->wr_ptr() - raw_block->base());

  ssize_t result;

  if ((result = peer().recv(raw_block->wr_ptr(), num_bytes_left)) == 0) {
    // The publisher has disconnected - check if this was unexpected.
    if (!(TcpSubscriber::stats_->is_num_packets(packet_count_))) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) TcpSubscriber::handle_input Publisher disconnected prematurely when "
                 "packet_count is %d.\n",
                 packet_count_));
    }

    // If we get here, then we have already received all of the packets
    // we expected from the publisher, so finding out that the publisher
    // has disconnected is actually expected.  But we always return -1
    // to indicate that we are done.
    return -1;
  }
  else if (result < 0) {
    // Something bad happened
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%P|%t) TcpSubscriber::handle_input bad read\n"),
                      -1);
  }

  // Move the write pointer forward the number of bytes we read.
  raw_block->wr_ptr(static_cast<size_t>(result));

  // Create any ACE_Message_Block pairs (header + payload), and insert
  // them into the msg_list.
  TestStats::MsgList msg_list;

  unsigned header_size = sizeof(TestData::Header);

  while (1) {
    // Check if a complete header exists in the the raw_block.
    //   If not, we are done collecting pairs.
    uintptr_t raw_size = raw_block->wr_ptr() - raw_block->rd_ptr();

    if (header_size > raw_size) {
      // We are done.
      break;
    }

    // Get the header_block by duplicating the raw_block, and then
    // adjusting the write pointer of the header block.
    ACE_Message_Block* header_block = raw_block->duplicate();
    header_block->wr_ptr(header_block->rd_ptr());
    header_block->wr_ptr(header_size);

    // Ask the header record for the payload size.
    TestData::Header* header_record =
                            (TestData::Header*)(header_block->rd_ptr());
    unsigned payload_size = header_record->payload_size_;

    // Check if a complete payload exists in the raw_block.
    //   If not, we need to release the header block before
    //   breaking out of this "collect pairs" loop.
    if (header_size + payload_size > raw_size) {
      header_block->release();
      break;
    }

    // We know there is both the full header and the full payload in
    // the raw_block.  We've already grabbed the header block - adjust
    // the raw_block's read pointer appropriately.
    raw_block->rd_ptr(header_size);

    // Create the payload block and adjust it's write pointer appropriately.
    ACE_Message_Block* payload_block = raw_block->duplicate();
    payload_block->wr_ptr(payload_block->rd_ptr());
    payload_block->wr_ptr(payload_size);

    // Adjust the read pointer of the raw_block to indicate that we have
    // taken the payload from the underlying buffer.
    raw_block->rd_ptr(payload_size);

    // Connect the header_block to point to the payload_block.
    // This is a "pair" that we can push into the msg_list.
    header_block->cont(payload_block);


    // Insert the pair into the msg_list.
    msg_list.push_back(header_block);
  }

  // Save the raw_block as the remainder_ for the next handle_input() call.
  remainder_ = raw_block->duplicate();

  // Release the raw_block now.
  raw_block->release();

  packet_count_ += static_cast<unsigned int>(msg_list.size());

  // Provide the TestStats object with the list.
  //
  // NOTE: This will "consume" the msg_list - making it empty upon return!
  //       (And it releases each Message Block pair).
  TcpSubscriber::stats_->messages_received(msg_list);

  return 0;
}


int
TcpSubscriber::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  destroy();
  return 0;
}
