/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportHeader.h"
#include "dds/DCPS/Serializer.h"
#include "EntryExit.h"

const ACE_CDR::Octet
OpenDDS::DCPS::TransportHeader::supported_id_[6] =
  { 0x44, 0x43, 0x50, 0x53, 0x01, 0x00 } ;
//   D     C     P     S     1     0

#if !defined (__ACE_INLINE__)
# include "TransportHeader.inl"
#endif /* !__ACE_INLINE__ */

// TBD
//MJM: I am still not sure why this is required by the compiler.  We
//MJM: should be able to just delegate to the other one.
ACE_CDR::Boolean
operator<<(ACE_Message_Block& buffer, OpenDDS::DCPS::TransportHeader& value)
{
  DBG_ENTRY_LVL("TransportHeader","operator<<",6);

  TAO::DCPS::Serializer writer(&buffer, value.byte_order_ != TAO_ENCAP_BYTE_ORDER);
  writer << ACE_OutputCDR::from_octet(value.byte_order_);

  writer.write_octet_array(value.packet_id_, sizeof(value.packet_id_)) ;
  writer << value.sequence_;
  writer << value.length_;

  return writer.good_bit();
}

ACE_CDR::Boolean
operator<<(ACE_Message_Block*& buffer, OpenDDS::DCPS::TransportHeader& value)
{
  DBG_ENTRY_LVL("TransportHeader","operator<<",6);

  TAO::DCPS::Serializer writer(buffer, value.byte_order_ != TAO_ENCAP_BYTE_ORDER);
  writer << ACE_OutputCDR::from_octet(value.byte_order_);

  writer.write_octet_array(value.packet_id_, sizeof(value.packet_id_)) ;
  writer << value.sequence_;
  writer << value.length_;

  return writer.good_bit();
}
