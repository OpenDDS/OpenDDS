// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportHeader.h"
#include  "dds/DCPS/Serializer.h"
#include  "EntryExit.h"

const ACE_CDR::Octet
TAO::DCPS::TransportHeader::supported_id_[6]
  = { 0x44, 0x43, 0x50, 0x53, 0x01, 0x00 } ;
   //   D     C     P     S     1     0

#if !defined (__ACE_INLINE__)
# include "TransportHeader.inl"
#endif /* ! __ACE_INLINE__ */


// TBD
//MJM: I am still not sure why this is required by the compiler.  We
//MJM: should be able to just delegate to the other one.
ACE_CDR::Boolean
operator<<(ACE_Message_Block& buffer, TAO::DCPS::TransportHeader& value)
{
  DBG_SUB_ENTRY("TransportHeader","operator<<",1);

  TAO::DCPS::Serializer writer(&buffer);

  writer.write_octet_array( value.packet_id_, sizeof(value.packet_id_)) ;
  writer << value.length_;

  return writer.good_bit();
}


ACE_CDR::Boolean
operator<<(ACE_Message_Block*& buffer, TAO::DCPS::TransportHeader& value)
{
  DBG_SUB_ENTRY("TransportHeader","operator<<",2);

  TAO::DCPS::Serializer writer(buffer);

  writer.write_octet_array( value.packet_id_, sizeof(value.packet_id_)) ;
  writer << value.length_;

  return writer.good_bit();
}
