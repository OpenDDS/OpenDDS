/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "ValueCommon.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool ListEnumHelper::get_value(ACE_CDR::Long& value, const char* name) const
{
  for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs_.begin(); it != pairs_.end(); ++it) {
    if (std::strcmp(it->name, name) == 0) {
      value = it->value;
      return true;
    }
  }
  return false;
}

bool ListEnumHelper::get_name(const char*& name, ACE_CDR::Long value) const
{
  for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs_.begin(); it != pairs_.end(); ++it) {
    if (it->value == value) {
      name = it->name;
      return true;
    }
  }
  return false;
}

MapBitmaskHelper::MapBitmaskHelper(const Pair* pairs, ACE_CDR::UShort bit_bound, XTypes::TypeKind as_uint)
  : bit_bound_(bit_bound)
  , as_uint_(as_uint)
{
  for (const Pair* ptr = pairs; ptr->name; ++ptr) {
    pos_to_name_.insert(std::make_pair(ptr->position, ptr->name));
    name_to_pos_.insert(std::make_pair(ptr->name, ptr->position));
  }
}

void MapBitmaskHelper::pairs(const OPENDDS_VECTOR(Pair)& pairs)
{
  for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs.begin(); it != pairs.end(); ++it) {
    pos_to_name_.insert(std::make_pair(it->position, it->name));
    name_to_pos_.insert(std::make_pair(it->name, it->position));
  }
}

bool MapBitmaskHelper::get_value(ACE_CDR::ULongLong& value, const OPENDDS_VECTOR(const char*)& names) const
{
  ACE_CDR::ULongLong rtn = 0;
  for (size_t i = 0; i < names.size(); ++i) {
    const ntp_iterator it = name_to_pos_.find(names.at(i));
    if (it == name_to_pos_.end()) {
      return false;
    }
    rtn |= 1ull << it->second;
  }
  value = rtn;
  return true;
}

size_t MapBitmaskHelper::get_names(OPENDDS_VECTOR(const char*)& names, ACE_CDR::ULongLong value) const
{
  size_t rtn_size = 0;
  OPENDDS_VECTOR(const char*) rtn;
  for (ACE_CDR::UShort i = 0; i < bit_bound_; ++i) {
    const ptn_iterator it = pos_to_name_.find(i);
    if ((it != pos_to_name_.end()) && (value & 1ull << i)) {
      rtn.push_back(it->second);
      rtn_size += std::strlen(it->second) + 1; // +1 for a delimiter like a pipe ('|') character
    }
  }
  names = rtn;
  return rtn_size;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
