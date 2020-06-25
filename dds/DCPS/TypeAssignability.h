/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPE_ASSIGNABILITY_H
#define OPENDDS_DCPS_TYPE_ASSIGNABILITY_H

#include "TypeObject.h"

#include <utility>
#include <map>
#include <cmath>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// Dummy class for type look up. Only use for facilitating tests.
class OpenDDS_Dcps_Export TypeLookup {
public:
  const MinimalTypeObject& lookup_minimal(const TypeIdentifier& ti) const
  {
    return table_[hash_to_unsigned(ti.equivalence_hash())];
  }

  static void insert_entry(const TypeIdentifier& ti, const MinimalTypeObject& tobj)
  {
    table_[hash_to_unsigned(ti.equivalence_hash())] = tobj;
  }

  static void get_equivalence_hash(EquivalenceHash& out)
  {
    static unsigned int hash = 0;
    unsigned int tmp = ++hash;
    for (int i = 13; i >= 0; --i) {
      out[i] = tmp % 256;
      tmp /= 256;
    }
  }

  static unsigned int hash_to_unsigned(const EquivalenceHash& h)
  {
    unsigned int val = 0;
    for (int i = 13; i >= 0; --i) {
      unsigned int multiplier = (unsigned int)pow(256, 13-i);
      val += h[i] * multiplier;
    }
    return val;
  }

private:
  static std::map<unsigned int, MinimalTypeObject> table_;
};

// Set of pairs of members with each pair contains members from
// two structure types that have the same member ID and name
typedef std::pair<const MinimalStructMember*, const MinimalStructMember*> MemberPair;
typedef OPENDDS_VECTOR(MemberPair) MatchedSet;

class OpenDDS_Dcps_Export TypeAssignability {
public:
  bool assignable(const TypeObject& ta, const TypeObject& tb) const;
  bool assignable(const TypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable(const TypeIdentifier& ta, const TypeObject& tb) const;

private:
  bool assignable_alias(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_annotation(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_annotation(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_struct(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_struct(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_union(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_union(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_bitset(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_bitset(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_sequence(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_sequence(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_array(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_array(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_map(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_map(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_enum(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_enum(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_bitmask(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;
  bool assignable_bitmask(const MinimalTypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable_extended(const MinimalTypeObject& ta, const MinimalTypeObject& tb) const;

  bool assignable_primitive(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable_primitive(const TypeIdentifier& ta, const MinimalTypeObject& tb) const;
  bool assignable_string(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable_string(const TypeIdentifier& ta, const MinimalTypeObject& tb) const;
  bool assignable_plain_sequence(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable_plain_sequence(const TypeIdentifier& ta, const MinimalTypeObject& tb) const;
  bool assignable_plain_array(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable_plain_array(const TypeIdentifier& ta, const MinimalTypeObject& tb) const;
  bool assignable_plain_map(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable_plain_map(const TypeIdentifier& ta, const MinimalTypeObject& tb) const;

  // General helpers
  bool strongly_assignable(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool is_delimited(const TypeIdentifier& ti) const;
  bool is_delimited(const MinimalTypeObject& tobj) const;
  bool is_delimited_with_flags(TypeFlag flags) const;
  bool equal_type_id(const TypeIdentifier& tia, const TypeIdentifier& tib) const;
  const TypeIdentifier& get_base_type(const MinimalTypeObject& type) const;

  // Helpers for assignability of struct
  void erase_key(MinimalTypeObject& type) const;
  void hold_key(MinimalTypeObject& type) const;
  bool hold_key(const TypeIdentifier& ti, MinimalTypeObject& to) const;
  bool struct_rule_enum_key(const MinimalTypeObject& tb, const CommonStructMember& ma) const;
  bool get_sequence_bound(LBound& b, const CommonStructMember& m) const;
  bool get_map_bound(LBound& b, const CommonStructMember& m) const;
  bool get_string_bound(LBound& b, const CommonStructMember& m) const;
  bool get_struct_member(const MinimalTypeObject*& ret, const CommonStructMember& m) const;
  bool get_union_member(const MinimalTypeObject*& ret, const CommonStructMember& m) const;

private:
  TypeLookup typelookup_;
};

} // namepace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TYPE_ASSIGNABILITY_H */
