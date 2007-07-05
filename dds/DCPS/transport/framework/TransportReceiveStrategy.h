// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY
#define OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "TransportHeader.h"
#include "ReceivedDataSample.h"
#include "TransportDefs.h"
#include "ace/Synch.h"


namespace OpenDDS
{

  namespace DCPS
  {
    /**
     * This class provides buffer for data received by transports, de-assemble
     * the data to individual samples and deliver them. 
     */ 
    class OpenDDS_Dcps_Export TransportReceiveStrategy 
      : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        virtual ~TransportReceiveStrategy();

        int start();
        void stop();

        int handle_input();

        /// The subclass needs to provide the implementation
        /// for re-establishing the datalink. This is called
        /// when recv returns an error.
        virtual void relink (bool do_suspend = true);

      protected:

        TransportReceiveStrategy();

        /// Only our subclass knows how to do this.
        virtual ssize_t receive_bytes(iovec          iov[],
                                      int            n,
                                      ACE_INET_Addr& remote_address) = 0;

        /// Called when there is a ReceivedDataSample to be delivered.
        virtual void deliver_sample(ReceivedDataSample&  sample,
                                    const ACE_INET_Addr& remote_address) = 0;

        /// Let the subclass start.
        virtual int start_i() = 0;

        /// Let the subclass stop.
        virtual void stop_i() = 0;

        /// Flag indicates if the GRACEFUL_DISCONNECT message is received.
        bool gracefully_disconnected_;

      private:

        /// Manage an index into the receive buffer array.
        size_t successor_index(size_t index) const;

        /// Bytes remaining in the current DataSample.
        size_t receive_sample_remaining_;

        /// Current receive TransportHeader.
        TransportHeader receive_transport_header_;

        //
        // Message Block Allocators are more plentiful since they hold samples
        // as well as data read from the handle(s).
        //
        enum { RECEIVE_BUFFERS  =    2 };
        enum { BUFFER_LOW_WATER = 1500 };
        enum { MESSAGE_BLOCKS   = 1000 };
        enum { DATA_BLOCKS      =  100 };

//MJM: We should probably bring the allocator typedefs down into this
//MJM: class since they are limited to this scope.
        TransportMessageBlockAllocator mb_allocator_;
        TransportDataBlockAllocator    db_allocator_;
        TransportDataAllocator         data_allocator_;

        /// Locking strategy for the allocators.
        ACE_Lock_Adapter<ACE_SYNCH_MUTEX> receive_lock_;

        /// Set of receive buffers in use.
        ACE_Message_Block* receive_buffers_[RECEIVE_BUFFERS];

        /// Current receive buffer index in use.
        size_t buffer_index_;

        /// Current data sample header.
        ReceivedDataSample receive_sample_;
    };

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "TransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY */
