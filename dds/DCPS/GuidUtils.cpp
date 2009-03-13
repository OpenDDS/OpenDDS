/*
 * $Id$
 */

#include <iostream>
#include <iomanip>

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include "GuidUtils.h"

std::ostream&
operator<<( std::ostream& str, const OpenDDS::DCPS::GUID_t& value)
{
  const CORBA::Octet* octets = reinterpret_cast<const CORBA::Octet*>( &value);
  for( unsigned int index = 0; index < sizeof(value); ++index) {
    if( index>0 && index%4 == 0) str << ".";
    unsigned short byte = octets[index];
    str << std::hex << std::setfill('0') << std::setw(2) << byte;
  }
  return str;
}

std::istream&
operator>>( std::istream& str, OpenDDS::DCPS::GUID_t& value)
{
  // Brute force read.
  char discard;
  unsigned long word;

  str >> std::hex >> word;
  OpenDDS::DCPS::fill_guid(value.guidPrefix, word, 4);
  str >> discard;

  str >> std::hex >> word;
  OpenDDS::DCPS::fill_guid(value.guidPrefix + 4, word, 4);
  str >> discard;

  str >> std::hex >> word;
  OpenDDS::DCPS::fill_guid(value.guidPrefix + 8, word, 4);
  str >> discard;

  str >> std::hex >> word;
  OpenDDS::DCPS::fill_guid(value.entityId.entityKey, word, 3);
  OpenDDS::DCPS::fill_guid(&value.entityId.entityKind, word);
  
  return str;
}
