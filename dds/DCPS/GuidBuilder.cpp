/*
 * $Id$
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "GuidBuilder.h"

namespace
{
inline void
fill_guid(CORBA::Octet* begin, long value, std::size_t len)
{
  for (std::size_t i = 0; i < len; ++i)
  {
    unsigned shift = (len - i - 1) << 3;
    begin[i] = static_cast<CORBA::Octet>((0xff & (value >> shift)));
  }
}

} // namespace

namespace OpenDDS
{
namespace DCPS
{
GuidBuilder::GuidBuilder(GUID_t& guid)
  : guid_(guid)
{
  guid_.guidPrefix[0] = VENDORID_OCI[0];
  guid_.guidPrefix[1] = VENDORID_OCI[1];
}

GuidBuilder::~GuidBuilder()
{
}

void
GuidBuilder::guidPrefix0(long p0)
{
  fill_guid(guid_.guidPrefix, p0, 4);
}

void
GuidBuilder::guidPrefix1(long p1)
{
  fill_guid(guid_.guidPrefix + 4, p1, 4);
}

void
GuidBuilder::guidPrefix2(long p2)
{
  fill_guid(guid_.guidPrefix + 8, p2, 4);
}

void
GuidBuilder::entityId(EntityId_t entityId)
{
  guid_.entityId = entityId;
}

void
GuidBuilder::entityId(long entityId)
{
  entityKey(entityId >> 8);
  entityKind(static_cast<CORBA::Octet>(0xff & entityId));
}

void
GuidBuilder::entityKey(long entityKey)
{
  fill_guid(guid_.entityId.entityKey, entityKey, 3);
}

void
GuidBuilder::entityKind(CORBA::Octet entityKind)
{
  guid_.entityId.entityKind = entityKind;
}

void
GuidBuilder::entityKind(EntityKind kind)
{
  switch (kind)
  {
  case KIND_WRITER:
    guid_.entityId.entityKind =
      ENTITYKIND_USER_WRITER_WITH_KEY;
    break;

  case KIND_READER:
    guid_.entityId.entityKind =
      ENTITYKIND_USER_READER_WITH_KEY;
    break;

  case KIND_TOPIC:
    guid_.entityId.entityKind =
      ENTITYKIND_OPENDDS_TOPIC;
    break;

  default:
    guid_.entityId.entityKind =
      ENTITYKIND_USER_UNKNOWN;
  }
}

} // namespace
} // namespace:
