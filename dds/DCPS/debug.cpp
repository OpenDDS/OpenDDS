/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "debug.h"
#include "Util.h"

#ifdef OPENDDS_SECURITY
#include "PoolAllocator.h"
#endif

#include <ace/Log_Msg.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export LogLevel log_level(LogLevel::Warning);
OpenDDS_Dcps_Export unsigned int DCPS_debug_level = 0;
#ifdef OPENDDS_SECURITY
OpenDDS_Dcps_Export SecurityDebug security_debug;
#endif

void LogLevel::set(LogLevel::Value value)
{
  level_ = value;
#ifdef OPENDDS_SECURITY
  if (level_ >= Notice) {
    security_debug.set_debug_level(1);
  } else {
    security_debug.set_all_flags_to(false);
  }
#endif
  if (level_ >= Debug) {
    if (DCPS_debug_level == 0) {
      DCPS_debug_level = 1;
    }
  } else {
    DCPS_debug_level = 0;
#ifndef OPENDDS_UTIL_BUILD
    Transport_debug_level = 0;
    transport_debug = TransportDebug();
#endif
  }
}

namespace {
  struct LogLevelNameValue {
    const char* const name;
    const LogLevel::Value value;
  };
  static const LogLevelNameValue log_levels[] = {
    {"none", LogLevel::None},
    {"error", LogLevel::Error},
    {"warning", LogLevel::Warning},
    {"notice", LogLevel::Notice},
    {"info", LogLevel::Info},
    {"debug", LogLevel::Debug}
  };
};

void LogLevel::set_from_string(const char* name)
{
  for (size_t i = 0; i < array_count(log_levels); ++i) {
    if (!std::strcmp(log_levels[i].name, name)) {
      set(log_levels[i].value);
      return;
    }
  }
  if (log_level >= Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LogLevel::set_from_string: "
      "Invalid log level name: %C\n", name));
  }
}

const char* LogLevel::get_as_string() const
{
  const unsigned index = static_cast<unsigned>(get());
  if (index >= array_count(log_levels)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LogLevel::get_as_string: "
      "Invalid log level value: %u\n", index));
    return "invalid";
  }
  return log_levels[index].name;
}

OpenDDS_Dcps_Export void set_DCPS_debug_level(unsigned int lvl)
{
  if (log_level.get() < LogLevel::Debug) {
    log_level.set(LogLevel::Debug);
  }
  if (log_level >= LogLevel::Info) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: set_DCPS_debug_level: set to %u\n", lvl));
  }
  DCPS_debug_level = lvl;
}

#ifdef OPENDDS_SECURITY
SecurityDebug::SecurityDebug()
  : fake_encryption(false)
  , force_auth_role(FORCE_AUTH_ROLE_NORMAL)
{
  set_all_flags_to(false);
}

void SecurityDebug::set_all_flags_to(bool value)
{
  encdec_error = value;
  encdec_warn = value;
  encdec_debug = value;
  auth_debug = value;
  auth_warn = value;
  new_entity_error = value;
  new_entity_warn = value;
  cleanup_error = value;
  access_error = value;
  access_warn = value;
  bookkeeping = value;
  showkeys = value;
  chlookup = value;
}

void SecurityDebug::parse_flags(const ACE_TCHAR* flags)
{
  String s(ACE_TEXT_ALWAYS_CHAR(flags));
  const String delim(",");
  while (true) {
    const size_t pos = s.find(delim);
    const String flag = s.substr(0, pos);
    if (flag.length()) {
      if (flag == "all") {
        set_all_flags_to(true);
      } else if (flag == "encdec_error") {
        encdec_error = true;
      } else if (flag == "encdec_warn") {
        encdec_warn = true;
      } else if (flag == "encdec_debug") {
        encdec_debug = true;
      } else if (flag == "auth_debug") {
        auth_debug = true;
      } else if (flag == "auth_warn") {
        auth_warn = true;
      } else if (flag == "new_entity_error") {
        new_entity_error = true;
      } else if (flag == "new_entity_warn") {
        new_entity_warn = true;
      } else if (flag == "cleanup_error") {
        cleanup_error = true;
      } else if (flag == "access_error") {
        access_error = true;
      } else if (flag == "access_warn") {
        access_warn = true;
      } else if (flag == "bookkeeping") {
        bookkeeping = true;
      } else if (flag == "showkeys") {
        showkeys = true;
      } else if (flag == "chlookup") {
        chlookup = true;
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: SecurityDebug::parse_flags: "
          "Unknown Security Debug Category: \"%C\"\n", flag.c_str()));
      }
    }
    if (pos == String::npos) {
      break;
    }
    s.erase(0, pos + delim.length());
  }
}

void SecurityDebug::set_debug_level(unsigned level)
{
  access_error = new_entity_error = cleanup_error = level >= 1;
  access_warn = level >= 2;
  auth_warn = encdec_error = level >= 3;
  new_entity_warn = level >= 3;
  auth_debug = encdec_warn = bookkeeping = level >= 4;
  encdec_debug = level >= 8;
  showkeys = level >= 9;
  chlookup = level >= 10;
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
