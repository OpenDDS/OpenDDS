/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportHeader.h"
#include "dds/DCPS/Serializer.h"
#include "EntryExit.h"

const ACE_CDR::Octet
OpenDDS::DCPS::TransportHeader::DCPS_PROTOCOL[] =
  { 0x44, 0x43, 0x50, 0x53, 0x02, 0x00 };
//   D     C     P     S     |     |__ minor version
//                           |________ major version

#if !defined (__ACE_INLINE__)
# include "TransportHeader.inl"
#endif /* !__ACE_INLINE__ */

ACE_CDR::Boolean
operator<<(ACE_Message_Block& buffer, OpenDDS::DCPS::TransportHeader& value)
{
  using OpenDDS::DCPS::TransportHeader;
  DBG_ENTRY_LVL("TransportHeader","operator<<",6);

  OpenDDS::DCPS::Serializer writer(&buffer, value.swap_bytes());

  writer.write_octet_array(value.protocol_, sizeof(value.protocol_));

  const ACE_CDR::Octet flags =
    (value.byte_order_ << TransportHeader::BYTE_ORDER_FLAG) |
    (value.last_fragment_ << TransportHeader::LAST_FRAGMENT_FLAG);
  writer << ACE_OutputCDR::from_octet(flags);

  writer << ACE_OutputCDR::from_octet(value.reserved_);

  writer << value.length_;
  writer << value.sequence_;
  writer << value.source_;

  return writer.good_bit();
}

ACE_CDR::Boolean
operator<<(ACE_Message_Block*& buffer, OpenDDS::DCPS::TransportHeader& value)
{
  return *buffer << value;
}
