// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "GuidUtils.h"

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace OpenDDS { namespace DCPS {

GuidConverter::GuidConverter( GUID_t& guid)
  : guid_( guid),
    newGuid_( GUID_UNKNOWN)
{
  this->output_[0] = '\0';
}

GuidConverter::GuidConverter( GUID_t* guid)
  : guid_( *guid),
    newGuid_( GUID_UNKNOWN)
{
  this->output_[0] = '\0';
}

GuidConverter::GuidConverter( long federation, long participant)
  : guid_( this->newGuid_),
    newGuid_( GUID_UNKNOWN)
{
  this->output_[0] = '\0';
  this->guid_.guidPrefix[ 0] = VENDORID_OCI[0];
  this->guid_.guidPrefix[ 1] = VENDORID_OCI[1];

  this->guid_.guidPrefix[ 4] = static_cast<CORBA::Octet>((0xff&(federation>>24)));
  this->guid_.guidPrefix[ 5] = static_cast<CORBA::Octet>((0xff&(federation>>16)));
  this->guid_.guidPrefix[ 6] = static_cast<CORBA::Octet>((0xff&(federation>> 8)));
  this->guid_.guidPrefix[ 7] = static_cast<CORBA::Octet>((0xff& federation));
  this->guid_.guidPrefix[ 8] = static_cast<CORBA::Octet>((0xff&(participant>>24)));
  this->guid_.guidPrefix[ 9] = static_cast<CORBA::Octet>((0xff&(participant>>16)));
  this->guid_.guidPrefix[10] = static_cast<CORBA::Octet>((0xff&(participant>> 8)));
  this->guid_.guidPrefix[11] = static_cast<CORBA::Octet>((0xff& participant));
  this->guid_.entityId       = ENTITYID_UNKNOWN;
}

GuidConverter::operator GUID_t() const
{
  return this->guid_;
}

GuidConverter::operator long() const
{
  return ACE::crc32( reinterpret_cast<void*>(&this->guid_), sizeof( this->guid_));
}

GuidConverter::operator const char*() const
{
  if( ACE_OS::strlen( this->output_) == 0) {
    std::stringstream buffer;
    buffer << this->guid_ << "(" << std::hex << this->operator long() << ")";
    ACE_OS::strcpy( &this->output_[0], buffer.str().c_str());
  }
  return &this->output_[0];
}

long
GuidConverter::federationId() const
{
  return (this->guid_.guidPrefix[4]<<24)
       + (this->guid_.guidPrefix[5]<<16)
       + (this->guid_.guidPrefix[6]<<8)
       + (this->guid_.guidPrefix[7]);
}

long
GuidConverter::participantId() const
{
  return (this->guid_.guidPrefix[ 8]<<24)
       + (this->guid_.guidPrefix[ 9]<<16)
       + (this->guid_.guidPrefix[10]<<8)
       + (this->guid_.guidPrefix[11]);
}

long
GuidConverter::value() const
{
  return (this->guid_.entityId.entityKey[0]<<16) // or reinterpret and mask.
       + (this->guid_.entityId.entityKey[1]<<8)
       + (this->guid_.entityId.entityKey[2]);
}

long
GuidConverter::vendor() const
{
  return (this->guid_.guidPrefix[0]<<8) + this->guid_.guidPrefix[1];
}

EntityKind
GuidConverter::type() const
{
  switch( this->guid_.entityId.entityKind) {
    case ENTITYKIND_OPENDDS_TOPIC:
      return KIND_TOPIC;

    case ENTITYKIND_USER_READER_NO_KEY:
    case ENTITYKIND_USER_READER_WITH_KEY:
      return KIND_READER;

    case ENTITYKIND_USER_WRITER_NO_KEY:
    case ENTITYKIND_USER_WRITER_WITH_KEY:
      return KIND_WRITER;

    case 0xc1: // Participant Kind.
      if( this->value() == 1) return KIND_PARTICIPANT;

    case ENTITYKIND_USER_UNKNOWN:
    case ENTITYKIND_BUILTIN_UNKNOWN:
    default:
      return KIND_UNKNOWN;
  }
}

CORBA::Octet
GuidConverter::kind() const
{
  return this->guid_.entityId.entityKind;
}

CORBA::Octet&
GuidConverter::kind()
{
  this->output_[0] = '\0';
  return this->guid_.entityId.entityKind;
}

CORBA::Octet*
GuidConverter::key()
{
  this->output_[0] = '\0';
  return &this->guid_.entityId.entityKey[0];
}

bool
GuidConverter::isBuiltin() const
{
  // Builtin is 11xx_xxxx
  return ((0xc0 & this->guid_.entityId.entityKind) == 0xc0);
}

bool
GuidConverter::isUser() const
{
  // User is 00xx_xxxx
  return ((0xc0 & this->guid_.entityId.entityKind) == 0x00);
}

bool
GuidConverter::isVendor() const
{
  // Vendor is 01xx_xxxx
  return ((0xc0 & this->guid_.entityId.entityKind) == 0x40);
}

bool
GuidConverter::isKeyed() const
{
  return (((0x3f & this->guid_.entityId.entityKind) == ENTITYKIND_USER_WRITER_WITH_KEY)
       || ((0x3f & this->guid_.entityId.entityKind) == ENTITYKIND_USER_READER_WITH_KEY));
}

}} // End namespace OpenDDS::DCPS

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
  value.guidPrefix[ 0] = static_cast<CORBA::Octet>((word>>24)&0xff);
  value.guidPrefix[ 1] = static_cast<CORBA::Octet>((word>>16)&0xff);
  value.guidPrefix[ 2] = static_cast<CORBA::Octet>((word>>8)&0xff);
  value.guidPrefix[ 3] = static_cast<CORBA::Octet>(word&0xff);
  str >> discard;

  str >> std::hex >> word;
  value.guidPrefix[ 4] = static_cast<CORBA::Octet>((word>>24)&0xff);
  value.guidPrefix[ 5] = static_cast<CORBA::Octet>((word>>16)&0xff);
  value.guidPrefix[ 6] = static_cast<CORBA::Octet>((word>>8)&0xff);
  value.guidPrefix[ 7] = static_cast<CORBA::Octet>(word&0xff);
  str >> discard;

  str >> std::hex >> word;
  value.guidPrefix[ 8] = static_cast<CORBA::Octet>((word>>24)&0xff);
  value.guidPrefix[ 9] = static_cast<CORBA::Octet>((word>>16)&0xff);
  value.guidPrefix[10] = static_cast<CORBA::Octet>((word>>8)&0xff);
  value.guidPrefix[11] = static_cast<CORBA::Octet>(word&0xff);
  str >> discard;

  str >> std::hex >> word;
  value.entityId.entityKey[0] = static_cast<CORBA::Octet>((word>>24)&0xff);
  value.entityId.entityKey[1] = static_cast<CORBA::Octet>((word>>16)&0xff);
  value.entityId.entityKey[2] = static_cast<CORBA::Octet>((word>>8)&0xff);
  value.entityId.entityKind = static_cast<CORBA::Octet>(word&0xff);
  return str;
}

