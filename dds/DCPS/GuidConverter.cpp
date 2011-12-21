/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include <iomanip>
#include <sstream>

#include "ace/ACE.h"

#include "GuidConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"

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
    return KIND_TOPIC;

  case ENTITYKIND_USER_READER_NO_KEY:
  case ENTITYKIND_USER_READER_WITH_KEY:
    return KIND_READER;

  case ENTITYKIND_USER_WRITER_NO_KEY:
  case ENTITYKIND_USER_WRITER_WITH_KEY:
  case ENTITYKIND_OPENDDS_NIL_WRITER:
    return KIND_WRITER;

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

GuidConverter::operator std::string() const
{
  std::ostringstream os;

  os << guid_ << "(" << std::hex << checksum() << ")";

  return os.str();
}

#ifdef DDS_HAS_WCHAR
GuidConverter::operator std::wstring() const
{
  std::wostringstream os;

  os << guid_ << "(" << std::hex << checksum() << ")";

  return os.str();
}
#endif

std::ostream&
operator<<(std::ostream& os, const GuidConverter& rhs)
{
  return os << std::string(rhs);
}

#ifdef DDS_HAS_WCHAR
std::wostream&
operator<<(std::wostream& os, const GuidConverter& rhs)
{
  return os << std::wstring(rhs);
}
#endif

std::string
GuidConverter::uniqueId() const
{
  char id[64];
  sprintf(id, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
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

