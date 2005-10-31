/// -*- C++ -*-
///
/// $Id$
#ifndef TAO_DCPS_DATASAMPLEHEADER_H
#define TAO_DCPS_DATASAMPLEHEADER_H

#include  "Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{

  namespace DCPS
  {
    /// One byte message id (< 256)
    enum MessageId
    {
      SAMPLE_DATA,             
      DATAWRITER_LIVELINESS,
      INSTANCE_REGISTRATION,
      UNREGISTER_INSTANCE,
      DISPOSE_INSTANCE
    };

    /// The header message of a data sample.
    /// This header and the data sample are in different
    /// message block and will be chained together.
    struct TAO_DdsDcps_Export DataSampleHeader {
      /// The enum MessageId.
      char message_id_;

      /// The flag indicates the last sample of a group of 
      /// coherent changes.
      bool last_sample_ : 1;

      /// 0 -  Message encoded using little-endian byte order.
      /// 1 -  Message encoded using network byte order.
      bool byte_order_  : 1;

      /// reservered bits
      bool reserved_1   : 1;
      bool reserved_2   : 1;
      bool reserved_3   : 1;
      bool reserved_4   : 1;
      bool reserved_5   : 1;
      bool reserved_6   : 1;

      //The size of the message including the entire header.
      ACE_UINT32 message_length_;
      
      /// The sequence number is obtained from the Publisher 
      /// associated with the DataWriter based on the PRESENTATION 
      /// requirement for the sequence value (access_scope == GROUP). 
      ACE_INT16   sequence_;

      /// The SOURCE_TIMESTAMP field is generated from the DataWriter
      /// or supplied by the application at the time of the write.  
      /// This value is derived from the local hosts system clock, 
      /// which is assumed to be synchronized with the clocks on other 
      /// hosts within the domain.  This field is required for 
      /// DESTINATION_ORDER and LIFESPAN policy behaviors of subscriptions.
      /// It is also required to be present for all data in the 
      /// SampleInfo structure supplied along with each data sample.
      ACE_INT32 source_timestamp_sec_;
      ACE_INT32 source_timestamp_nanosec_;

      /// The COHERENCY_GROUP field is obtained from the Publisher as well, 
      /// in order to support the same scope as the SEQUENCE field.  
      /// The special value of 0 indicates that the sample is not 
      /// participating in a coherency group.
      ACE_UINT16  coherency_group_;

      /// Identify the DataWriter that produced the sample data being 
      /// sent.
      PublicationId  publication_id_;

      /// Default constructor.
      DataSampleHeader() ;

      /// Construct with values extracted from a buffer.
      DataSampleHeader (ACE_Message_Block* buffer) ;
      DataSampleHeader (ACE_Message_Block& buffer) ;

      /// Assignment from an ACE_Message_Block.
      DataSampleHeader& operator= (ACE_Message_Block* buffer) ;
      DataSampleHeader& operator= (ACE_Message_Block& buffer) ;

      /// Amount of data read when initializing from a buffer.
      size_t marshaled_size() ;

      /// Similar to IDL compiler generated methods.
      size_t max_marshaled_size() ;

      private:
        /// Implement load from buffer.
        void init( ACE_Message_Block* buffer) ;

        /// Keep track of the amount of data read from a buffer.
        size_t marshaled_size_ ;
    };

    /// Used to allocator the DataSampleHeader object.
    typedef Cached_Allocator_With_Overflow<DataSampleHeader, ACE_Null_Mutex>  
      DataSampleHeaderAllocator;

#if defined(__ACE_INLINE__)
#include "DataSampleHeader.inl"

#endif /* __ACE_INLINE__ */

  }  /* namespace DCPS */

}  /* namespace TAO */

/// Marshal/Insertion into a buffer.
extern TAO_DdsDcps_Export
ACE_CDR::Boolean
operator<< (ACE_Message_Block*&, TAO::DCPS::DataSampleHeader& value) ;

#endif  /* TAO_DCPS_DATASAMPLEHEADER_H */

