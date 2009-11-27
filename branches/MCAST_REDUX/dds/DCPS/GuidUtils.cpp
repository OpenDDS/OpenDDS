/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include "GuidBuilder.h"

namespace {

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

} // namespace

std::ostream&
operator<<(std::ostream& os, const OpenDDS::DCPS::GUID_t& rhs)
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
operator>>(std::istream& is, OpenDDS::DCPS::GUID_t& rhs)
{
  long word;
  char discard;

  OpenDDS::DCPS::GuidBuilder builder(rhs);

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
