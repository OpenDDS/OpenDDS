/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"

#include "PoolAllocator.h"

#include <ace/Log_Msg.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS_Dcps_Export unsigned int OpenDDS::DCPS::DCPS_debug_level = 0;
#ifdef OPENDDS_SECURITY
OpenDDS_Dcps_Export OpenDDS::DCPS::SecurityDebug OpenDDS::DCPS::security_debug;
#endif

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export void set_DCPS_debug_level(unsigned int lvl)
{
  OpenDDS::DCPS::DCPS_debug_level = lvl;
}

#ifdef OPENDDS_SECURITY
SecurityDebug::SecurityDebug()
  : fake_encryption(false)
{
  set_all_flags_to(false);
}

void
SecurityDebug::set_all_flags_to(bool value)
{
  encdec_error = value;
  encdec_warn = value;
  encdec_debug = value;
  auth_debug = value;
  auth_warn = value;
  access_warn = value;
  bookkeeping = value;
  showkeys = value;
  chlookup = value;
}

void
SecurityDebug::parse_flags(const ACE_TCHAR* flags)
{
  OPENDDS_STRING s(ACE_TEXT_ALWAYS_CHAR(flags));
  const OPENDDS_STRING delim(",");
  while (true) {
    const size_t pos = s.find(delim);
    const OPENDDS_STRING flag = s.substr(0, pos);
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
      } else if (flag == "access_warn") {
        access_warn = true;
      } else if (flag == "bookkeeping") {
        bookkeeping = true;
      } else if (flag == "showkeys") {
        showkeys = true;
      } else if (flag == "chlookup") {
        chlookup = true;
      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) SecurityDebug::parse_flags: ")
          ACE_TEXT("Unknown Security Debug Category: \"%C\"\n"), flag.c_str()));
      }
    }
    if (pos == OPENDDS_STRING::npos) {
      break;
    }
    s.erase(0, pos + delim.length());
  }
}

void
SecurityDebug::set_debug_level(unsigned level)
{
  access_warn = level >= 1;
  auth_warn = encdec_error = level >= 3;
  auth_debug = encdec_warn = bookkeeping = level >= 4;
  encdec_debug = level >= 8;
  showkeys = level >= 9;
  chlookup = level >= 10;
}
#endif

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
