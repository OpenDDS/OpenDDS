// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTHEADER_H
#define TAO_DCPS_TRANSPORTHEADER_H

#include  "dds/DCPS/Definitions.h"


namespace TAO
{
  namespace DCPS
  {

    /**
     * @struct TransportHeader
     *
     * @brief Defines class that represents a transport packet header.
     *
     * The TransportHeader is the transport packet header.  Each packet
     * sent by the transport will always start with a transport packet
     * header, followed by one or more data samples (all belonging to the
     * same transport packet).
     */
    struct TAO_DdsDcps_Export TransportHeader
    {
        /// Default constructor.
        TransportHeader();

        /// Construct with values extracted from a buffer.
        TransportHeader(ACE_Message_Block* buffer);
        TransportHeader(ACE_Message_Block& buffer);

        /// Assignment from an ACE_Message_Block.
        TransportHeader& operator=(ACE_Message_Block* buffer);
        TransportHeader& operator=(ACE_Message_Block& buffer);

        /// Determine if this is a valid packet header.
        bool valid() const ;

        /// Byte order for the transport header. This byte_order_ flag indicates
        /// the endianess of the host of the publisher side. This is not affected
        /// by the swap_bytes configuration defined for the specific TransportImpl
        /// instance.
        ACE_CDR::Octet byte_order_;
        
        /// The protocol and version of the packet being transmitted.
        ACE_CDR::Octet packet_id_[6];
       
        /// The size of the message following this header, not including the
        /// 8 bytes used by this TransportHeader.
        ACE_UINT32 length_;

        /// Similar to IDL compiler generated methods.
        size_t max_marshaled_size() ;

      private:

        /// Demarshall transport packet from ACE_Message_Block.
        void init(ACE_Message_Block* buffer);

        /// Supported value of the packet ID.
        static const ACE_CDR::Octet supported_id_[ 6] ;
    };

  }
}

extern
ACE_CDR::Boolean
operator<<(ACE_Message_Block&, TAO::DCPS::TransportHeader& value);
extern
ACE_CDR::Boolean
operator<<(ACE_Message_Block*&, TAO::DCPS::TransportHeader& value);

extern
ACE_CDR::Boolean
operator<<(ACE_Message_Block&, TAO::DCPS::TransportHeader& value);
// TBD
//MJM: DUPLICATE declaration!(??)

#if defined(__ACE_INLINE__)
#include "TransportHeader.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTHEADER_H */
