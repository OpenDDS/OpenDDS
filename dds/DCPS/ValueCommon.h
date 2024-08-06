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

class OpenDDS_Dcps_Export EnumHelper {
public:
  virtual ~EnumHelper() {}
  virtual bool valid(const char* name) const = 0;
  virtual bool valid(ACE_CDR::Long value) const = 0;
  virtual bool get_value(ACE_CDR::Long& value, const char* name) const = 0;
  virtual bool get_name(const char*& name, ACE_CDR::Long value) const = 0;
  virtual XTypes::TypeKind get_equivalent_int() const = 0;

  ACE_CDR::Long get_value(const char* name) const;
  const char* get_name(ACE_CDR::Long value) const;
};

class OpenDDS_Dcps_Export ListEnumHelper : public EnumHelper {
public:
  struct Pair {
    const char* name;
    ACE_CDR::Long value;
  };

  explicit ListEnumHelper(XTypes::TypeKind as_int)
    : as_int_(as_int)
  {}

  explicit ListEnumHelper(const Pair* pairs, XTypes::TypeKind as_int = XTypes::TK_INT32)
    : as_int_(as_int)
  {
    for (const Pair* ptr = pairs; ptr->name; ++ptr) {
      pairs_.push_back(*ptr);
    }
  }

  void pairs(const OPENDDS_VECTOR(Pair)& pairs)
  {
    pairs_ = pairs;
  }

  bool valid(const char* name) const;
  bool valid(ACE_CDR::Long value) const;
  bool get_value(ACE_CDR::Long& value, const char* name) const;
  bool get_name(const char*& name, ACE_CDR::Long value) const;

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
  virtual bool get_value(ACE_CDR::ULongLong& value, const OPENDDS_VECTOR(String)& names) const = 0;

  // Return an estimated length of a string constructed by the returned flag names
  // with a delimiter character between two consecutive flags.
  virtual size_t get_names(OPENDDS_VECTOR(String)& names, ACE_CDR::ULongLong value) const = 0;

  virtual XTypes::TypeKind get_equivalent_uint() const = 0;
};

class OpenDDS_Dcps_Export MapBitmaskHelper : public BitmaskHelper {
public:
  struct Pair {
    const char* name;
    ACE_CDR::UShort position;
  };

  typedef OPENDDS_MAP(ACE_CDR::UShort, String) PosToNameMap;
  typedef OPENDDS_MAP(String, ACE_CDR::UShort) NameToPosMap;
  typedef PosToNameMap::const_iterator ptn_iterator;
  typedef NameToPosMap::const_iterator ntp_iterator;

  explicit MapBitmaskHelper(XTypes::TypeKind as_uint)
    : bit_bound_(32)
    , as_uint_(as_uint)
  {}

  MapBitmaskHelper(ACE_CDR::UShort bit_bound, XTypes::TypeKind as_uint)
    : bit_bound_(bit_bound)
    , as_uint_(as_uint)
  {}

  MapBitmaskHelper(const Pair* pairs, ACE_CDR::UShort bit_bound, XTypes::TypeKind as_uint);
  void pairs(const OPENDDS_VECTOR(Pair)& pairs);

  void bit_bound(ACE_CDR::UShort bound)
  {
    bit_bound_ = bound;
  }

  bool get_value(ACE_CDR::ULongLong& value, const OPENDDS_VECTOR(String)& names) const;
  size_t get_names(OPENDDS_VECTOR(String)& names, ACE_CDR::ULongLong value) const;

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

OpenDDS_Dcps_Export String bitmask_to_string(ACE_CDR::ULongLong value, const BitmaskHelper& helper);
OpenDDS_Dcps_Export ACE_CDR::ULongLong string_to_bitmask(const String& flags, const BitmaskHelper& helper);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_VALUE_COMMON_H */
