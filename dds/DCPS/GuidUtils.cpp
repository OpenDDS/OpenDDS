/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DCPS/SafetyProfileStreams.h"

#include <cstdlib>

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include "GuidBuilder.h"

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

namespace OpenDDS { namespace DCPS {

OPENDDS_STRING
to_string(const GUID_t& guid)
{
  std::size_t len;
  std::size_t tot_len;

  tot_len = sizeof(guid.guidPrefix) / sizeof(CORBA::Octet) +
        (((sizeof(guid.guidPrefix) / sizeof(CORBA::Octet)) / 4) * sizeof(".")) +
        sizeof(guid.entityId.entityKey) / sizeof(CORBA::Octet) +
        sizeof(CORBA::Octet);

  char buffer[tot_len];
  char* cur_char = buffer;

  len = sizeof(guid.guidPrefix) / sizeof(CORBA::Octet);

  for (std::size_t i = 0; i < len; ++i) {
    cur_char += sprintf(cur_char, "%02x", unsigned(guid.guidPrefix[i]));

    if ((i + 1) % 4 == 0) {
      cur_char += sprintf(cur_char, ".");
    }
  }

  len = sizeof(guid.entityId.entityKey) / sizeof(CORBA::Octet);

  for (std::size_t i = 0; i < len; ++i) {
    cur_char += sprintf(cur_char, "%02x", unsigned(guid.entityId.entityKey[i]));
  }
  cur_char += sprintf(cur_char, "%02x", unsigned(guid.entityId.entityKind));

  OPENDDS_STRING ret(buffer);

  return ret;
}

#ifndef OPENDDS_SAFETY_PROFILE
std::ostream&
operator<<(std::ostream& os, const GUID_t& rhs)
{
  std::size_t len;

  len = sizeof(rhs.guidPrefix) / sizeof(CORBA::Octet);

  os << std::hex;

  for (std::size_t i = 0; i < len; ++i) {
    os << setopts << unsigned(rhs.guidPrefix[i]);

    if ((i + 1) % 4 == 0) os << sep;
  }

  len = sizeof(rhs.entityId.entityKey) / sizeof(CORBA::Octet);

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
