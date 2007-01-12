// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportReceiveStrategy.h"
#include  "ace/INET_Addr.h"


#if !defined (__ACE_INLINE__)
#include "TransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportReceiveStrategy::TransportReceiveStrategy()
  : gracefully_disconnected_(false),
    receive_sample_remaining_(0),
    mb_allocator_(MESSAGE_BLOCKS),
    db_allocator_(DATA_BLOCKS),
    data_allocator_(DATA_BLOCKS),
    buffer_index_(0)
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) %@ TransportReceiveStrategy::TransportReceiveStrategy\n", this));
  DBG_ENTRY_LVL("TransportReceiveStrategy","TransportReceiveStrategy",5);

  if (DCPS_debug_level >= 2)
    {
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

  // No aggregate assignment possible in initializer list.  That I know
  // of anyway.
  ACE_OS::memset(this->receive_buffers_, 0x0, sizeof(this->receive_buffers_));
}


TAO::DCPS::TransportReceiveStrategy::~TransportReceiveStrategy()
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) TransportReceiveStrategy::~TransportReceiveStrategy\n"));
  //fflush (stdout);
  DBG_ENTRY_LVL("TransportReceiveStrategy","~TransportReceiveStrategy",5);
}

/// Note that this is just an initial implementation.  We may take
/// some shortcuts (we will) that will need to be dealt with later once
/// a more robust implementation can be put in place.
///
/// Our handle_input() method is called by the reactor when there is
/// data to be pulled from our peer() ACE_SOCK_Stream.
//TAO::DCPS::TransportReceiveStrategy::handle_input(ACE_HANDLE)
int
TAO::DCPS::TransportReceiveStrategy::handle_input()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","handle_input",5);

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
  for( index = 0 ; index < RECEIVE_BUFFERS ; ++index)
    {
      if ( (this->receive_buffers_[ index] != 0)
        && (this->receive_buffers_[ index]->length() == 0)
        && (this->receive_buffers_[ index]->space() < BUFFER_LOW_WATER)
         )
        {
          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                     "Remove a receive_buffer_[%d] from use.\n",
           index));

          // unlink any Message_Block that continues to this one
          // being removed.
          // This avoids a possible infinite ->cont() loop.
          for (size_t ii =0; ii < RECEIVE_BUFFERS; ii++)
            {
              if ( (0 != this->receive_buffers_[ii]) &&
                    (this->receive_buffers_[ii]->cont() ==
                       this->receive_buffers_[ index]) )
                {
                  this->receive_buffers_[ii]->cont(0);
                }
            }

          //
          // Remove the receive buffer from use.
          //
          ACE_DES_FREE(
            this->receive_buffers_[ index],
            this->mb_allocator_.free,
            ACE_Message_Block
          ) ;
          this->receive_buffers_[ index] = 0 ;
        }
    }

  //
  // Allocate buffers for any empty slots.  We may have emptied one just
  // here, but others may have been emptied by a large read during the
  // last trip through the code as well.
  //
  size_t previous = this->buffer_index_;
  for( index = this->buffer_index_;
       this->successor_index( previous) != this->buffer_index_ ;
       index = this->successor_index( index)
     )
    {
      if( this->receive_buffers_[ index] == 0)
        {
          VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                     "Allocate a Message_Block for new receive_buffer_[%d].\n",
                     index));

          ACE_NEW_MALLOC_RETURN(
            this->receive_buffers_[ index],
            (ACE_Message_Block*) this->mb_allocator_.malloc(
                                   sizeof(ACE_Message_Block)
                                 ),
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
            -1
          ) ;
        }

      //
      // Chain the buffers.  This allows us to have portions of parsed
      // data cross buffer boundaries without a problem.
      //
      if( previous != index)
        {
          this->receive_buffers_[ previous]->cont(
            this->receive_buffers_[index]
          ) ;
        }

      previous = index ;
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
  iovec iov[ RECEIVE_BUFFERS] ;
  size_t vec_index = 0 ;
  size_t current = this->buffer_index_;
  for( index = 0 ;
       index < RECEIVE_BUFFERS ;
       ++index, current = this->successor_index( current)
     )
  {
    // Invariant.  ASSERT?
    if (this->receive_buffers_[ current] == 0)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Unrecoverably corrupted ")
                          ACE_TEXT("receive buffer management detected.\n")),
                         -1) ;
      }

    // This check covers the case where we have an unread fragment in
    // the first buffer, but no space to write any more data.
    if( this->receive_buffers_[ current]->space() > 0)
      {
        iov[ vec_index].iov_len  = this->receive_buffers_[ current]->space() ;
        iov[ vec_index].iov_base = this->receive_buffers_[ current]->wr_ptr() ;

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "index==%d, len==%d, base==%x\n",
                   vec_index, iov[vec_index].iov_len, iov[vec_index].iov_base
                 ));

        vec_index++ ;
      }
  }

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
             "Perform the recvv() call\n"));

  //
  // Read into the buffers.
  //
  ACE_INET_Addr remote_address;
  ssize_t bytes_remaining = this->receive_bytes(iov,
                                                vec_index,
                                                remote_address);

  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
             "recvv() return %d - we call this the bytes_remaining.\n",
            bytes_remaining), 5);

  if( bytes_remaining == 0 && this->gracefully_disconnected_)
    {
      VDBG_LVL((LM_ERROR,
                ACE_TEXT("(%P|%t) Peer has gracefully disconnected.\n"))
               ,1);
      return -1;
    }
  else if((bytes_remaining == 0 && ! this->gracefully_disconnected_)
    || bytes_remaining < 0)
    {
      VDBG_LVL((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Unrecoverable problem ")
                      ACE_TEXT("with data link detected: %p.\n"),
                "receive_bytes"), 1);

      // The relink() will handle the connection to the ReconnectTask to do
      // the reconnect so this reactor thread will not be block.
      this->relink ();

      // Close connection anyway.
      return -1;
      // Returning -1 takes the handle out of the reactor read mask.
    }

  VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
             "START Adjust the message block chain pointers to account for the "
            "new data.\n"), 5);

  //
  // Adjust the message block chain pointers to account for the new
  // data.
  //
  size_t  bytes = bytes_remaining;
  for( index = this->buffer_index_ ;
       bytes > 0 ;
       index = this->successor_index( index)
     )
    {
      VDBG((LM_DEBUG,"(%P|%t) DBG:    -> "
                 "At top of for..loop block.\n"));
      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "index == %d.\n", index));
      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "bytes == %d.\n", bytes));

      size_t amount
        = ace_min<size_t>( bytes, this->receive_buffers_[ index]->space()) ;

      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "amount == %d.\n", amount));

      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "this->receive_buffers_[ index]->rd_ptr() ==  %u.\n",
                 this->receive_buffers_[ index]->rd_ptr()));
      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "this->receive_buffers_[ index]->wr_ptr() ==  %u.\n",
                 this->receive_buffers_[ index]->wr_ptr()));

      this->receive_buffers_[ index]->wr_ptr( amount) ;

      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "Now, this->receive_buffers_[ index]->wr_ptr() ==  %u.\n",
                 this->receive_buffers_[ index]->wr_ptr()));

      bytes -= amount ;

      VDBG((LM_DEBUG,"(%P|%t) DBG:       "
                 "Now, bytes == %d .\n", bytes));

      // This is yukky to do here, but there is a fine line between
      // where things are moved and where they are checked.
      if (bytes > 0 && this->successor_index( index) == this->buffer_index_)
        {
          // Here we have read more data than we passed in buffer.  Bad.
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Unrecoverably corrupted ")
                            ACE_TEXT("receive buffer management detected: ")
                            ACE_TEXT("read more bytes than available.\n")),
                           -1) ;
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
  bool done = false ;
  while( done == false) {

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
    // Check the transport packet length.  We can use the header value
    // here to keep track of how much packet is left since we fully
    // consume the header here and no downstream processing will expect
    // to examine the value.
    //
    if( this->receive_transport_header_.length_ == 0)
      {
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "We are expecting a transport packet header.\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->buffer_index_ == %d.\n",this->buffer_index_));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_buffers_[ this->buffer_index_]->rd_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_buffers_[ this->buffer_index_]->wr_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->wr_ptr()));

        if( this->receive_buffers_[ this->buffer_index_]->total_length()
            < this->receive_transport_header_.max_marshaled_size())
          {
            //
            // Not enough room in the buffer for the entire Transport
            // header that we need to read, so relinquish control until
            // we get more data.
            //
            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "Not enough bytes read to account for a transport "
                       "packet header.  We are done here - we need to "
                       "receive more bytes.\n"));
            return 0;
          }
        else
          {
            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "We have enough bytes to demarshall the transport "
                       "packet header.\n"));

            // only do the hexdump if it will be printed - to not impact perfomance.
            if (::TAO::DCPS::Transport_debug_level)
              {
                char xbuffer[4096];
                int xbytes =
                        this->receive_buffers_[this->buffer_index_]->length();
                if (xbytes > 8) { xbytes = 8; }
                ACE::format_hexdump
                       (this->receive_buffers_[this->buffer_index_]->rd_ptr(),
                        xbytes, xbuffer, sizeof(xbuffer)) ;

                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "Hex Dump of (marshaled) transport header block "
                       "(%d bytes):\n%s\n", xbytes,xbuffer));
              }

            //
            // Demarshal the transport header.
            //
            this->receive_transport_header_
              = this->receive_buffers_[ this->buffer_index_] ;

            //
            // Check the TransportHeader.
            //
            if( this->receive_transport_header_.valid() == false)
              {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT
                                      ("(%P|%t) ERROR: TransportHeader invalid.\n")),
                                 -1) ;
              }

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "Amount of transport packet bytes (remaining): %d.\n",
                       this->receive_transport_header_.length_));
          }
      }

    //
    // Keep processing samples while we have data to read.
    //
    while( this->receive_transport_header_.length_ > 0)
      {
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
        if( this->receive_sample_remaining_ == 0)
          {
            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "We are not working on some remaining sample data.  "
                       "We are expecting to parse a sample header now.\n"));

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "this->buffer_index_ == %d.\n",this->buffer_index_));

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "this->receive_buffers_[ this->buffer_index_]->rd_ptr() "
                       "== %u.\n",
                       this->receive_buffers_[ this->buffer_index_]->rd_ptr()));

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "this->receive_buffers_[ this->buffer_index_]->wr_ptr() "
                       "== %u.\n",
                       this->receive_buffers_[ this->buffer_index_]->wr_ptr()));

            // TODO: We should use the marshaled_size() instead of max_marshaled_size()
            //       for this checking.
            if( this->receive_buffers_[ this->buffer_index_]->total_length()
                < this->receive_sample_.header_.max_marshaled_size())
              {
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
                return 0;
              }
            else
              {
                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                           "We have received enough bytes for the sample "
                           "header.  Demarshall the sample header now.\n"));

                // only do the hexdump if it will be printed - to not impact perfomance.
                if (::TAO::DCPS::Transport_debug_level)
                  {
                    char ebuffer[4096] ;
                    ACE::format_hexdump
                       (this->receive_buffers_[this->buffer_index_]->rd_ptr(),
                        this->receive_sample_.header_.max_marshaled_size(),
                        ebuffer, sizeof(ebuffer)) ;

                    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                              "Hex Dump:\n%s\n", ebuffer));
                  }

                //
                // Demarshal the sample header.
                //
                this->receive_sample_.header_
                  = this->receive_buffers_[ this->buffer_index_] ;

                //
                // Check the DataSampleHeader.
                //
                /// @TODO

                //
                // Set the amount to parse into the message buffer.  We
                // can't just use the header value to keep track of
                // where we are since downstream processing will expect
                // the value to be correct (unadjusted by us).
                //
                this->receive_sample_remaining_
                  = this->receive_sample_.header_.message_length_ ;

                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                           "The demarshalled sample header says that we "
                           "have %d bytes to read for the data portion of "
                           "the sample.\n",
                           this->receive_sample_remaining_));

                //
                // Decrement packet size.
                //
                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                           "this->receive_sample_.header_.marshaled_size() "
                           "== %d.\n",
                           this->receive_sample_.header_.marshaled_size()));

                this->receive_transport_header_.length_
                  -= this->receive_sample_.header_.marshaled_size() ;

                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                           "Amount of transport packet remaining: %d.\n",
                           this->receive_transport_header_.length_));
              }
          }

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "Adjust the buffer chain in case we crossed into the next "
                   "buffer after the last read(s).\n"));
        //
        // Adjust the buffer chain in case we crossed into the next
        // buffer after the last read(s).
        //
        size_t initial = this->buffer_index_ ;
        while( this->receive_buffers_[ this->buffer_index_]->length() == 0)
          {
            this->buffer_index_ = this->successor_index( this->buffer_index_) ;

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "Set this->buffer_index_ = %d.\n",
                       this->buffer_index_));

            if( initial == this->buffer_index_)
              {
                //
                // All buffers are empty, we have no more data to process.
                //
                VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                           "We have 'consumed' all of the received data.  "
                           "We are done (for now)\n"));
                return 0;
              }
          }

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
                   "this->receive_buffers_[ this->buffer_index_]->rd_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_buffers_[ this->buffer_index_]->wr_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->wr_ptr()));
     //
        // Determine the amount of data for the next block in the chain.
        //
        size_t amount
                 = ace_min<size_t>(
                     this->receive_sample_remaining_,
                     this->receive_buffers_[ this->buffer_index_]->length()
                   ) ;

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "amount of data for the next block in the chain is %d\n",
                   amount));
        //
        // Now create a message block for the data in the current buffer
        // and chain it if we are starting a new sample.
        //
        ACE_Message_Block* current_sample_block = 0 ;
        ACE_NEW_MALLOC_RETURN(
          current_sample_block,
          (ACE_Message_Block*) this->mb_allocator_.malloc(
                                 sizeof(ACE_Message_Block)
                               ),
          ACE_Message_Block(
            this->receive_buffers_[ this->buffer_index_]
              ->data_block()->duplicate(),
            0,
            &this->mb_allocator_
          ),
          -1
        ) ;

        //
        // Chain it to the end of the current sample.
        //
        if( this->receive_sample_.sample_ == 0)
          {
            this->receive_sample_.sample_ = current_sample_block ;
          }
        else
          {
            ACE_Message_Block* block = this->receive_sample_.sample_ ;
            while (block->cont() != 0 )
              {
                 block = block->cont() ;
              }
            block->cont (current_sample_block) ;
          }

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "Before adjustment of the pointers and byte counters\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_sample_.sample_->rd_ptr() "
                   "== %u.\n",
                   this->receive_sample_.sample_->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_sample_.sample_->wr_ptr() "
                   "== %u.\n",
                   this->receive_sample_.sample_->wr_ptr()));

        //
        // Adjust the pointers and byte counters.
        //
        current_sample_block->rd_ptr(
          this->receive_buffers_[ this->buffer_index_]->rd_ptr()
        ) ;
        current_sample_block->wr_ptr(
          this->receive_buffers_[ this->buffer_index_]->wr_ptr()
        ) ;
        this->receive_buffers_[ this->buffer_index_]->rd_ptr( amount) ;
        this->receive_sample_remaining_         -= amount ;
        this->receive_transport_header_.length_ -= amount ;

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "After adjustment of the pointers and byte counters\n"));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_sample_.sample_->rd_ptr() "
                   "== %u.\n",
                   this->receive_sample_.sample_->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_sample_.sample_->wr_ptr() "
                   "== %u.\n",
                   this->receive_sample_.sample_->wr_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_buffers_[ this->buffer_index_]->rd_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->rd_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "this->receive_buffers_[ this->buffer_index_]->wr_ptr() "
                   "== %u.\n",
                   this->receive_buffers_[ this->buffer_index_]->wr_ptr()));

        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "After adjustment, remaining sample bytes == %d\n",
                   this->receive_sample_remaining_));
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                   "After adjustment, remaining transport packet bytes == %d\n",
                   this->receive_transport_header_.length_));

        //
        // Dispatch the received message if we have received it all.
        //
        // NB: Since we are doing this synchronously and without passing
        //     ownership of the sample, we can use NULL mutex lock for
        //     the allocators.  Since only one thread can be in this
        //     routine at a time, that is.
        //
        if( this->receive_sample_remaining_ == 0)
          {
            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                 "Now dispatch the sample to the DataLink\n"));

            this->deliver_sample(this->receive_sample_, remote_address);

            VDBG((LM_DEBUG,"(%P|%t) DBG:   "
                       "Release the sample that we just sent.\n"));
            //
            // Release the entire chain.  This manages the data block
            // refcount and buffer space as well.
            //
            // @TODO: Manage this differently if we pass ownership.
            //
            this->receive_sample_.sample_->release() ;
            this->receive_sample_.sample_ = 0 ;
          }

      } // End of while( this->receive_transport_header_.length_ > 0)

    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
               "Let's try to do some more.\n"));
  } // End of while( done == false)

  VDBG((LM_DEBUG,"(%P|%t) DBG:   "
             "It looks like we are done - the done loop has finished.\n"));
  //
  // Relinquish control.
  //
  //   This involves ensuring that when we reenter this method, we will
  //   pick up from where we left off correctly.
  //
  return 0 ;
}
