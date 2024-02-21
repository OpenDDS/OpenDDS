/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_COMMON_H
#define OPENDDS_DCPS_VALUE_COMMON_H

#include "XTypes/TypeObject.h"

#include <dds/Versioned_Namespace.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class EnumHelper {
public:
  virtual ~EnumHelper() {}
  virtual bool get_value(ACE_CDR::Long& value, const char* name) const = 0;
  virtual bool get_name(const char*& name, ACE_CDR::Long value) const = 0;
  virtual XTypes::TypeKind get_equivalent_int() const = 0;
};

class ListEnumHelper : public EnumHelper {
public:
  struct Pair {
    const char* name;
    ACE_CDR::Long value;
  };

  ListEnumHelper(XTypes::TypeKind as_int)
    : as_int_(as_int)
  {}

  ListEnumHelper(const OPENDDS_VECTOR(Pair)& pairs, XTypes::TypeKind as_int)
    : pairs_(pairs)
    , as_int_(as_int)
  {}

  void pairs(const OPENDDS_VECTOR(Pair)& pairs)
  {
    pairs_ = pairs;
  }

  bool get_value(ACE_CDR::Long& value, const char* name) const
  {
    for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs_.begin(); it != pairs_.end(); ++it) {
      if (std::strcmp(it->name, name) == 0) {
        value = it->value;
        return true;
      }
    }
    return false;
  }

  bool get_name(const char*& name, ACE_CDR::Long value) const
  {
    for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs_.begin(); it != pairs_.end(); ++it) {
      if (it->value == value) {
        name = it->name;
        return true;
      }
    }
    return false;
  }

  XTypes::TypeKind get_equivalent_int() const
  {
    return as_int_;
  }

private:
  OPENDDS_VECTOR(Pair) pairs_;
  XTypes::TypeKind as_int_;
};

class BitmaskHelper {
public:
  virtual ~BitmaskHelper() {}
  virtual bool get_value(ACE_CDR::ULongLong& value, const OPENDDS_VECTOR(const char*)& names) const = 0;
  virtual size_t get_name(OPENDDS_VECTOR(const char*)& names, ACE_CDR::ULongLong value) const = 0;
  virtual XTypes::TypeKind get_equivalent_uint() const = 0;
};

class MapBitmaskHelper : public BitmaskHelper {
public:
  struct Pair {
    const char* name;
    ACE_CDR::UShort position;
  };

  typedef OPENDDS_MAP(ACE_CDR::UShort, const char*) PosToNameMap;
  typedef OPENDDS_MAP(const char*, ACE_CDR::UShort) NameToPosMap;
  typedef PosToNameMap::const_iterator ptn_iterator;
  typedef NameToPosMap::const_iterator ntp_iterator;

  MapBitmaskHelper(XTypes::TypeKind as_uint)
    : bit_bound_(as_uint == XTypes::TK_UINT8 ? 8 :
                 (as_uint == XTypes::TK_UINT16 ? 16 :
                  (as_uint == XTypes::TK_UINT32 ? 32 :
                   (as_uint == XTypes::TK_UINT64 ? 64 : 0))))
    , as_uint_(as_uint)
  {}

  MapBitmaskHelper(const Pair* pairs, ACE_CDR::UShort bound, XTypes::TypeKind as_uint)
    : bit_bound_(bound)
    , as_uint_(as_uint)
  {
    for (const Pair* ptr = pairs; ptr->name; ++ptr) {
      pos_to_name_.insert(std::make_pair(ptr->position, ptr->name));
      name_to_pos_.insert(std::make_pair(ptr->name, ptr->position));
    }
  }

  void pairs(const OPENDDS_VECTOR(Pair)& pairs)
  {
    for (OPENDDS_VECTOR(Pair)::const_iterator it = pairs.begin(); it != pairs.end(); ++it) {
      pos_to_name_.insert(std::make_pair(it->position, it->name));
      name_to_pos_.insert(std::make_pair(it->name, it->position));
    }
  }

  void bit_bound(ACE_CDR::UShort bit_bound)
  {
    bit_bound_ = bit_bound;
  }

  bool get_value(ACE_CDR::ULongLong& value, const OPENDDS_VECTOR(const char*)& names) const
  {
    ACE_CDR::ULongLong rtn = 0;
    for (size_t i = 0; i < names.size(); ++i) {
      const ntp_iterator it = name_to_pos_.find(names.at(i));
      if (it == name_to_pos_.end()) {
        return false;
      }
      rtn |= 1 << it->position;
    }
    value = rtn;
    return true;
  }

  // Return an estimated length of a string constructed by the returned flag names
  // with a delimiter character between two consecutive flags.
  size_t get_names(OPENDDS_VECTOR(const char*)& names, ACE_CDR::ULongLong value) const
  {
    size_t rtn_size = 0;
    OPENDDS_VECTOR(const char*) rtn;
    for (ACE_CDR::UShort i = 0; i < bit_bound_; ++i) {
      const ptn_iterator it = pos_to_name_.find(i);
      if ((it != pos_to_name_.end()) && (value & 1 << i)) {
        rtn.push_back(it->name);
        rtn_size += std::strlen(it->name) + 1; // +1 for a delimiter like a pipe ('|') character
      }
    }
    names = rtn;
    return rtn_size;
  }

  XTypes::TypeKind get_equivalent_uint() const
  {
    return as_uint_;
  }

private:
  PosToNameMap pos_to_name_;
  NameToPosMap name_to_pos_;
  ACE_CDR::UShort bit_bound_;
  XTypes::TypeKind as_uint_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_VALUE_COMMON_H */
