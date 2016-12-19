/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportReceiveStrategy_T.h"
#include "ace/INET_Addr.h"

#if !defined (__ACE_INLINE__)
#include "TransportReceiveStrategy_T.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename TH, typename DSH>
TransportReceiveStrategy<TH, DSH>::TransportReceiveStrategy()
  : gracefully_disconnected_(false),
    receive_sample_remaining_(0),
    mb_allocator_(MESSAGE_BLOCKS),
    db_allocator_(DATA_BLOCKS),
    data_allocator_(DATA_BLOCKS),
    buffer_index_(0),
    payload_(0),
    good_pdu_(true),
    pdu_remaining_(0)
{
  DBG_ENTRY_LVL("TransportReceiveStrategy", "TransportReceiveStrategy" ,6);

  if (Transport_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-mb"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               &mb_allocator_, MESSAGE_BLOCKS));
    ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-db"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               &db_allocator_, DATA_BLOCKS));
    ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-data"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               &data_allocator_, DATA_BLOCKS));
  }

  ACE_OS::memset(this->receive_buffers_, 0, sizeof(this->receive_buffers_));
}

template<typename TH, typename DSH>
TransportReceiveStrategy<TH, DSH>::~TransportReceiveStrategy()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","~TransportReceiveStrategy",6);

  if (this->receive_buffers_[this->buffer_index_] != 0) {
    size_t size = this->receive_buffers_[this->buffer_index_]->total_length();

    if (size > 0) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: TransportReceiveStrategy::~TransportReceiveStrategy() - ")
                 ACE_TEXT("terminating with %d unprocessed bytes.\n"),
                 size));
    }
  }
}

template<typename TH, typename DSH>
bool
TransportReceiveStrategy<TH, DSH>::check_header(const TH& /*header*/)
{
  return true;
}

template<typename TH, typename DSH>
bool
TransportReceiveStrategy<TH, DSH>::check_header(const DSH& /*header*/)
{
  return true;
}

/// Note that this is just an initial implementation.  We may take
/// some shortcuts (we will) that will need to be dealt with later once
/// a more robust implementation can be put in place.
///
/// Our handle_dds_input() method is called by the reactor when there is
/// data to be pulled from our peer() ACE_SOCK_Stream.
template<typename TH, typename DSH>
int
TransportReceiveStrategy<TH, DSH>::handle_dds_input(ACE_HANDLE fd)
{
  DBG_ENTRY_LVL("TransportReceiveStrategy", "handle_dds_input", 6);

  //
  // What we will be doing here:
  //
  //   1) Read as much data as possible from the handle.
  //   2) Process each Transport layer packet
  //      A) Parse the Transport header
  //      B) Process each DataSample in the packet
  //         a) Parse the DataSampleHeader
  //         b) Parse the remainder of the sample
  //         c) call data_received for complete samples
  //         d) store any partial sample or header data
  //   3) return
  //
  // The state that we might need to maintain includes:
  //     I) Current Transport layer header
  //    II) Current DataSample header
  //   III) Current DataSample data
  //
  // For each of these elements, they can be:
  //     i) empty
  //    ii) partial (not able to parse yet).
  //   iii) complete (parsed values available).
  //
  // The read buffers will be a series of data buffers of a fixed size
  // that are each managed by an ACE_Data_Block, each of which is
  // managed by an ACE_Message_Block containing the read and write
  // pointers into the data.
  //
  // As messages are parsed, new ACE_Message_Blocks will be formed with
  // read and write pointers adjusted to reference the individual
  // DataSample buffer contents.
  //
  // The Underlying buffers, ACE_Data_Blocks, and ACE_Message_Blocks
  // will be acquired from a Cached_Allocater_with_overflow.  The size
  // of the individual data buffers will be set initially to the
  // ethernet MTU to allow for full TCP messages to be received into
  // individual blocks.
  //
  // Reading will be performed with a two member struct iov, to allow
  // for the remainder of the current buffer to be used along with at
  // least one completely empty buffer.  There will be a low water
  // parameter used to stop the use of the current block when the
  // available space in it is reduced.
  //

  //
  // Establish the current receive buffer.
  //
  //   This involves checking for any remainder from the previous trip
  //   through the code.  If there is no current buffer, or the buffer
  //   has less than the low water mark space() left, then promote the
  //   next receive buffer to the current.  If none is present, create a
  //   new one and use it.
  //
  //
  // Establish the next receive buffer.
  //
  //   This involves checking for any previous complete buffers that
  //   were not promoted in the previous step.  If none are present,
  //   create a new one and use that.
  //

  //
  // Remove any buffers that have been completely read and have less
  // than the low water amount of space left.
  //
  size_t index;

  for (index = 0; index < RECEIVE_BUFFERS; ++index) {
    if ((this->receive_buffers_[index] != 0)
        && (this->receive_buffers_[index]->length() == 0)
        && (this->receive_buffers_[index]->space() < BUFFER_LOW_WATER)) {
      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "Remove a receive_buffer_[%d] from use.\n",
            index));

      // unlink any Message_Block that continues to this one
      // being removed.
      // This avoids a possible infinite ->cont() loop.
      for (size_t ii =0; ii < RECEIVE_BUFFERS; ii++) {
        if ((0 != this->receive_buffers_[ii]) &&
            (this->receive_buffers_[ii]->cont() ==
             this->receive_buffers_[index])) {
          this->receive_buffers_[ii]->cont(0);
        }
      }

      //
      // Remove the receive buffer from use.
      //
      ACE_DES_FREE(
        this->receive_buffers_[index],
        this->mb_allocator_.free,
        ACE_Message_Block);
      this->receive_buffers_[index] = 0;
    }
  }

  //
  // Allocate buffers for any empty slots.  We may have emptied one just
  // here, but others may have been emptied by a large read during the
  // last trip through the code as well.
  //
  size_t previous = this->buffer_index_;

  for (index = this->buffer_index_;
       this->successor_index(previous) != this->buffer_index_;
       index = this->successor_index(index)) {
    if (this->receive_buffers_[index] == 0) {
      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "Allocate a Message_Block for new receive_buffer_[%d].\n",
            index));

      ACE_NEW_MALLOC_RETURN(
        this->receive_buffers_[index],
        (ACE_Message_Block*) this->mb_allocator_.malloc(
          sizeof(ACE_Message_Block)),
        ACE_Message_Block(
          RECEIVE_DATA_BUFFER_SIZE,     // Buffer size
          ACE_Message_Block::MB_DATA,   // Default
          0,                            // Start with no continuation
          0,                            // Let the constructor allocate
          &this->data_allocator_,       // Our buffer cache
          &this->receive_lock_,         // Our locking strategy
          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY, // Default
          ACE_Time_Value::zero,         // Default
          ACE_Time_Value::max_time,     // Default
          &this->db_allocator_,         // Our data block cache
          &this->mb_allocator_          // Our message block cache
        ),
        -1);
    }

    //
    // Chain the buffers.  This allows us to have portions of parsed
    // data cross buffer boundaries without a problem.
    //
    if (previous != index) {
      this->receive_buffers_[previous]->cont(
        this->receive_buffers_[index]);
    }

    previous = index;
  }

  //
  // Read from handle.
  //
  //   This involves a non-blocking recvv_n().  Any remaining space in
  //   the buffers should be saved for the next pass through the code.
  //   If more than one receive buffer has data, chain them.  Promote
  //   all receive buffers so that the next pass through, the first
  //   buffer with space remaining will appear as the current receive
  //   buffer.
  //
  //   This is probably what should remain in the specific
  //   implementation, with the rest of this factored back up into the
  //   framework.
  //

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "Form the iovec from the message block\n"));

  //
  // Form the iovec from the message block chain of receive buffers.
  //
  iovec iov[RECEIVE_BUFFERS];
  size_t vec_index = 0;
  size_t current = this->buffer_index_;

  for (index = 0;
       index < RECEIVE_BUFFERS;
       ++index, current = this->successor_index(current)) {
    // Invariant.  ASSERT?
    if (this->receive_buffers_[current] == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Unrecoverably corrupted ")
                        ACE_TEXT("receive buffer management detected.\n")),
                       -1);
    }

#ifdef _MSC_VER
#pragma warning(push)
// iov_len is 32-bit on 64-bit VC++, but we don't want a cast here
// since on other platforms iov_len is 64-bit
#pragma warning(disable : 4267)
#endif
    // This check covers the case where we have unread data in
    // the first buffer, but no space to write any more data.
    if (this->receive_buffers_[current]->space() > 0) {
      iov[vec_index].iov_len  = this->receive_buffers_[current]->space();
      iov[vec_index].iov_base = this->receive_buffers_[current]->wr_ptr();

      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "index==%d, len==%d, base==%x\n",
            vec_index, iov[vec_index].iov_len, iov[vec_index].iov_base));

      vec_index++;
    }
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "Perform the recvv() call\n"));

  //
  // Read into the buffers.
  //
  ACE_INET_Addr remote_address;
  ssize_t bytes_remaining = this->receive_bytes(iov,
                                                static_cast<int>(vec_index),
                                                remote_address,
                                                fd);

  if (bytes_remaining < 0) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Problem ")
              ACE_TEXT("with data link detected: %p.\n"),
              ACE_TEXT("receive_bytes")));

    // The relink() will handle the connection to the ReconnectTask to do
    // the reconnect so this reactor thread will not be block.
    this->relink();

    // Close connection anyway.
    return -1;
    // Returning -1 takes the handle out of the reactor read mask.
  }

  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
            "recvv() return %d - we call this the bytes_remaining.\n",
            bytes_remaining), 5);

  if (bytes_remaining == 0) {
    if (this->gracefully_disconnected_) {
      VDBG_LVL((LM_INFO,
                ACE_TEXT("(%P|%t) Peer has gracefully disconnected.\n"))
               ,1);
      return -1;

    } else {
      VDBG_LVL((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Unrecoverable problem ")
                ACE_TEXT("with data link detected\n")), 1);

      // The relink() will handle the connection to the ReconnectTask to do
      // the reconnect so this reactor thread will not be block.
      this->relink();

      // Close connection anyway.
      return -1;
      // Returning -1 takes the handle out of the reactor read mask.
    }
  }

  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
            "START Adjust the message block chain pointers to account for the "
            "new data.\n"), 5);

  //
  // Adjust the message block chain pointers to account for the new
  // data.
  //
  size_t bytes = bytes_remaining;

  if (!this->pdu_remaining_) {
    this->receive_transport_header_.length_ = static_cast<ACE_UINT32>(bytes);
  }

  for (index = this->buffer_index_;
       bytes > 0;
       index = this->successor_index(index)) {
    VDBG((LM_DEBUG,"(%P|%t) DBG:    -> "
          "At top of for..loop block.\n"));
    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "index == %d.\n", index));
    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "bytes == %d.\n", bytes));

    size_t amount
    = ace_min<size_t>(bytes, this->receive_buffers_[index]->space());

    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "amount == %d.\n", amount));

    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "this->receive_buffers_[index]->rd_ptr() ==  %u.\n",
          this->receive_buffers_[index]->rd_ptr()));
    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "this->receive_buffers_[index]->wr_ptr() ==  %u.\n",
          this->receive_buffers_[index]->wr_ptr()));

    this->receive_buffers_[index]->wr_ptr(amount);

    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "Now, this->receive_buffers_[index]->wr_ptr() ==  %u.\n",
          this->receive_buffers_[index]->wr_ptr()));

    bytes -= amount;

    VDBG((LM_DEBUG,"(%P|%t) DBG:       "
          "Now, bytes == %d .\n", bytes));

    // This is yukky to do here, but there is a fine line between
    // where things are moved and where they are checked.
    if (bytes > 0 && this->successor_index(index) == this->buffer_index_) {
      // Here we have read more data than we passed in buffer.  Bad.
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Unrecoverably corrupted ")
                        ACE_TEXT("receive buffer management detected: ")
                        ACE_TEXT("read more bytes than available.\n")),
                       -1);
    }
  }

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "DONE Adjust the message block chain pointers to account "
        "for the new data.\n"));

  //
  // While the receive buffer is not empty:
  //
  //   This is the receive buffer(s) that was just read.  The receive
  //   buffer message block is used to manage our parsing through this
  //   data.  When we have parsed out all the data, then it will be
  //   considered empty.  This message block read pointer indicates
  //   where we are in the parsing process.  Other message blocks refer
  //   to the parsed data and retain the data block references to
  //   prevent the data from being released until we have completed all
  //   uses for the data in the receive buffer.  This will normally be
  //   after the sample data is demarshaled in the DataReader
  //   components.
  //
  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "Start processing the data we just received.\n"));

  //
  // Operate while we have more data to process.
  //
  while (true) {

    //
    // Manage the current transport header.
    //
    //   This involves checking to see if we have remaining data in the
    //   current packet.  If not, or if we have not read a complete
    //   header, then we read a transport header and check it for
    //   validity.  As we have a valid transport header with data
    //   remaining to read in the packet that it represents, we move on.
    //
    //   As new transport headers are to be read, they are read into a
    //   message buffer member variable and demarshaled directly.  The
    //   values are retained for the lifetime of the processing of the
    //   instant transport packet.  The member message buffer allows us
    //   to retain partially read transport headers until we can read
    //   more data.
    //

    //
    // Check the remaining transport packet length to see if we are
    // expecting a new one.
    //
    if (this->pdu_remaining_ == 0) {
      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "We are expecting a transport packet header.\n"));

      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "this->buffer_index_ == %d.\n",this->buffer_index_));

      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "this->receive_buffers_[this->buffer_index_]->rd_ptr() "
            "== %u.\n",
            this->receive_buffers_[this->buffer_index_]->rd_ptr()));

      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "this->receive_buffers_[this->buffer_index_]->wr_ptr() "
            "== %u.\n",
            this->receive_buffers_[this->buffer_index_]->wr_ptr()));

      if (this->receive_buffers_[this->buffer_index_]->total_length()
          < this->receive_transport_header_.max_marshaled_size()) {
        //
        // Not enough room in the buffer for the entire Transport
        // header that we need to read, so relinquish control until
        // we get more data.
        //
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Not enough bytes read to account for a transport "
              "packet header.  We are done here - we need to "
              "receive more bytes.\n"));

        this->receive_transport_header_.incomplete(
          *this->receive_buffers_[this->buffer_index_]);

        return 0;

      } else {
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "We have enough bytes to demarshall the transport "
              "packet header.\n"));

        // only do the hexdump if it will be printed - to not impact performance.
        if (Transport_debug_level > 5) {
          ACE_TCHAR xbuffer[4096];
          const ACE_Message_Block& mb =
            *this->receive_buffers_[this->buffer_index_];
          size_t xbytes = mb.length();

          xbytes = (std::min)(xbytes, TH::max_marshaled_size());

          ACE::format_hexdump(mb.rd_ptr(), xbytes, xbuffer, sizeof(xbuffer));

          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "Hex Dump of transport header block "
                "(%d bytes):\n%s\n", xbytes, xbuffer));
        }

        //
        // Demarshal the transport header.
        //
        this->receive_transport_header_ =
          *this->receive_buffers_[this->buffer_index_];

        //
        // Check the TransportHeader.
        //
        if (!this->receive_transport_header_.valid()) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT
                            ("(%P|%t) ERROR: TransportHeader invalid.\n")),
                           -1);
        }

        this->good_pdu_ = check_header(this->receive_transport_header_);
        this->pdu_remaining_ = this->receive_transport_header_.length_;

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Amount of transport packet bytes (remaining): %d.\n",
              this->pdu_remaining_));
      }
    }

    //
    // Ignore bad PDUs by skipping over them.  We do this out here
    // in case we did not skip over the entire bad PDU last time.
    //
    {
      int rtn_code = skip_bad_pdus();
      if (rtn_code <= 0) return rtn_code;
    }

    //
    // Keep processing samples while we have data to read.
    //
    while (this->pdu_remaining_ > 0) {
      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "We have a transport packet now.  There are more sample "
            "bytes to parse in order to complete the packet.\n"));

      //
      // Manage the current sample header.
      //
      //  This involves checking to see if we have remaining data in the
      //  current sample.  If not, or if we have not read a complete
      //  header, then we read a sample header and check it for validity,
      //  which currently involves checking the message type only.  As we
      //  have a valid sample header with data remaining to read in the
      //  sample that it represents, we move on.
      //
      //  As new sample headers are read, the are read into a message
      //  buffer member variable and demarshaled directly.  The values are
      //  retained for the lifetime of the sample and are passed as part
      //  of the recieve data sample itself.  The member message buffer
      //  allows us to retain partially read sample headers until we can
      //  read more data.
      //
      if (this->receive_sample_remaining_ == 0) {
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "We are not working on some remaining sample data.  "
              "We are expecting to parse a sample header now.\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->buffer_index_ == %d.\n",this->buffer_index_));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->rd_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->wr_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->wr_ptr()));

        if (DSH::partial(*this->receive_buffers_[this->buffer_index_])) {
          //
          // Not enough room in the buffer for the entire Sample
          // header that we need to read, so relinquish control until
          // we get more data.
          //
          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "Not enough bytes have been received to account "
                "for a complete sample header.  We are done for "
                "now - we need to receive more data before we "
                "can go on.\n"));
          if (Transport_debug_level > 2) {
            ACE_TCHAR ebuffer[350];
            ACE_Message_Block& mb = *this->receive_buffers_[this->buffer_index_];
            const size_t sz = (std::min)(DSH::max_marshaled_size(), mb.length());
            ACE::format_hexdump(mb.rd_ptr(), sz, ebuffer, sizeof(ebuffer));
            ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG:   "
              "Partial DataSampleHeader:\n%s\n", ebuffer));
          }

          return 0;

        } else {
          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "We have received enough bytes for the sample "
                "header.  Demarshall the sample header now.\n"));

          // only do the hexdump if it will be printed - to not impact performance.
          if (Transport_debug_level > 5) {
            ACE_TCHAR ebuffer[4096];
            ACE::format_hexdump
            (this->receive_buffers_[this->buffer_index_]->rd_ptr(),
             this->data_sample_header_.max_marshaled_size(),
             ebuffer, sizeof(ebuffer));

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                  "Hex Dump:\n%s\n", ebuffer));
          }

          this->data_sample_header_.pdu_remaining(this->pdu_remaining_);

          //
          // Demarshal the sample header.
          //
          this->data_sample_header_ =
            *this->receive_buffers_[this->buffer_index_];

          //
          // Check the DataSampleHeader.
          //

          this->good_pdu_ = check_header(data_sample_header_);

          //
          // Set the amount to parse into the message buffer.  We
          // can't just use the header value to keep track of
          // where we are since downstream processing will expect
          // the value to be correct (unadjusted by us).
          //
          this->receive_sample_remaining_ =
            this->data_sample_header_.message_length();

          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "The demarshalled sample header says that we "
                "have %d bytes to read for the data portion of "
                "the sample.\n",
                this->receive_sample_remaining_));

          //
          // Decrement packet size.
          //
          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "this->data_sample_header_.marshaled_size() "
                "== %d.\n",
                this->data_sample_header_.marshaled_size()));

          this->pdu_remaining_
          -= this->data_sample_header_.marshaled_size();

          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                "Amount of transport packet remaining: %d.\n",
                this->pdu_remaining_));

          int rtn_code = skip_bad_pdus();
          if (rtn_code <= 0) return rtn_code;
        }
      }

      bool last_buffer = false;
      update_buffer_index(last_buffer);

      if (this->receive_sample_remaining_ > 0) {
        //
        // Manage the current sample data.
        //
        //   This involves reading data to complete the current sample.  As
        //   samples are completed, they are dispatched via the
        //   data_received() mechanism.  This data is read into message
        //   blocks that are obtained from the pool of message blocks since
        //   the lifetime of this data will last until the DataReader
        //   components demarshal the sample data.  A reference to the
        //   current sample being built is retained as a member to allow us
        //   to hold partialy read samples until they are completed.
        //
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Determine amount of data for the next block in the chain\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->buffer_index_ == %d.\n",this->buffer_index_));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->rd_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->wr_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->wr_ptr()));
        //
        // Determine the amount of data for the next block in the chain.
        //
        const size_t amount = ace_min<size_t>(
          this->receive_sample_remaining_,
          this->receive_buffers_[this->buffer_index_]->length());

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "amount of data for the next block in the chain is %d\n",
              amount));
        //
        // Now create a message block for the data in the current buffer
        // and chain it if we are starting a new sample.
        //
        ACE_Message_Block* current_sample_block = 0;
        ACE_NEW_MALLOC_RETURN(
          current_sample_block,
          (ACE_Message_Block*) this->mb_allocator_.malloc(
            sizeof(ACE_Message_Block)),
          ACE_Message_Block(
            this->receive_buffers_[this->buffer_index_]
            ->data_block()->duplicate(),
            0,
            &this->mb_allocator_),
          -1);

        //
        // Chain it to the end of the current sample.
        //
        if (this->payload_ == 0) {
          this->payload_ = current_sample_block;

        } else {
          ACE_Message_Block* block = this->payload_;

          while (block->cont() != 0) {
            block = block->cont();
          }

          block->cont(current_sample_block);
        }

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Before adjustment of the pointers and byte counters\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->payload_->rd_ptr() "
              "== %u.\n",
              this->payload_->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->payload_->wr_ptr() "
              "== %u.\n",
              this->payload_->wr_ptr()));

        //
        // Adjust the pointers and byte counters.
        //
        current_sample_block->rd_ptr(
          this->receive_buffers_[this->buffer_index_]->rd_ptr());
        current_sample_block->wr_ptr(current_sample_block->rd_ptr() + amount);
        this->receive_buffers_[this->buffer_index_]->rd_ptr(amount);
        this->receive_sample_remaining_ -= amount;
        this->pdu_remaining_            -= amount;

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "After adjustment of the pointers and byte counters\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->payload_->rd_ptr() "
              "== %u.\n",
              this->payload_->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->payload_->wr_ptr() "
              "== %u.\n",
              this->payload_->wr_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->rd_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "this->receive_buffers_[this->buffer_index_]->wr_ptr() "
              "== %u.\n",
              this->receive_buffers_[this->buffer_index_]->wr_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "After adjustment, remaining sample bytes == %d\n",
              this->receive_sample_remaining_));
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "After adjustment, remaining transport packet bytes == %d\n",
              this->pdu_remaining_));
      }

      //
      // Dispatch the received message if we have received it all.
      //
      // NB: Since we are doing this synchronously and without passing
      //     ownership of the sample, we can use NULL mutex lock for
      //     the allocators.  Since only one thread can be in this
      //     routine at a time, that is.
      //
      if (this->receive_sample_remaining_ == 0) {
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Now dispatch the sample to the DataLink\n"));

        ReceivedDataSample rds(this->payload_);
        this->payload_ = 0;  // rds takes ownership of payload_
        if (this->data_sample_header_.into_received_data_sample(rds)) {

          if (this->data_sample_header_.more_fragments()
              || this->receive_transport_header_.last_fragment()) {
            VDBG((LM_DEBUG,"(%P|%t) DBG:   Attempt reassembly of fragments\n"));

            if (this->reassemble(rds)) {
              VDBG((LM_DEBUG,"(%P|%t) DBG:   Reassembled complete message\n"));
              this->deliver_sample(rds, remote_address);
            }
            // If reassemble() returned false, it takes ownership of the data
            // just like deliver_sample() does.

          } else {
            this->deliver_sample(rds, remote_address);
          }
        }

        // For the reassembly algorithm, the 'last_fragment_' header bit only
        // applies to the first DataSampleHeader in the TransportHeader
        this->receive_transport_header_.last_fragment(false);

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "Release the sample that we just sent.\n"));
        // ~ReceivedDataSample() releases the payload_ message block
      }

      update_buffer_index(last_buffer);

      if (last_buffer) {
        // Relinquish control if there is no more data to process.
        VDBG((LM_DEBUG,"(%P|%t) DBG:   We are done - no more data.\n"));
        return 0;
      }

    } // End of while (this->pdu_remaining_ > 0)

    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
          "Let's try to do some more.\n"));
  } // End of while (true)

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "It looks like we are done - the done loop has finished.\n"));
  //
  // Relinquish control.
  //
  //   This involves ensuring that when we reenter this method, we will
  //   pick up from where we left off correctly.
  //
  return 0;
}

template<typename TH, typename DSH>
bool
TransportReceiveStrategy<TH, DSH>::reassemble(ReceivedDataSample&)
{
  ACE_DEBUG((LM_WARNING, "(%P|%t) TransportReceiveStrategy::reassemble() "
    "WARNING: derived class must override if specific transport type uses "
    "fragmentation and reassembly\n"));
  return false;
}

template<typename TH, typename DSH>
void
TransportReceiveStrategy<TH, DSH>::reset()
{
  this->receive_sample_remaining_ = 0;
  ACE_Message_Block::release(this->payload_);
  this->payload_ = 0;
  this->good_pdu_ = true;
  this->pdu_remaining_ = 0;
  for (int i = 0; i < RECEIVE_BUFFERS; ++i) {
    ACE_Message_Block& rb = *this->receive_buffers_[i];
    rb.rd_ptr(rb.wr_ptr());
  }
}

template<typename TH, typename DSH>
void
TransportReceiveStrategy<TH, DSH>::update_buffer_index(bool& done)
{
  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
        "Adjust the buffer chain in case we crossed into the next "
        "buffer after the last read(s).\n"));
  const size_t initial = this->buffer_index_;
  while (this->receive_buffers_[this->buffer_index_]->length() == 0) {
    this->buffer_index_ = this->successor_index(this->buffer_index_);

    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
          "Set this->buffer_index_ = %d.\n",
          this->buffer_index_));

    if (initial == this->buffer_index_) {
      done = true; // no other buffers in receive_buffers_ have data
      return;
    }
  }
}

template<typename TH, typename DSH>
int
TransportReceiveStrategy<TH, DSH>::skip_bad_pdus()
{
  if (this->good_pdu_) return 1;

  //
  // Adjust the message block chain pointers to account for the
  // skipped data.
  //
  for (size_t index = this->buffer_index_;
       this->pdu_remaining_ > 0;
       index = this->successor_index(index)) {
    size_t amount =
      ace_min<size_t>(this->pdu_remaining_, this->receive_buffers_[index]->length());

    this->receive_buffers_[index]->rd_ptr(amount);
    this->pdu_remaining_ -= amount;

    if (this->pdu_remaining_ > 0 && this->successor_index(index) == this->buffer_index_) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("TransportReceiveStrategy::skip_bad_pdus()")
                        ACE_TEXT(" - Unrecoverably corrupted ")
                        ACE_TEXT("receive buffer management detected: ")
                        ACE_TEXT("read more bytes than available.\n")),
                       -1);
    }
  }

  this->receive_sample_remaining_ = 0;

  this->receive_sample_remaining_ = 0;

  bool done = false;
  update_buffer_index(done);
  return done ? 0 : 1;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
