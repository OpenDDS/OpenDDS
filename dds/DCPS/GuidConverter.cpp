/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "GuidConverter.h"

#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <ace/ACE.h>
#include <ace/OS_NS_stdio.h>

#ifdef DDS_HAS_WCHAR
#  include <sstream>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

GuidConverter::GuidConverter(const GUID_t& guid)
  : guid_(guid)
{}

GuidConverter::~GuidConverter()
{}

long
GuidConverter::checksum() const
{
  return ACE::crc32(reinterpret_cast<const void*>(&guid_), sizeof(guid_));
}

long
GuidConverter::vendorId() const
{
  return guid_.guidPrefix[0] << 8
         | guid_.guidPrefix[1];
}

long
GuidConverter::entityId() const
{
  return entityKey() << 8
         | guid_.entityId.entityKind;
}

long
GuidConverter::entityKey() const
{
  return guid_.entityId.entityKey[0] << 16
         | guid_.entityId.entityKey[1] << 8
         | guid_.entityId.entityKey[2];
}

EntityKind
GuidConverter::entityKind() const
{
  switch (guid_.entityId.entityKind) {
  case ENTITYKIND_OPENDDS_TOPIC:
    return KIND_USER_TOPIC;

  case ENTITYKIND_BUILTIN_TOPIC:
    return KIND_BUILTIN_TOPIC;

  case ENTITYKIND_USER_READER_NO_KEY:
  case ENTITYKIND_USER_READER_WITH_KEY:
    return KIND_USER_READER;

  case ENTITYKIND_USER_WRITER_NO_KEY:
  case ENTITYKIND_USER_WRITER_WITH_KEY:
  case ENTITYKIND_OPENDDS_NIL_WRITER:
    return KIND_USER_WRITER;

  case ENTITYKIND_BUILTIN_READER_NO_KEY:
  case ENTITYKIND_BUILTIN_READER_WITH_KEY:
    return KIND_BUILTIN_READER;

  case ENTITYKIND_BUILTIN_WRITER_NO_KEY:
  case ENTITYKIND_BUILTIN_WRITER_WITH_KEY:
    return KIND_BUILTIN_WRITER;

  case ENTITYKIND_BUILTIN_PARTICIPANT:
    return KIND_PARTICIPANT;

  case ENTITYKIND_OPENDDS_PUBLISHER:
    return KIND_PUBLISHER;

  case ENTITYKIND_OPENDDS_SUBSCRIBER:
    return KIND_SUBSCRIBER;

  case ENTITYKIND_OPENDDS_USER:
    return KIND_USER;

  case ENTITYKIND_USER_UNKNOWN:
  case ENTITYKIND_BUILTIN_UNKNOWN:
  default:
    return KIND_UNKNOWN;
  }
}

bool GuidConverter::isBuiltinDomainEntity() const
{
  switch (guid_.entityId.entityKind) {

  case ENTITYKIND_BUILTIN_READER_NO_KEY:
  case ENTITYKIND_BUILTIN_READER_WITH_KEY:
  case ENTITYKIND_BUILTIN_WRITER_NO_KEY:
  case ENTITYKIND_BUILTIN_WRITER_WITH_KEY:
  case ENTITYKIND_BUILTIN_TOPIC:
    return true;

  default:
    return false;
  }
}

bool GuidConverter::isUserDomainEntity() const
{
  switch (guid_.entityId.entityKind) {

  case ENTITYKIND_USER_READER_NO_KEY:
  case ENTITYKIND_USER_READER_WITH_KEY:
  case ENTITYKIND_USER_WRITER_NO_KEY:
  case ENTITYKIND_USER_WRITER_WITH_KEY:
  case ENTITYKIND_OPENDDS_TOPIC:
    return true;

  default:
    return false;
  }
}

bool GuidConverter::isWriter() const
{
  EntityKind kind = entityKind();
  return kind == KIND_USER_WRITER || kind == KIND_BUILTIN_WRITER;
}

bool GuidConverter::isReader() const
{
  EntityKind kind = entityKind();
  return kind == KIND_USER_READER || kind == KIND_BUILTIN_READER;
}

bool GuidConverter::isTopic() const
{
  EntityKind kind = entityKind();
  return kind == KIND_USER_TOPIC || kind == KIND_BUILTIN_TOPIC;
}

GuidConverter::operator OPENDDS_STRING() const
{
  OPENDDS_STRING ret(to_string(guid_));
  ret += "(";
  ret += to_dds_string((unsigned long) checksum(), true);
  ret += ")";
  return ret;
}

#ifdef DDS_HAS_WCHAR
GuidConverter::operator std::wstring() const
{
  std::wostringstream os;
  const OPENDDS_STRING guid_str(to_string(guid_));
  os << guid_str.c_str() << "(" << std::hex << checksum() << ")";
  return os.str();
}
#endif

#ifndef OPENDDS_SAFETY_PROFILE

std::ostream&
operator<<(std::ostream& os, const GuidConverter& rhs)
{
  return os << OPENDDS_STRING(rhs);
}

#ifdef DDS_HAS_WCHAR
std::wostream&
operator<<(std::wostream& os, const GuidConverter& rhs)
{
  return os << std::wstring(rhs);
}
#endif //DDS_HAS_WCHAR
#endif //OPENDDS_SAFETY_PROFILE

OPENDDS_STRING
GuidConverter::uniqueParticipantId() const
{
  char id[64];
  ACE_OS::snprintf(id, sizeof id,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          guid_.guidPrefix[ 0],
          guid_.guidPrefix[ 1],
          guid_.guidPrefix[ 2],
          guid_.guidPrefix[ 3],
          guid_.guidPrefix[ 4],
          guid_.guidPrefix[ 5],
          guid_.guidPrefix[ 6],
          guid_.guidPrefix[ 7],
          guid_.guidPrefix[ 8],
          guid_.guidPrefix[ 9],
          guid_.guidPrefix[10],
          guid_.guidPrefix[11]);
  return id;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
