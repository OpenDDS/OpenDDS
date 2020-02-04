/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "GuidUtils.h"
#include "Util.h"
#include "GuidBuilder.h"
#include "SafetyProfileStreams.h"

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include <cstdlib>
#include <cstdio>

namespace {

#ifndef OPENDDS_SAFETY_PROFILE
inline std::ostream&
sep(std::ostream& os)
{
  return os << ".";
}

inline std::ostream&
setopts(std::ostream& os)
{
  return os << std::setfill('0') << std::setw(2);
}
#endif

} // namespace

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace DCPS {

OPENDDS_STRING
to_string(const EntityId_t& entityId)
{
  return to_hex_dds_string(&entityId.entityKey[0], sizeof(EntityKey_t)) +
    to_dds_string(unsigned(entityId.entityKind), true);
}

OPENDDS_STRING
to_string(const GUID_t& guid)
{
  return to_hex_dds_string(&guid.guidPrefix[0], sizeof(GuidPrefix_t), '.', 4) +
    '.' + to_string(guid.entityId);
}

void intersect(const RepoIdSet& a, const RepoIdSet& b, RepoIdSet& result)
{
  result.clear();
  intersect_sorted_ranges(a.begin(), a.end(), b.begin(), b.end(),
                          std::inserter(result, result.end()), GUID_tKeyLessThan());
}

#ifndef OPENDDS_SAFETY_PROFILE
std::ostream&
operator<<(std::ostream& os, const GUID_t& rhs)
{
  std::size_t len = sizeof(rhs.guidPrefix);

  os << std::hex;

  for (std::size_t i = 0; i < len; ++i) {
    os << setopts << unsigned(rhs.guidPrefix[i]);

    if ((i + 1) % 4 == 0) os << sep;
  }

  len = sizeof(rhs.entityId.entityKey);

  for (std::size_t i = 0; i < len; ++i) {
    os << setopts << unsigned(rhs.entityId.entityKey[i]);
  }

  os << setopts << unsigned(rhs.entityId.entityKind);

  // Reset, because hex is "sticky"
  os << std::dec;

  return os;
}

std::istream&
operator>>(std::istream& is, GUID_t& rhs)
{
  long word;
  char discard;

  GuidBuilder builder(rhs);

  is >> std::hex >> word;
  builder.guidPrefix0(word);
  is >> discard; // sep

  is >> std::hex >> word;
  builder.guidPrefix1(word);
  is >> discard; // sep

  is >> std::hex >> word;
  builder.guidPrefix2(word);
  is >> discard; // sep

  is >> std::hex >> word;
  builder.entityId(word);

  return is;
}
#endif

}  }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
