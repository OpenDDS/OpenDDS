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

const ACE_INT32
OpenDDS::DCPS::TransportHeader::DCPS_PROTOCOL(0x44435053); // DCPS

const ACE_INT32
OpenDDS::DCPS::TransportHeader::DCPS_PROTOCOL_SWAPPED(0x53504344);  // SPCD

const ACE_CDR::Octet
OpenDDS::DCPS::TransportHeader::DCPS_VERSION_MAJOR(0x02);

const ACE_CDR::Octet
OpenDDS::DCPS::TransportHeader::DCPS_VERSION_MINOR(0x00);

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

  TAO::DCPS::Serializer writer(&buffer);

  writer << value.protocol_;
  writer << ACE_OutputCDR::from_octet(value.version_major_);
  writer << ACE_OutputCDR::from_octet(value.version_minor_);
  writer << value.source_;
  writer << value.sequence_;
  writer << value.length_;

  return writer.good_bit();
}

ACE_CDR::Boolean
operator<<(ACE_Message_Block*& buffer, OpenDDS::DCPS::TransportHeader& value)
{
  DBG_ENTRY_LVL("TransportHeader","operator<<",6);

  TAO::DCPS::Serializer writer(buffer);

  writer << value.protocol_;
  writer << ACE_OutputCDR::from_octet(value.version_major_);
  writer << ACE_OutputCDR::from_octet(value.version_minor_);
  writer << value.source_;
  writer << value.sequence_;
  writer << value.length_;

  return writer.good_bit();
}
