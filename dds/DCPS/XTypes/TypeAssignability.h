/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPE_ASSIGNABILITY_H
#define OPENDDS_DCPS_TYPE_ASSIGNABILITY_H

#include "TypeObject.h"
#include "TypeLookupService.h"

#include <utility>
#include <map>
#include <cmath>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// Set of pairs of members with each pair contains members from
// two structure types that have the same member ID and name
typedef std::pair<const MinimalStructMember*, const MinimalStructMember*> MemberPair;
typedef OPENDDS_VECTOR(MemberPair) MatchedSet;

class OpenDDS_Dcps_Export TypeAssignability {
public:
  explicit TypeAssignability(TypeLookupService_rch tls) : tl_service_(tls) {}

  bool assignable(const TypeObject& ta, const TypeObject& tb) const;
  bool assignable(const TypeObject& ta, const TypeIdentifier& tb) const;
  bool assignable(const TypeIdentifier& ta, const TypeIdentifier& tb) const;
  bool assignable(const TypeIdentifier& ta, const TypeObject& tb) const;

  void insert_entry(const TypeIdentifier& ti, const TypeObject& tobj)
  {
    tl_service_->add_type_objects_to_cache(ti, tobj);
  }

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

  XTypes::TypeLookupService_rch tl_service_;

  const MinimalTypeObject& lookup_minimal(const TypeIdentifier& ti) const
  {
    return tl_service_->get_type_objects(ti).minimal;
  }
};

} // namepace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TYPE_ASSIGNABILITY_H */
