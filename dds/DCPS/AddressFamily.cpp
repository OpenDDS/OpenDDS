/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "AddressFamily.h"

#include "ConfigStoreImpl.h"
#include "Service_Participant.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

AddressFamily
get_address_family(const char* config_key)
{
  const String value = TheServiceParticipant->config_store()->get(
    config_key, address_family_name(default_address_family()));
  AddressFamily result;
  return parse_address_family(value.c_str(), result) ? result :
    ADDRESS_FAMILY_IPV4;
}

bool
set_address_family(const char* config_key, AddressFamily value)
{
  if (!address_family_supported(value)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: set_address_family: "
                 "for %C, unsupported AddressFamily configuration: %C\n",
                 config_key, address_family_name(value)));
    }
    return false;
  }
  TheServiceParticipant->config_store()->set(
    config_key, address_family_name(value));
  return true;
}

bool
set_address_family(const char* config_key, const char* value)
{
  AddressFamily result;
  if (parse_address_family(value, result)) {
    return set_address_family(config_key, result);
  }
  if (log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: set_address_family: "
               "for %C, invalid AddressFamily configuration: %C\n",
               config_key, value));
  }
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
