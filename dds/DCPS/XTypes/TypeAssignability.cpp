/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "TypeAssignability.h"

#include <set>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

/**
 * @brief Both input type objects must be minimal
 */
bool TypeAssignability::assignable(const TypeObject& ta,
                                   const TypeObject& tb) const
{
  if (EK_MINIMAL == ta.kind && EK_MINIMAL == tb.kind) {
    if (TK_ALIAS == ta.minimal.kind || TK_ALIAS == tb.minimal.kind) {
      return assignable_alias(ta.minimal, tb.minimal);
    }

    switch (ta.minimal.kind) {
    case TK_ANNOTATION:
      return assignable_annotation(ta.minimal, tb.minimal);
    case TK_STRUCTURE:
      return assignable_struct(ta.minimal, tb.minimal);
    case TK_UNION:
      return assignable_union(ta.minimal, tb.minimal);
    case TK_BITSET:
      return assignable_bitset(ta.minimal, tb.minimal);
    case TK_SEQUENCE:
      return assignable_sequence(ta.minimal, tb.minimal);
    case TK_ARRAY:
      return assignable_array(ta.minimal, tb.minimal);
    case TK_MAP:
      return assignable_map(ta.minimal, tb.minimal);
    case TK_ENUM:
      return assignable_enum(ta.minimal, tb.minimal);
    case TK_BITMASK:
      return assignable_bitmask(ta.minimal, tb.minimal);
    default:
      return assignable_extended(ta.minimal, tb.minimal);
    }
  }

  return false;
}

/**
 * @brief The first argument must be a minimal type object
 */
bool TypeAssignability::assignable(const TypeObject& ta,
                                   const TypeIdentifier& tb) const
{
  if (EK_MINIMAL == ta.kind) {
    if (TK_ALIAS == ta.minimal.kind) {
      return assignable(get_base_type(ta.minimal), tb);
    }

    switch (ta.minimal.kind) {
    case TK_ANNOTATION:
      return assignable_annotation(ta.minimal, tb);
    case TK_STRUCTURE:
      return assignable_struct(ta.minimal, tb);
    case TK_UNION:
      return assignable_union(ta.minimal, tb);
    case TK_BITSET:
      return assignable_bitset(ta.minimal, tb);
    case TK_SEQUENCE:
      return assignable_sequence(ta.minimal, tb);
    case TK_ARRAY:
      return assignable_array(ta.minimal, tb);
    case TK_MAP:
      return assignable_map(ta.minimal, tb);
    case TK_ENUM:
      return assignable_enum(ta.minimal, tb);
    case TK_BITMASK:
      return assignable_bitmask(ta.minimal, tb);
    default:
      return false;
    }
  }

  return false;
}

/**
 * @brief Both input can be of any type
 */
bool TypeAssignability::assignable(const TypeIdentifier& ta,
                                   const TypeIdentifier& tb) const
{
  if (ta == tb) {
    return true;
  }

  switch (ta.kind()) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
    return assignable_primitive(ta, tb);
  case TI_STRING8_SMALL:
  case TI_STRING8_LARGE:
  case TI_STRING16_SMALL:
  case TI_STRING16_LARGE:
    return assignable_string(ta, tb);
  case TI_PLAIN_SEQUENCE_SMALL:
  case TI_PLAIN_SEQUENCE_LARGE:
    return assignable_plain_sequence(ta, tb);
  case TI_PLAIN_ARRAY_SMALL:
  case TI_PLAIN_ARRAY_LARGE:
    return assignable_plain_array(ta, tb);
  case TI_PLAIN_MAP_SMALL:
  case TI_PLAIN_MAP_LARGE:
    return assignable_plain_map(ta, tb);
  case TI_STRONGLY_CONNECTED_COMPONENT:
    // No rule in the spec for strongly connected components
    return false;
  case EK_COMPLETE:
    // Assuming only equivalence kind of EK_MINIMAL is supported
    return false;
  case EK_MINIMAL: {
    const MinimalTypeObject& base_type_a = lookup_minimal(ta);
    return assignable(TypeObject(base_type_a), tb);
  }
  default:
    return false; // Future extensions
  }
}

/**
 * @brief The second argument must be a minimal type object
 */
bool TypeAssignability::assignable(const TypeIdentifier& ta,
                                   const TypeObject& tb) const
{
  if (EK_MINIMAL == tb.kind) {
    if (TK_ALIAS == tb.minimal.kind) {
      return assignable(ta, get_base_type(tb.minimal));
    }

    switch (ta.kind()) {
    case TK_BOOLEAN:
    case TK_BYTE:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
    case TK_FLOAT32:
    case TK_FLOAT64:
    case TK_FLOAT128:
    case TK_INT8:
    case TK_UINT8:
    case TK_CHAR8:
    case TK_CHAR16:
      return assignable_primitive(ta, tb.minimal);
    case TI_STRING8_SMALL:
    case TI_STRING8_LARGE:
    case TI_STRING16_SMALL:
    case TI_STRING16_LARGE:
      return assignable_string(ta, tb.minimal);
    case TI_PLAIN_SEQUENCE_SMALL:
    case TI_PLAIN_SEQUENCE_LARGE:
      return assignable_plain_sequence(ta, tb.minimal);
    case TI_PLAIN_ARRAY_SMALL:
    case TI_PLAIN_ARRAY_LARGE:
      return assignable_plain_array(ta, tb.minimal);
    case TI_PLAIN_MAP_SMALL:
    case TI_PLAIN_MAP_LARGE:
      return assignable_plain_map(ta, tb.minimal);
    case TI_STRONGLY_CONNECTED_COMPONENT:
      return false;
    case EK_COMPLETE:
      return false;
    case EK_MINIMAL: {
      const MinimalTypeObject& tobj_a = lookup_minimal(ta);
      return assignable(TypeObject(tobj_a), tb);
    }
    default:
      return false;
    }
  }

  return false;
}

/**
 * @brief At least one input type object must be TK_ALIAS
 */
bool TypeAssignability::assignable_alias(const MinimalTypeObject& ta,
                                         const MinimalTypeObject& tb) const
{
  if (TK_ALIAS == ta.kind && TK_ALIAS != tb.kind) {
    const TypeIdentifier& tia = ta.alias_type.body.common.related_type;
    switch (tia.kind()) {
    case TK_BOOLEAN:
    case TK_BYTE:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
    case TK_FLOAT32:
    case TK_FLOAT64:
    case TK_FLOAT128:
    case TK_INT8:
    case TK_UINT8:
    case TK_CHAR8:
    case TK_CHAR16:
      return assignable_primitive(tia, tb);
    case TI_STRING8_SMALL:
    case TI_STRING8_LARGE:
    case TI_STRING16_SMALL:
    case TI_STRING16_LARGE:
      return assignable_string(tia, tb);
    case TI_PLAIN_SEQUENCE_SMALL:
    case TI_PLAIN_SEQUENCE_LARGE:
      return assignable_plain_sequence(tia, tb);
    case TI_PLAIN_ARRAY_SMALL:
    case TI_PLAIN_ARRAY_LARGE:
      return assignable_plain_array(tia, tb);
    case TI_PLAIN_MAP_SMALL:
    case TI_PLAIN_MAP_LARGE:
      return assignable_plain_map(tia, tb);
    case TI_STRONGLY_CONNECTED_COMPONENT:
      // Does alias ever have SCC as its base type?
      return false;
    case EK_COMPLETE:
      // Supporting minimal base type only
      return false;
    case EK_MINIMAL: {
      const MinimalTypeObject& base_type_a = lookup_minimal(tia);
      return assignable(TypeObject(base_type_a), TypeObject(tb));
    }
    default:
      return false; // Future extensions
    }
  } else if (TK_ALIAS != ta.kind && TK_ALIAS == tb.kind) {
    const TypeIdentifier& tib = tb.alias_type.body.common.related_type;
    switch (ta.kind) {
    case TK_ANNOTATION:
      return assignable_annotation(ta, tib);
    case TK_STRUCTURE:
      return assignable_struct(ta, tib);
    case TK_UNION:
      return assignable_union(ta, tib);
    case TK_BITSET:
      return assignable_bitset(ta, tib);
    case TK_SEQUENCE:
      return assignable_sequence(ta, tib);
    case TK_ARRAY:
      return assignable_array(ta, tib);
    case TK_MAP:
      return assignable_map(ta, tib);
    case TK_ENUM:
      return assignable_enum(ta, tib);
    case TK_BITMASK:
      return assignable_bitmask(ta, tib);
    default:
      return false; // Future extensions
    }
  } else if (TK_ALIAS == ta.kind && TK_ALIAS == tb.kind) {
    const TypeIdentifier& tia = ta.alias_type.body.common.related_type;
    const TypeIdentifier& tib = tb.alias_type.body.common.related_type;
    return assignable(tia, tib);
  }

  return false;
}

/**
 * @brief The first type must be TK_ANNOTATION.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_annotation(const MinimalTypeObject&,
                                              const MinimalTypeObject&) const
{
  // No rule for annotation in the spec
  return false;
}

/**
 * @brief The first type must be TK_ANNOTATION.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_annotation(const MinimalTypeObject&,
                                              const TypeIdentifier&) const
{
  // No rule for annotation in the spec
  return false;
}

/**
 * @brief The first type must be TK_STRUCTURE.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                          const MinimalTypeObject& tb) const
{
  if (TK_STRUCTURE != tb.kind) {
    return false;
  }

  // Extensibility kind must match
  const TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
  const ACE_CDR::UShort a_exten = ta.struct_type.struct_flags & extensibility_mask;
  if (a_exten != (tb.struct_type.struct_flags & extensibility_mask)) {
    return false;
  }

  // If T1 is appendable, then members with the same member_index have the
  // same member ID, the same setting for the 'optional' attribute and the
  // T1 member type is strongly assignable from the T2 member type.
  // If T1 is final, then they meet the same condition as for T1 being
  // appendable and in addition T1 and T2 have the same set of member IDs.
  if (IS_FINAL == a_exten &&
      ta.struct_type.member_seq.length() != tb.struct_type.member_seq.length()) {
    return false;
  }
  if (IS_APPENDABLE == a_exten || IS_FINAL == a_exten) {
    const unsigned num_members = (std::min)(ta.struct_type.member_seq.length(),
                                        tb.struct_type.member_seq.length());
    for (unsigned i = 0; i < num_members; ++i) {
      if (ta.struct_type.member_seq[i].common.member_id !=
          tb.struct_type.member_seq[i].common.member_id ||
          (ta.struct_type.member_seq[i].common.member_flags & IS_OPTIONAL) !=
          (tb.struct_type.member_seq[i].common.member_flags & IS_OPTIONAL) ||
          !strongly_assignable(ta.struct_type.member_seq[i].common.member_type_id,
                               tb.struct_type.member_seq[i].common.member_type_id)) {
        return false;
      }
    }
  }

  // Any members in T1 and T2 that have the same name also have
  // the same ID, and vice versa
  MatchedSet matched_members;
  for (unsigned i = 0; i < ta.struct_type.member_seq.length(); ++i) {
    MemberId id_a = ta.struct_type.member_seq[i].common.member_id;
    const NameHash& h_a = ta.struct_type.member_seq[i].detail.name_hash;
    ACE_CDR::ULong name_a = (h_a[0] << 24) | (h_a[1] << 16) | (h_a[2] << 8) | (h_a[3]);
    for (unsigned j = 0; j < tb.struct_type.member_seq.length(); ++j) {
      MemberId id_b = tb.struct_type.member_seq[j].common.member_id;
      const NameHash& h_b = tb.struct_type.member_seq[j].detail.name_hash;
      ACE_CDR::ULong name_b = (h_b[0] << 24) | (h_b[1] << 16) | (h_b[2] << 8) | (h_b[3]);

      if (!type_consistency_.ignore_member_names) {
        if ((name_a == name_b && id_a != id_b) || (id_a == id_b && name_a != name_b)) {
          return false;
        } else if (name_a == name_b && id_a == id_b) {
          matched_members.push_back(std::make_pair(&ta.struct_type.member_seq[i],
                                                   &tb.struct_type.member_seq[j]));
          break;
        }
      } else if (id_a == id_b) {
        matched_members.push_back(std::make_pair(&ta.struct_type.member_seq[i],
                                                 &tb.struct_type.member_seq[j]));
        break;
      }
    }
  }

  // There is at least one member m1 of T1 and one corresponding member
  // m2 of T2 such that m1.id == m2.id
  if (matched_members.size() == 0) {
    return false;
  }

  // For any member m2 of T2, if there is a member m1 of T1 with the same
  // ID, then the type KeyErased(m1.type) is-assignable-from the type
  // KeyErased(m2.type).
  // For any non-aggregated type T, we consider that KeyErased(T) = T
  // (whereas the spec only defines KeyErased for aggregated types).
  // Consequently, this rule applies to any pair of members m2 of T2 and
  // m1 of T1 with the same ID.
  for (size_t i = 0; i < matched_members.size(); ++i) {
    const CommonStructMember& member = matched_members[i].second->common;
    const MinimalTypeObject* toa = 0;
    const MinimalTypeObject* tob = 0;
    bool aggregated_type_matched = false;
    if (get_struct_member(tob, member)) {
      if (!get_struct_member(toa, matched_members[i].first->common)) {
        return false;
      }
      aggregated_type_matched = true;
    } else if (get_union_member(tob, member)) {
      if (!get_union_member(toa, matched_members[i].first->common)) {
        return false;
      }
      aggregated_type_matched = true;
    }

    if (aggregated_type_matched) {
      MinimalTypeObject key_erased_a = *toa, key_erased_b = *tob;
      erase_key(key_erased_a);
      erase_key(key_erased_b);
      if (!assignable(TypeObject(key_erased_a), TypeObject(key_erased_b))) {
        return false;
      }
    } else if (!assignable(matched_members[i].first->common.member_type_id,
                           matched_members[i].second->common.member_type_id)) {
      return false;
    }
  }

  // Members for which both optional is false and must_understand is true in
  // either T1 or T2 appear in both T1 and T2.
  // Members marked as key in either T1 or T2 appear in both T1 and T2.
  for (unsigned i = 0; i < ta.struct_type.member_seq.length(); ++i) {
    const MemberFlag& flags = ta.struct_type.member_seq[i].common.member_flags;
    MemberId id = ta.struct_type.member_seq[i].common.member_id;
    bool found = false;
    if ((flags & (IS_OPTIONAL | IS_MUST_UNDERSTAND)) == IS_MUST_UNDERSTAND) {
      for (size_t j = 0; j < matched_members.size(); ++j) {
        if (id == matched_members[j].first->common.member_id) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }

    found = false;
    if ((flags & IS_KEY) == IS_KEY) {
      for (size_t j = 0; j < matched_members.size(); ++j) {
        if (id == matched_members[j].first->common.member_id) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
  }

  for (unsigned i = 0; i < tb.struct_type.member_seq.length(); ++i) {
    const MemberFlag& flags = tb.struct_type.member_seq[i].common.member_flags;
    MemberId id = tb.struct_type.member_seq[i].common.member_id;
    bool found = false;
    if ((flags & (IS_OPTIONAL | IS_MUST_UNDERSTAND)) == IS_MUST_UNDERSTAND) {
      for (size_t j = 0; j < matched_members.size(); ++j) {
        if (id == matched_members[j].second->common.member_id) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }

    found = false;
    if ((flags & IS_KEY) == IS_KEY) {
      for (size_t j = 0; j < matched_members.size(); ++j) {
        if (id == matched_members[j].second->common.member_id) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
  }

  // For any string key member m2 in T2, the m1 member of T1 with the
  // same member ID verifies m1.type.length >= m2.type.length
  for (size_t i = 0; i < matched_members.size(); ++i) {
    const CommonStructMember& member = matched_members[i].second->common;
    MemberFlag flags = member.member_flags;
    LBound bound_a, bound_b;
    if ((flags & IS_KEY) == IS_KEY && get_string_bound(bound_b, member)) {
      if (!get_string_bound(bound_a, matched_members[i].first->common)) {
        return false;
      }
      if (bound_a < bound_b) {
        return false;
      }
    }
  }

  // For any enumerated key member m2 in T2, the m1 member of T1 with
  // the same member ID verifies that all literals in m2.type appear as
  // literals in m1.type
  for (size_t i = 0; i < matched_members.size(); ++i) {
    const CommonStructMember& member = matched_members[i].second->common;
    MemberFlag flags = member.member_flags;
    if ((flags & IS_KEY) == IS_KEY &&
        EK_MINIMAL == member.member_type_id.kind()) {
      const MinimalTypeObject& tob = lookup_minimal(member.member_type_id);
      if (TK_ENUM == tob.kind) {
        if (!struct_rule_enum_key(tob, matched_members[i].first->common)) {
          return false;
        }
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base_b = get_base_type(tob);
        if (EK_MINIMAL == base_b.kind()) {
          const MinimalTypeObject& base_obj_b = lookup_minimal(base_b);
          if (TK_ENUM == base_obj_b.kind &&
              !struct_rule_enum_key(base_obj_b, matched_members[i].first->common)) {
            return false;
          }
        }
      }
    }
  }

  // For any sequence or map key member m2 in T2, the m1 member of T1
  // with the same member ID verifies m1.type.length >= m2.type.length
  for (size_t i = 0; i < matched_members.size(); ++i) {
    const CommonStructMember& member = matched_members[i].second->common;
    MemberFlag flags = member.member_flags;
    LBound bound_a, bound_b;
    if ((flags & IS_KEY) == IS_KEY) {
      if (get_sequence_bound(bound_b, member)) {
        if (!get_sequence_bound(bound_a, matched_members[i].first->common)) {
          return false;
        }
        if (bound_a < bound_b) {
          return false;
        }
      } else if (get_map_bound(bound_b, member)) {
        if (!get_map_bound(bound_a, matched_members[i].first->common)) {
          return false;
        }
        if (bound_a < bound_b) {
          return false;
        }
      }
    }
  }

  // For any structure or union key member m2 in T2, the m1 member
  // of T1 with the same member ID verifies that KeyHolder(m1.type)
  // is-assignable-from KeyHolder(m2.type)
  for (size_t i = 0; i < matched_members.size(); ++i) {
    const CommonStructMember& member = matched_members[i].second->common;
    MemberFlag flags = member.member_flags;
    if ((flags & IS_KEY) == IS_KEY) {
      const MinimalTypeObject* toa = 0;
      const MinimalTypeObject* tob = 0;
      bool type_matched = false;
      if (get_struct_member(tob, member)) {
        if (!get_struct_member(toa, matched_members[i].first->common)) {
          return false;
        }
        type_matched = true;
      } else if (get_union_member(tob, member)) {
        if (!get_union_member(toa, matched_members[i].first->common)) {
          return false;
        }
        type_matched = true;
      }

      if (type_matched) {
        MinimalTypeObject key_holder_a = *toa, key_holder_b = *tob;
        hold_key(key_holder_a);
        hold_key(key_holder_b);
        if (!assignable(TypeObject(key_holder_a), TypeObject(key_holder_b))) {
          return false;
        }

        // For any union key member m2 in T2, the m1 member of T1 with the
        // same ID verifies that: for every discriminator value of m2.type
        // that selects a member m22 in m2.type, the discriminator value
        // selects a member m11 in m1.type that verifies KeyHolder(m11.type)
        // is-assignable-from KeyHolder(m22.type)
        if (TK_UNION == tob->kind) {
          const MinimalUnionMemberSeq& mseq_a = toa->union_type.member_seq;
          const MinimalUnionMemberSeq& mseq_b = tob->union_type.member_seq;
          for (unsigned j = 0; j < mseq_b.length(); ++j) {
            const UnionCaseLabelSeq& labels_b = mseq_b[j].common.label_seq;
            for (unsigned k = 0; k < mseq_a.length(); ++k) {
              const UnionCaseLabelSeq& labels_a = mseq_a[k].common.label_seq;
              bool matched = false;
              for (unsigned p = 0; p < labels_b.length(); ++p) {
                for (unsigned q = 0; q < labels_a.length(); ++q) {
                  if (labels_b[p] == labels_a[q]) {
                    const TypeIdentifier& tib = mseq_b[j].common.type_id;
                    const TypeIdentifier& tia = mseq_a[k].common.type_id;
                    MinimalTypeObject kh_a, kh_b;
                    bool ret_b = hold_key(tib, kh_b);
                    bool ret_a = hold_key(tia, kh_a);
                    if ((ret_a && ret_b && !assignable(TypeObject(kh_a), TypeObject(kh_b))) ||
                        (ret_a && !ret_b && !assignable(TypeObject(kh_a), tib)) ||
                        (!ret_a && ret_b && !assignable(tia, TypeObject(kh_b))) ||
                        (!ret_a && !ret_b && !assignable(tia, tib))) {
                      return false;
                    }
                    matched = true;
                    break;
                  }
                } // labels_a
                if (matched) break;
              } // labels_b
            } // mseq_a
          } // mseq_b
        }
      }
    } // IS_KEY
  }

  return true;
}

/**
 * @brief The first type must be TK_STRUCTURE.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                          const TypeIdentifier& tb) const
{
  if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_STRUCTURE == tob.kind) {
      return assignable_struct(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_struct(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_UNION.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_union(const MinimalTypeObject& ta,
                                         const MinimalTypeObject& tb) const
{
  if (TK_UNION != tb.kind) {
    return false;
  }

  // Extensibility kind must match
  const TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
  if ((ta.union_type.union_flags & extensibility_mask) !=
      (tb.union_type.union_flags & extensibility_mask)) {
    return false;
  }

  OPENDDS_SET(ACE_CDR::Long) labels_set_a;
  for (unsigned i = 0; i < ta.union_type.member_seq.length(); ++i) {
    const UnionCaseLabelSeq& labels_a = ta.union_type.member_seq[i].common.label_seq;
    labels_set_a.insert(labels_a.members.begin(), labels_a.members.end());
  }

  // If extensibility is final, then the set of labels must be identical.
  // Assuming labels are mapped to values identically in both input types.
  if ((ta.union_type.union_flags & extensibility_mask) == IS_FINAL) {
    for (unsigned i = 0; i < tb.union_type.member_seq.length(); ++i) {
      const UnionCaseLabelSeq& labels_b = tb.union_type.member_seq[i].common.label_seq;
      for (unsigned j = 0; j < labels_b.length(); ++j) {
        if (labels_set_a.find(labels_b.members[j]) == labels_set_a.end()) {
          return false;
        }
        labels_set_a.erase(labels_b.members[j]);
      }
    }
    if (labels_set_a.size() > 0) {
      return false;
    }
  } else { // Must have at least one common label other than the default
    // This implementation assumes that the default member has IS_DEFAULT
    // flag turned on, but the label "default" does not map into a numeric
    // value for storing on the member's UnionCaseLabelSeq. Instead, only
    // the other labels, if any, for this default members will have their
    // numeric values stored in its UnionCaseLabelSeq.
    bool found = false;
    for (unsigned i = 0; i < tb.union_type.member_seq.length(); ++i) {
      const UnionCaseLabelSeq& labels_b = tb.union_type.member_seq[i].common.label_seq;
      for (unsigned j = 0; j < labels_b.length(); ++j) {
        if (labels_set_a.find(labels_b[j]) != labels_set_a.end()) {
          found = true;
          break;
        }
      }
      if (found) break;
    }
    if (!found) {
      return false;
    }
  }

  // Discriminator type must be one of these: (i) non-float primitive types,
  // or (ii) enumerated types, or (iii) an alias type that resolves to
  // one of the above two type kinds
  const TypeIdentifier& tia = ta.union_type.discriminator.common.type_id;
  const TypeIdentifier& tib = tb.union_type.discriminator.common.type_id;
  if (!strongly_assignable(tia, tib)) {
    return false;
  }

  // Both discriminators are keys or neither are keys
  const MemberFlag& flags_a = ta.union_type.discriminator.common.member_flags;
  const MemberFlag& flags_b = tb.union_type.discriminator.common.member_flags;
  if ((((flags_a & IS_KEY) == IS_KEY) && ((flags_b & IS_KEY) != IS_KEY)) ||
      (((flags_a & IS_KEY) != IS_KEY) && ((flags_b & IS_KEY) == IS_KEY))) {
    return false;
  }

  // Members with the same ID must have the same name, and vice versa
  if (!type_consistency_.ignore_member_names) {
    OPENDDS_MAP(MemberId, ACE_CDR::ULong) id_to_name_a;
    OPENDDS_MAP(ACE_CDR::ULong, MemberId) name_to_id_a;
    for (unsigned i = 0; i < ta.union_type.member_seq.length(); ++i) {
      MemberId id = ta.union_type.member_seq[i].common.member_id;
      const NameHash& h = ta.union_type.member_seq[i].detail.name_hash;
      ACE_CDR::ULong name = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      id_to_name_a[id] = name;
      name_to_id_a[name] = id;
    }

    for (unsigned i = 0; i < tb.union_type.member_seq.length(); ++i) {
      MemberId id = tb.union_type.member_seq[i].common.member_id;
      const NameHash& h = tb.union_type.member_seq[i].detail.name_hash;
      ACE_CDR::ULong name = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      if (id_to_name_a.find(id) != id_to_name_a.end() &&
          id_to_name_a[id] != name) {
        return false;
      }

      if (name_to_id_a.find(name) != name_to_id_a.end() &&
          name_to_id_a[name] != id) {
        return false;
      }
    }
  }

  // For all non-default labels in T2 that select some member in T1,
  // the type of the selected member in T1 is assignable from the
  // type of the T2 member
  for (unsigned i = 0; i < tb.union_type.member_seq.length(); ++i) {
    const UnionCaseLabelSeq& label_seq_b = tb.union_type.member_seq[i].common.label_seq;
    for (unsigned j = 0; j < ta.union_type.member_seq.length(); ++j) {
      // Consider a case when tb has multiple labels for a member, e.g.,
      // "LABEL1" and "LABEL2" are associated with a member of type MemberB,
      // and ta has two members, one has label "LABEL1" and
      // type MemberA1, and the other has label "LABEL2" and
      // type MemberA2. There are two possible ways to check assignability:
      // (i) check whether BOTH MemberA1 and MemberA2 are assignable from
      // MemberB since labels for MemberB match the labels of both MemberA1
      // and MemberA2 (i.e., all must be assignable), or (ii) check EITHER
      // MemberA1 OR MemberA2 is assignable from MemberB (i.e., one member
      // of ta that is assignable is sufficient). The spec does not clearly
      // say which way we should do it. For now we are going with method (i).
      const UnionCaseLabelSeq& label_seq_a = ta.union_type.member_seq[j].common.label_seq;
      bool matched = false;
      for (unsigned k = 0; k < label_seq_b.length(); ++k) {
        for (unsigned t = 0; t < label_seq_a.length(); ++t) {
          if (label_seq_b.members[k] == label_seq_a.members[t]) {
            const TypeIdentifier& tia = ta.union_type.member_seq[j].common.type_id;
            const TypeIdentifier& tib = tb.union_type.member_seq[i].common.type_id;
            if (!assignable(tia, tib)) {
              return false;
            }
            matched = true;
            break;
          }
        }
        if (matched) break;
      }
    }
  }

  // If any non-default labels of T1 that select the default member of T2,
  // the type of the member in T1 is assignable from the type of the default
  // member in T2
  for (unsigned i = 0; i < tb.union_type.member_seq.length(); ++i) {
    const UnionMemberFlag& flags_b = tb.union_type.member_seq[i].common.member_flags;
    if ((flags_b & IS_DEFAULT) == IS_DEFAULT) {
      const UnionCaseLabelSeq& label_seq_b = tb.union_type.member_seq[i].common.label_seq;
      for (unsigned j = 0; j < ta.union_type.member_seq.length(); ++j) {
        const UnionCaseLabelSeq& label_seq_a = ta.union_type.member_seq[j].common.label_seq;
        bool matched = false;
        for (unsigned k = 0; k < label_seq_a.length(); ++k) {
          for (unsigned t = 0; t < label_seq_b.length(); ++t) {
            if (label_seq_a[k] == label_seq_b[t]) {
              const TypeIdentifier& tia = ta.union_type.member_seq[j].common.type_id;
              const TypeIdentifier& tib = tb.union_type.member_seq[i].common.type_id;
              if (!assignable(tia, tib)) {
                return false;
              }
              matched = true;
              break;
            }
          }
          if (matched) break;
        }
      }
      break;
    }
  }

  // If T1 and T2 both have default labels, the type of T1's default member
  // is assignable from the type of T2's default member
  for (unsigned i = 0; i < ta.union_type.member_seq.length(); ++i) {
    const UnionMemberFlag& flags_a = ta.union_type.member_seq[i].common.member_flags;
    if ((flags_a & IS_DEFAULT) == IS_DEFAULT) {
      for (unsigned j = 0; j < tb.union_type.member_seq.length(); ++j) {
        const UnionMemberFlag& flags_b = tb.union_type.member_seq[j].common.member_flags;
        if ((flags_b & IS_DEFAULT) == IS_DEFAULT) {
          const TypeIdentifier& tia = ta.union_type.member_seq[i].common.type_id;
          const TypeIdentifier& tib = tb.union_type.member_seq[j].common.type_id;
          if (!assignable(tia, tib)) {
            return false;
          }
          break;
        }
      }
      break;
    }
  }

  return true;
}

/**
 * @brief The first type must be TK_UNION.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_union(const MinimalTypeObject& ta,
                                         const TypeIdentifier& tb) const
{
  if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_UNION == tob.kind) {
      return assignable_union(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_union(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_BITSET.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_bitset(const MinimalTypeObject&,
                                          const MinimalTypeObject&) const
{
  // No rule for bitset in the spec
  return false;
}

/**
 * @brief The first type must be TK_BITSET.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_bitset(const MinimalTypeObject&,
                                          const TypeIdentifier&) const
{
  // No rule for bitset in the spec
  return false;
}

/**
 * @brief The first type must be TK_SEQUENCE.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
{
  if (TK_SEQUENCE != tb.kind) {
    return false;
  }
  return strongly_assignable(ta.sequence_type.element.common.type,
                             tb.sequence_type.element.common.type);
}

/**
 * @brief The first type must be TK_SEQUENCE.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
{
  if (TI_PLAIN_SEQUENCE_SMALL == tb.kind()) {
    return strongly_assignable(ta.sequence_type.element.common.type,
                               *tb.seq_sdefn().element_identifier);
  } else if (TI_PLAIN_SEQUENCE_LARGE == tb.kind()) {
    return strongly_assignable(ta.sequence_type.element.common.type,
                               *tb.seq_ldefn().element_identifier);
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_SEQUENCE == tob.kind) {
      return assignable_sequence(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_sequence(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_ARRAY.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_array(const MinimalTypeObject& ta,
                                         const MinimalTypeObject& tb) const
{
  if (TK_ARRAY != tb.kind) {
    return false;
  }

  // Bounds must match
  const LBoundSeq& bounds_a = ta.array_type.header.common.bound_seq;
  const LBoundSeq& bounds_b = tb.array_type.header.common.bound_seq;
  if (bounds_a.members.size() != bounds_b.members.size()) {
    return false;
  }

  for (unsigned i = 0; i < bounds_a.members.size(); ++i) {
    if (bounds_a.members[i] != bounds_b.members[i]) {
      return false;
    }
  }
  return strongly_assignable(ta.array_type.element.common.type,
                             tb.array_type.element.common.type);
}

/**
 * @brief The first type must be TK_ARRAY.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_array(const MinimalTypeObject& ta,
                                         const TypeIdentifier& tb) const
{
  const LBoundSeq& bounds_a = ta.array_type.header.common.bound_seq;
  if (TI_PLAIN_ARRAY_SMALL == tb.kind()) {
    const SBoundSeq& bounds_b = tb.array_sdefn().array_bound_seq;
    if (bounds_a.members.size() != bounds_b.members.size()) {
      return false;
    }

    for (unsigned i = 0; i < bounds_a.members.size(); ++i) {
      if (bounds_a.members[i] != static_cast<LBound>(bounds_b.members[i])) {
        return false;
      }
    }

    return strongly_assignable(ta.array_type.element.common.type,
                               *tb.array_sdefn().element_identifier);
  } else if (TI_PLAIN_ARRAY_LARGE == tb.kind()) {
    const LBoundSeq& bounds_b = tb.array_ldefn().array_bound_seq;
    if (bounds_a.members.size() != bounds_b.members.size()) {
      return false;
    }

    for (unsigned i = 0; i < bounds_a.members.size(); ++i) {
      if (bounds_a.members[i] != bounds_b.members[i]) {
        return false;
      }
    }
    return strongly_assignable(ta.array_type.element.common.type,
                               *tb.array_ldefn().element_identifier);
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_ARRAY == tob.kind) {
      return assignable_array(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_array(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_MAP.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                       const MinimalTypeObject& tb) const
{
  if (TK_MAP != tb.kind) {
    return false;
  }
  return strongly_assignable(ta.map_type.key.common.type,
                             tb.map_type.key.common.type) &&
    strongly_assignable(ta.map_type.element.common.type,
                        tb.map_type.element.common.type);
}

/**
 * @brief The first type must be TK_MAP.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                       const TypeIdentifier& tb) const
{
  if (TI_PLAIN_MAP_SMALL == tb.kind()) {
    return strongly_assignable(ta.map_type.key.common.type,
                               *tb.map_sdefn().key_identifier) &&
      strongly_assignable(ta.map_type.element.common.type,
                          *tb.map_sdefn().element_identifier);
  } else if (TI_PLAIN_MAP_LARGE == tb.kind()) {
    return strongly_assignable(ta.map_type.key.common.type,
                               *tb.map_ldefn().key_identifier) &&
      strongly_assignable(ta.map_type.element.common.type,
                          *tb.map_ldefn().element_identifier);
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_MAP == tob.kind) {
      return assignable_map(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_map(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_ENUM.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_enum(const MinimalTypeObject& ta,
                                        const MinimalTypeObject& tb) const
{
  if (TK_ENUM != tb.kind) {
    return false;
  }

  // Assuming that EnumTypeFlag is used and contains extensibility
  // of the containing type.
  const TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
  TypeFlag ta_ext = ta.enumerated_type.enum_flags & extensibility_mask;
  TypeFlag tb_ext = tb.enumerated_type.enum_flags & extensibility_mask;
  if (ta_ext != tb_ext) {
    return false;
  }

  // T1.bit_bound and T2.bit_bound must be equal (DDSXTY14-34)
  if (ta.enumerated_type.header.common.bit_bound !=
      tb.enumerated_type.header.common.bit_bound) {
    return false;
  }

  const size_t size_a = ta.enumerated_type.literal_seq.members.size();
  const size_t size_b = tb.enumerated_type.literal_seq.members.size();
  OPENDDS_MAP(ACE_CDR::ULong, ACE_CDR::Long) ta_name_to_value;
  for (size_t i = 0; i < size_a; ++i) {
    const NameHash& h = ta.enumerated_type.literal_seq.members[i].detail.name_hash;
    ACE_CDR::ULong key_a = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
    ta_name_to_value[key_a] = ta.enumerated_type.literal_seq.members[i].common.value;
  }

  // If extensibility is FINAL, both must have the same literals.
  if (IS_FINAL == ta_ext) {
    if (size_a != size_b) {
      return false;
    }

    for (size_t i = 0; i < size_b; ++i) {
      const NameHash& h = tb.enumerated_type.literal_seq.members[i].detail.name_hash;
      ACE_CDR::ULong key_b = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);

      // Literals that have the same name must have the same value.
      if (ta_name_to_value.find(key_b) == ta_name_to_value.end() ||
          ta_name_to_value[key_b] != tb.enumerated_type.literal_seq.members[i].common.value) {
        return false;
      }
    }
  } else {
    // Any literals that have the same name also have the same value
    for (size_t i = 0; i < size_b; ++i) {
      const NameHash& h = tb.enumerated_type.literal_seq.members[i].detail.name_hash;
      ACE_CDR::ULong key_b = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      if (ta_name_to_value.find(key_b) != ta_name_to_value.end() &&
          ta_name_to_value[key_b] != tb.enumerated_type.literal_seq.members[i].common.value) {
        return false;
      }
    }

    OPENDDS_MAP(ACE_CDR::ULong, ACE_CDR::ULong) ta_value_to_name;
    for (size_t i = 0; i < size_a; ++i) {
      ACE_CDR::ULong value_a = ta.enumerated_type.literal_seq.members[i].common.value;
      const NameHash& h = ta.enumerated_type.literal_seq.members[i].detail.name_hash;
      ACE_CDR::ULong name_a = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      ta_value_to_name[value_a] = name_a;
    }

    // Any literals that have the same value also have the same name
    for (size_t i = 0; i < size_b; ++i) {
      ACE_CDR::ULong value_b = tb.enumerated_type.literal_seq.members[i].common.value;
      const NameHash& h = tb.enumerated_type.literal_seq.members[i].detail.name_hash;
      ACE_CDR::ULong name_b = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      if (ta_value_to_name.find(value_b) != ta_value_to_name.end() &&
          ta_value_to_name[value_b] != name_b) {
        return false;
      }
    }
  }

  return true;
}

/**
 * @brief The first type must be TK_ENUM.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_enum(const MinimalTypeObject& ta,
                                        const TypeIdentifier& tb) const
{
  if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_ENUM == tob.kind) {
      return assignable_enum(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_enum(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be TK_BITMASK.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_bitmask(const MinimalTypeObject& ta,
                                           const MinimalTypeObject& tb) const
{
  if (TK_BITMASK == tb.kind) {
    return ta.bitmask_type.header.common.bit_bound ==
      tb.bitmask_type.header.common.bit_bound;
  }

  return false;
}

/**
 * @brief The first type must be TK_BITMASK.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_bitmask(const MinimalTypeObject& ta,
                                           const TypeIdentifier& tb) const
{
  BitBound ta_bit_bound = ta.bitmask_type.header.common.bit_bound;
  if (TK_UINT8 == tb.kind()) {
    return 1 <= ta_bit_bound && ta_bit_bound <= 8;
  } else if (TK_UINT16 == tb.kind()) {
    return 9 <= ta_bit_bound && ta_bit_bound <= 16;
  } else if (TK_UINT32 == tb.kind()) {
    return 17 <= ta_bit_bound && ta_bit_bound <= 32;
  } else if (TK_UINT64 == tb.kind()) {
    return 33 <= ta_bit_bound && ta_bit_bound <= 64;
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_BITMASK == tob.kind) {
      return assignable_bitmask(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_bitmask(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a future extension type kind.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_extended(const MinimalTypeObject&,
                                            const MinimalTypeObject&) const
{
  // Future extensions
  return false;
}

/**
 * @brief The first type must be a primitive type.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_primitive(const TypeIdentifier& ta,
                                             const TypeIdentifier& tb) const
{
  if (ta.kind() == tb.kind()) {
    return true;
  }

  if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_BITMASK == tob.kind) {
      return assignable_primitive(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_primitive(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a primitive type.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_primitive(const TypeIdentifier& ta,
                                             const MinimalTypeObject& tb) const
{
  if (TK_BITMASK != tb.kind ||
      !(TK_UINT8 == ta.kind() || TK_UINT16 == ta.kind() ||
        TK_UINT32 == ta.kind() || TK_UINT64 == ta.kind())) {
    return false;
  }

  BitBound bit_bound = tb.bitmask_type.header.common.bit_bound;
  if (TK_UINT8 == ta.kind()) {
    return 1 <= bit_bound && bit_bound <= 8;
  } else if (TK_UINT16 == ta.kind()) {
    return 9 <= bit_bound && bit_bound <= 16;
  } else if (TK_UINT32 == ta.kind()) {
    return 17 <= bit_bound && bit_bound <= 32;
  } else { // TK_UINT64
    return 33 <= bit_bound && bit_bound <= 64;
  }
}

/**
 * @brief The first type must be a string type.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_string(const TypeIdentifier& ta,
                                          const TypeIdentifier& tb) const
{
  if (TI_STRING8_SMALL == tb.kind() || TI_STRING8_LARGE == tb.kind()) {
    if (TI_STRING8_SMALL == ta.kind() || TI_STRING8_LARGE == ta.kind()) {
      return true;
    }
  } else if (TI_STRING16_SMALL == tb.kind() || TI_STRING16_LARGE == tb.kind()) {
    if (TI_STRING16_SMALL == ta.kind() || TI_STRING16_LARGE == ta.kind()) {
      return true;
    }
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_string(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a string type.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_string(const TypeIdentifier&,
                                          const MinimalTypeObject&) const
{
  // The second type cannot be string since string types do not have
  // type object. Thus the first type is not assignable from the second type.
  return false;
}

/**
 * @brief The first type must be a plain sequence type.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_plain_sequence(const TypeIdentifier& ta,
                                                  const TypeIdentifier& tb) const
{
  if (TI_PLAIN_SEQUENCE_SMALL == tb.kind()) {
    if (TI_PLAIN_SEQUENCE_SMALL == ta.kind()) {
      return strongly_assignable(*ta.seq_sdefn().element_identifier,
                                 *tb.seq_sdefn().element_identifier);
    } else { // TI_PLAIN_SEQUENCE_LARGE
      return strongly_assignable(*ta.seq_ldefn().element_identifier,
                                 *tb.seq_sdefn().element_identifier);
    }
  } else if (TI_PLAIN_SEQUENCE_LARGE == tb.kind()) {
    if (TI_PLAIN_SEQUENCE_SMALL == ta.kind()) {
      return strongly_assignable(*ta.seq_sdefn().element_identifier,
                                 *tb.seq_ldefn().element_identifier);
    } else { // TI_PLAIN_SEQUENCE_LARGE
      return strongly_assignable(*ta.seq_ldefn().element_identifier,
                                 *tb.seq_ldefn().element_identifier);
    }
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_SEQUENCE == tob.kind) {
      return assignable_plain_sequence(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_plain_sequence(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Can tb.kind be EK_COMPLETE? More generally, can a MinimalTypeObject
    // depend on a TypeIdentifier that identifies a class of types which have
    // COMPLETE equivalence relation.
    // For now, assuming tb.kind of EK_COMPLETE is not supported.
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a plain sequence type.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_plain_sequence(const TypeIdentifier& ta,
                                                  const MinimalTypeObject& tb) const
{
  if (TK_SEQUENCE == tb.kind) {
    if (TI_PLAIN_SEQUENCE_SMALL == ta.kind()) {
      return strongly_assignable(*ta.seq_sdefn().element_identifier,
                                 tb.sequence_type.element.common.type);
    } else { // TI_PLAIN_SEQUENCE_LARGE
      return strongly_assignable(*ta.seq_ldefn().element_identifier,
                                 tb.sequence_type.element.common.type);
    }
  }

  return false;
}

/**
 * @brief The first type must be a plain array type.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_plain_array(const TypeIdentifier& ta,
                                               const TypeIdentifier& tb) const
{
  if (TI_PLAIN_ARRAY_SMALL == tb.kind()) {
    const Sequence<SBound>& bounds_b = tb.array_sdefn().array_bound_seq;
    if (TI_PLAIN_ARRAY_SMALL == ta.kind()) {
      const Sequence<SBound>& bounds_a = ta.array_sdefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_sdefn().element_identifier,
                                 *tb.array_sdefn().element_identifier);
    } else { // TI_PLAIN_ARRAY_LARGE
      const Sequence<LBound>& bounds_a = ta.array_ldefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != static_cast<LBound>(bounds_b.members[i])) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_ldefn().element_identifier,
                                 *tb.array_sdefn().element_identifier);
    }
  } else if (TI_PLAIN_ARRAY_LARGE == tb.kind()) {
    const Sequence<LBound>& bounds_b = tb.array_ldefn().array_bound_seq;
    if (TI_PLAIN_ARRAY_SMALL == ta.kind()) {
      const Sequence<SBound>& bounds_a = ta.array_sdefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (static_cast<LBound>(bounds_a.members[i]) != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_sdefn().element_identifier,
                                 *tb.array_ldefn().element_identifier);
    } else { // TI_PLAIN_ARRAY_LARGE
      const Sequence<LBound>& bounds_a = ta.array_ldefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_ldefn().element_identifier,
                                 *tb.array_ldefn().element_identifier);
    }
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_ARRAY == tob.kind) {
      return assignable_plain_array(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_plain_array(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported (similar to the way
    // assignability for plain sequences and plain maps are being handled)
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a plain array type.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_plain_array(const TypeIdentifier& ta,
                                               const MinimalTypeObject& tb) const
{
  if (TK_ARRAY == tb.kind) {
    const Sequence<LBound>& bounds_b = tb.array_type.header.common.bound_seq;
    if (TI_PLAIN_ARRAY_SMALL == ta.kind()) {
      const Sequence<SBound>& bounds_a = ta.array_sdefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (static_cast<LBound>(bounds_a.members[i]) != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_sdefn().element_identifier,
                                 tb.array_type.element.common.type);
    } else { // TI_PLAIN_ARRAY_LARGE
      const Sequence<LBound>& bounds_a = ta.array_ldefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_ldefn().element_identifier,
                                 tb.array_type.element.common.type);
    }
  }

  return false;
}

/**
 * @brief The first type must be a plain map type.
 *        The second type can be anything.
 */
bool TypeAssignability::assignable_plain_map(const TypeIdentifier& ta,
                                             const TypeIdentifier& tb) const
{
  if (TI_PLAIN_MAP_SMALL == tb.kind()) {
    if (TI_PLAIN_MAP_SMALL == ta.kind()) {
      return strongly_assignable(*ta.map_sdefn().key_identifier,
                                 *tb.map_sdefn().key_identifier) &&
        strongly_assignable(*ta.map_sdefn().element_identifier,
                            *tb.map_sdefn().element_identifier);
    } else { // TI_PLAIN_MAP_LARGE
      return strongly_assignable(*ta.map_ldefn().key_identifier,
                                 *tb.map_sdefn().key_identifier) &&
        strongly_assignable(*ta.map_ldefn().element_identifier,
                            *tb.map_sdefn().element_identifier);
    }
  } else if (TI_PLAIN_MAP_LARGE == tb.kind()) {
    if (TI_PLAIN_MAP_SMALL == ta.kind()) {
      return strongly_assignable(*ta.map_sdefn().key_identifier,
                                 *tb.map_ldefn().key_identifier) &&
        strongly_assignable(*ta.map_sdefn().element_identifier,
                            *tb.map_ldefn().element_identifier);
    } else { // TI_PLAIN_MAP_LARGE
      return strongly_assignable(*ta.map_ldefn().key_identifier,
                                 *tb.map_ldefn().key_identifier) &&
        strongly_assignable(*ta.map_ldefn().element_identifier,
                            *tb.map_ldefn().element_identifier);
    }
  } else if (EK_MINIMAL == tb.kind()) {
    const MinimalTypeObject& tob = lookup_minimal(tb);
    if (TK_MAP == tob.kind) {
      return assignable_plain_map(ta, tob);
    } else if (TK_ALIAS == tob.kind) {
      const TypeIdentifier& base = tob.alias_type.body.common.related_type;
      return assignable_plain_map(ta, base);
    }
  } else if (EK_COMPLETE == tb.kind()) {
    // Assuming tb.kind of EK_COMPLETE is not supported (similar to how
    // assignability for plain sequences and plain arrays are being handled)
    return false;
  }

  return false;
}

/**
 * @brief The first type must be a plain map type.
 *        The second type must not be TK_ALIAS.
 */
bool TypeAssignability::assignable_plain_map(const TypeIdentifier& ta,
                                             const MinimalTypeObject& tb) const
{
  if (TK_MAP == tb.kind) {
    if (TI_PLAIN_MAP_SMALL == ta.kind()) {
      return strongly_assignable(*ta.map_sdefn().key_identifier,
                                 tb.map_type.key.common.type) &&
        strongly_assignable(*ta.map_sdefn().element_identifier,
                            tb.map_type.element.common.type);
    } else { // TI_PLAIN_MAP_LARGE
      return strongly_assignable(*ta.map_ldefn().key_identifier,
                                 tb.map_type.key.common.type) &&
        strongly_assignable(*ta.map_ldefn().element_identifier,
                            tb.map_type.element.common.type);
    }
  }

  return false;
}

/**
 * @brief If types T1 and T2 are equivalent using the MINIMAL relation,
 * or alternatively if T1 is-assignable-from T2 and T2 is a delimited type,
 * then T1 is said to be strongly assignable from T2
 */
bool TypeAssignability::strongly_assignable(const TypeIdentifier& tia,
                                            const TypeIdentifier& tib) const
{
  if (equal_type_id(tia, tib)) {
    return true;
  }

  if (assignable(tia, tib) && is_delimited(tib)) {
    return true;
  }
  return false;
}

/**
 * @brief Check whether two type identifiers are equal
 */
bool TypeAssignability::equal_type_id(const TypeIdentifier& tia,
                                      const TypeIdentifier& tib) const
{
  switch (tia.kind()) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16: {
    if (tib.kind() == tia.kind()) {
      return true;
    }
    break;
  }
  case TI_STRING8_SMALL:
  case TI_STRING16_SMALL: {
    if (tib.kind() == tia.kind() && tib.string_sdefn().bound == tia.string_sdefn().bound) {
      return true;
    }
    break;
  }
  case TI_STRING8_LARGE:
  case TI_STRING16_LARGE: {
    if (tib.kind() == tia.kind() && tib.string_ldefn().bound == tia.string_ldefn().bound) {
      return true;
    }
    break;
  }
  case TI_PLAIN_SEQUENCE_SMALL: {
    if (tib.kind() == tia.kind() &&
        tib.seq_sdefn().bound == tia.seq_sdefn().bound &&
        equal_type_id(*tia.seq_sdefn().element_identifier,
                      *tib.seq_sdefn().element_identifier)) {
      return true;
    }
    break;
  }
  case TI_PLAIN_SEQUENCE_LARGE: {
    if (tib.kind() == tia.kind() &&
        tib.seq_ldefn().bound == tia.seq_ldefn().bound &&
        equal_type_id(*tia.seq_ldefn().element_identifier,
                      *tib.seq_ldefn().element_identifier)) {
      return true;
    }
    break;
  }
  case TI_PLAIN_ARRAY_SMALL: {
    if (tib.kind() == tia.kind()) {
      const SBoundSeq& bounds_a = tia.array_sdefn().array_bound_seq;
      const SBoundSeq& bounds_b = tia.array_sdefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        break;
      }
      bool equal_bounds = true;
      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          equal_bounds = false;
          break;
        }
      }
      if (!equal_bounds) {
        break;
      }
      if (equal_type_id(*tia.array_sdefn().element_identifier,
                        *tib.array_sdefn().element_identifier)) {
        return true;
      }
    }
    break;
  }
  case TI_PLAIN_ARRAY_LARGE: {
    if (tib.kind() == tia.kind()) {
      const LBoundSeq& bounds_a = tia.array_ldefn().array_bound_seq;
      const LBoundSeq& bounds_b = tib.array_ldefn().array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        break;
      }
      bool equal_bounds = true;
      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          equal_bounds = false;
          break;
        }
      }
      if (!equal_bounds) {
        break;
      }
      if (equal_type_id(*tia.array_ldefn().element_identifier,
                        *tib.array_ldefn().element_identifier)) {
        return true;
      }
    }
    break;
  }
  case TI_PLAIN_MAP_SMALL: {
    if (tib.kind() == tia.kind() &&
        tib.map_sdefn().bound == tia.map_sdefn().bound &&
        equal_type_id(*tia.map_sdefn().key_identifier,
                      *tib.map_sdefn().key_identifier) &&
        equal_type_id(*tia.map_sdefn().element_identifier,
                      *tib.map_sdefn().element_identifier)) {
      return true;
    }
    break;
  }
  case TI_PLAIN_MAP_LARGE: {
    if (tib.kind() == tia.kind() &&
        tib.map_ldefn().bound == tia.map_ldefn().bound &&
        equal_type_id(*tia.map_ldefn().key_identifier,
                      *tib.map_ldefn().key_identifier) &&
        equal_type_id(*tia.map_ldefn().element_identifier,
                      *tib.map_ldefn().element_identifier)) {
      return true;
    }
    break;
  }
  case TI_STRONGLY_CONNECTED_COMPONENT: {
    if (tib.kind() == tia.kind() &&
        tib.sc_component_id().scc_length == tia.sc_component_id().scc_length &&
        tib.sc_component_id().sc_component_id.kind ==
        tia.sc_component_id().sc_component_id.kind) {
      const EquivalenceHash& ha = tia.sc_component_id().sc_component_id.hash;
      const EquivalenceHash& hb = tib.sc_component_id().sc_component_id.hash;
      bool equal_hash = true;
      for (size_t i = 0; i < 14; ++i) {
        if (ha[i] != hb[i]) {
          equal_hash = false;
          break;
        }
      }
      if (equal_hash) {
        return true;
      }
    }
    break;
  }
  case EK_COMPLETE:
  case EK_MINIMAL: {
    if (tib.kind() == tia.kind()) {
      bool equal_hash = true;
      for (size_t i = 0; i < 14; ++i) {
        if (tia.equivalence_hash()[i] != tib.equivalence_hash()[i]) {
          equal_hash = false;
          break;
        }
      }
      if (equal_hash) {
        return true;
      }
    }
    break;
  }
  default:
    return false; // Future extensions
  }

  return false;
}

/**
 * @brief Concept of delimited types (sub-clause 7.2.4.2)
 */
bool TypeAssignability::is_delimited(const TypeIdentifier& ti) const
{
  switch (ti.kind()) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
  case TI_STRING8_SMALL:
  case TI_STRING8_LARGE:
  case TI_STRING16_SMALL:
  case TI_STRING16_LARGE:
    return true;
  case TI_PLAIN_SEQUENCE_SMALL:
    return is_delimited(*ti.seq_sdefn().element_identifier);
  case TI_PLAIN_SEQUENCE_LARGE:
    return is_delimited(*ti.seq_ldefn().element_identifier);
  case TI_PLAIN_ARRAY_SMALL:
    return is_delimited(*ti.array_sdefn().element_identifier);
  case TI_PLAIN_ARRAY_LARGE:
    return is_delimited(*ti.array_ldefn().element_identifier);
  case TI_PLAIN_MAP_SMALL:
    return is_delimited(*ti.map_sdefn().key_identifier) &&
      is_delimited(*ti.map_sdefn().element_identifier);
  case TI_PLAIN_MAP_LARGE:
    return is_delimited(*ti.map_ldefn().key_identifier) &&
      is_delimited(*ti.map_ldefn().element_identifier);
  case EK_COMPLETE:
  case EK_MINIMAL: {
    const MinimalTypeObject& tobj = lookup_minimal(ti);
    return is_delimited(tobj);
  }
  default:
    // Future extensions and strongly connected components
    return false;
  }
}

/**
 * @brief Check if a type is delimited (sub-clause 7.2.4.2)
 */
bool TypeAssignability::is_delimited(const MinimalTypeObject& tobj) const
{
  switch (tobj.kind) {
  case TK_ALIAS: {
    const TypeIdentifier& base = get_base_type(tobj);
    return is_delimited(base);
  }
  case TK_ANNOTATION:
    return is_delimited_with_flags(tobj.annotation_type.annotation_flag);
  case TK_STRUCTURE:
    return is_delimited_with_flags(tobj.struct_type.struct_flags);
  case TK_UNION:
    return is_delimited_with_flags(tobj.union_type.union_flags);
  case TK_BITSET:
    return is_delimited_with_flags(tobj.bitset_type.bitset_flags);
  case TK_SEQUENCE:
    return is_delimited(tobj.sequence_type.element.common.type);
  case TK_ARRAY:
    return is_delimited(tobj.array_type.element.common.type);
  case TK_MAP:
    return is_delimited(tobj.map_type.key.common.type) &&
      is_delimited(tobj.map_type.element.common.type);
  case TK_ENUM:
  case TK_BITMASK:
    return true;
  default:
    return false; // Future extensions
  }
}

bool TypeAssignability::is_delimited_with_flags(TypeFlag flags) const
{
  if ((flags & IS_FINAL) == IS_FINAL) {
    return false;
  } else if ((flags & IS_MUTABLE) == IS_MUTABLE) {
    // Mutable types are delimited with both encoding versions 1 and 2
    return true;
  } else { // Default extensibility is APPENDABLE (7.3.1.2.1.8)
    // Types with extensibility kind APPENDABLE are delimited
    // if serialized with encoding version 2 and are not
    // delimited if serialized with encoding version 1.
    // We are supporting XCDR2 in this iteration.
    return true;
  }
}

/**
 * @brief Key-Erased type of an aggregated type T (struct or union)
 * is constructed from T by removing the key designation from
 * any member that has it (sub-clause 7.2.2.4.6).
 * The input type must be either a struct or an union.
 */
void TypeAssignability::erase_key(MinimalTypeObject& type) const
{
  if (TK_STRUCTURE == type.kind) {
    MinimalStructMemberSeq& mseq = type.struct_type.member_seq;
    for (size_t i = 0; i < mseq.members.size(); ++i) {
      MemberFlag& flags = mseq.members[i].common.member_flags;
      if ((flags & IS_KEY) == IS_KEY) {
        flags &= ~IS_KEY;
      }
    }
  } else if (TK_UNION == type.kind) {
    MemberFlag& flags = type.union_type.discriminator.common.member_flags;
    if ((flags & IS_KEY) == IS_KEY) {
      flags &= ~IS_KEY;
    }
  }
}

/**
 * @brief Key-Holder type of an aggregated type T (struct or union)
 * is constructed from T (sub-clause 7.2.2.4.7)
 * The input MinimalTypeObject is modified to get the corresponding KeyHolder type.
 * The input must be either a struct or an union.
 */
void TypeAssignability::hold_key(MinimalTypeObject& type) const
{
  if (TK_STRUCTURE == type.kind) {
    MinimalStructMemberSeq& mseq = type.struct_type.member_seq;
    bool found_key = false;
    for (size_t i = 0; i < mseq.members.size(); ++i) {
      const MemberFlag& flags = mseq.members[i].common.member_flags;
      if ((flags & IS_KEY) == IS_KEY) {
        found_key = true;
        break;
      }
    }

    if (found_key) { // Remove all non-key members
      Sequence<MinimalStructMember> key_members;
      for (size_t i = 0; i < mseq.members.size(); ++i) {
        const MemberFlag& flags = mseq.members[i].common.member_flags;
        if ((flags & IS_KEY) == IS_KEY) {
          key_members.append(mseq.members[i]);
        }
      }
      mseq.members = key_members.members;
    } else { // Add a key designator to each member
      for (size_t i = 0; i < mseq.members.size(); ++i) {
        const MemberFlag& flags = mseq.members[i].common.member_flags;
        if ((flags & IS_KEY) != IS_KEY) {
          mseq.members[i].common.member_flags |= IS_KEY;
        }
      }
    }
  } else if (TK_UNION == type.kind) {
    if ((type.union_type.discriminator.common.member_flags & IS_KEY) == IS_KEY) {
      // Remove all non-key members
      type.union_type.member_seq = Sequence<MinimalUnionMember>();
    }
  }
}

/**
 * @brief Return false if the input type does not have type object; the output
 * MinimalTypeObject is not used in this case.
 * Return true if the input type has type object; the output MinimalTypeObject
 * contains the KeyHolder type of the corresponding type.
 */
bool TypeAssignability::hold_key(const TypeIdentifier& ti, MinimalTypeObject& to) const
{
  if (EK_MINIMAL != ti.kind() && EK_COMPLETE != ti.kind()) {
    return false;
  }

  to = lookup_minimal(ti);
  switch (to.kind) {
  case TK_STRUCTURE:
  case TK_UNION: {
    hold_key(to);
    return true;
  }
  case TK_ALIAS: {
    const TypeIdentifier& base = get_base_type(to);
    return hold_key(base, to);
  }
  default: // KeyHolder is not defined for other types
    return true;
  }
}

/**
 * @brief The input must be of type TK_ALIAS
 * Return the non-alias base type identifier of the input
 */
const TypeIdentifier& TypeAssignability::get_base_type(const MinimalTypeObject& type) const
{
  const TypeIdentifier& base = type.alias_type.body.common.related_type;
  switch (base.kind()) {
  case EK_COMPLETE:
  case EK_MINIMAL: {
    const MinimalTypeObject& type_obj = lookup_minimal(base);
    if (TK_ALIAS == type_obj.kind) {
      return get_base_type(type_obj);
    }
    return base;
  }
  default:
    return base;
  }
}

/**
 * @brief The first argument must be TK_ENUM and is the type object
 * of a key member of the containing struct. Therefore, there must be a
 * member with the same ID (and name) in the other struct type.
 */
bool TypeAssignability::struct_rule_enum_key(const MinimalTypeObject& tb,
                                             const CommonStructMember& ma) const
{
  if (EK_MINIMAL != ma.member_type_id.kind()) {
    return false;
  }

  const MinimalEnumeratedLiteralSeq& literals_b = tb.enumerated_type.literal_seq;
  const MinimalTypeObject& toa = lookup_minimal(ma.member_type_id);
  const MinimalEnumeratedLiteralSeq* literals_a = 0;
  if (TK_ENUM == toa.kind) {
    literals_a = &toa.enumerated_type.literal_seq;
  } else if (TK_ALIAS == toa.kind) {
    const TypeIdentifier& base_a = get_base_type(toa);
    if (EK_MINIMAL == base_a.kind()) {
      const MinimalTypeObject& base_obj_a = lookup_minimal(base_a);
      if (TK_ENUM == base_obj_a.kind) {
        literals_a = &base_obj_a.enumerated_type.literal_seq;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
    return false;
  }

  // All literals in tb must appear as literals in toa
  for (size_t j = 0; j < literals_b.members.size(); ++j) {
    const NameHash& h_b = literals_b.members[j].detail.name_hash;
    ACE_CDR::ULong key_b = (h_b[0] << 24) | (h_b[1] << 16) | (h_b[2] << 8) | (h_b[3]);
    bool found = false;
    for (size_t k = 0; k < literals_a->members.size(); ++k) {
      const NameHash& h_a = literals_a->members[k].detail.name_hash;
      ACE_CDR::ULong key_a = (h_a[0] << 24) | (h_a[1] << 16) | (h_a[2] << 8) | (h_a[3]);
      if (key_a == key_b) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Check whether a struct member is of sequence type and
 * if so compute its bound into the first argument
 */
bool TypeAssignability::get_sequence_bound(LBound& bound,
                                           const CommonStructMember& member) const
{
  ACE_CDR::Octet kind = member.member_type_id.kind();
  bool is_sequence = false;
  if (EK_MINIMAL == kind) {
    const MinimalTypeObject& tobj = lookup_minimal(member.member_type_id);
    if (TK_SEQUENCE == tobj.kind) {
      bound = tobj.sequence_type.header.common.bound;
      is_sequence = true;
    } else if (TK_ALIAS == tobj.kind) {
      const TypeIdentifier& base = get_base_type(tobj);
      if (EK_MINIMAL == base.kind()) {
        const MinimalTypeObject& base_obj = lookup_minimal(base);
        if (TK_SEQUENCE == base_obj.kind) {
          bound = base_obj.sequence_type.header.common.bound;
          is_sequence = true;
        }
      } else if (TI_PLAIN_SEQUENCE_SMALL == base.kind()) {
        bound = static_cast<LBound>(base.seq_sdefn().bound);
        is_sequence = true;
      } else if (TI_PLAIN_SEQUENCE_LARGE == base.kind()) {
        bound = base.seq_ldefn().bound;
        is_sequence = true;
      }
    }
  } else if (TI_PLAIN_SEQUENCE_SMALL == kind) {
    bound = static_cast<LBound>(member.member_type_id.seq_sdefn().bound);
    is_sequence = true;
  } else if (TI_PLAIN_SEQUENCE_LARGE == kind) {
    bound = member.member_type_id.seq_ldefn().bound;
    is_sequence = true;
  }
  return is_sequence;
}

/**
 * @brief Check whether a struct member is of map type and if so
 * compute its bound into the first argument
 */
bool TypeAssignability::get_map_bound(LBound& bound,
                                      const CommonStructMember& member) const
{
  ACE_CDR::Octet kind = member.member_type_id.kind();
  bool is_map = false;
  if (EK_MINIMAL == kind) {
    const MinimalTypeObject& tobj = lookup_minimal(member.member_type_id);
    if (TK_MAP == tobj.kind) {
      bound = tobj.map_type.header.common.bound;
      is_map = true;
    } else if (TK_ALIAS == tobj.kind) {
      const TypeIdentifier& base = get_base_type(tobj);
      if (EK_MINIMAL == base.kind()) {
        const MinimalTypeObject& base_obj = lookup_minimal(base);
        if (TK_MAP == base_obj.kind) {
          bound = base_obj.map_type.header.common.bound;
          is_map = true;
        }
      } else if (TI_PLAIN_MAP_SMALL == base.kind()) {
        bound = static_cast<LBound>(base.map_sdefn().bound);
        is_map = true;
      } else if (TI_PLAIN_MAP_LARGE == base.kind()) {
        bound = base.map_ldefn().bound;
        is_map = true;
      }
    }
  } else if (TI_PLAIN_MAP_SMALL == kind) {
    bound = static_cast<LBound>(member.member_type_id.map_sdefn().bound);
    is_map = true;
  } else if (TI_PLAIN_MAP_LARGE == kind) {
    bound = member.member_type_id.map_ldefn().bound;
    is_map = true;
  }
  return is_map;
}

/**
 * @brief Check whether the input struct member is of string type and
 * if so compute its bound into the first argument
 */
bool TypeAssignability::get_string_bound(LBound& bound,
                                         const CommonStructMember& member) const
{
  ACE_CDR::Octet kind = member.member_type_id.kind();
  bool is_string = false;
  if (EK_MINIMAL == kind) {
    const MinimalTypeObject& tobj = lookup_minimal(member.member_type_id);
    if (TK_ALIAS == tobj.kind) {
      const TypeIdentifier& base = get_base_type(tobj);
      if (TI_STRING8_SMALL == base.kind() || TI_STRING16_SMALL == base.kind()) {
        bound = static_cast<LBound>(base.string_sdefn().bound);
        is_string = true;
      } else if (TI_STRING8_LARGE == base.kind() || TI_STRING16_LARGE == base.kind()) {
        bound = base.string_ldefn().bound;
        is_string = true;
      }
    }
  } else if (TI_STRING8_SMALL == kind || TI_STRING16_SMALL == kind) {
    bound = static_cast<LBound>(member.member_type_id.string_sdefn().bound);
    is_string = true;
  } else if (TI_STRING8_LARGE == kind || TI_STRING16_LARGE == kind) {
    bound = member.member_type_id.string_ldefn().bound;
    is_string = true;
  }
  return is_string;
}

/**
 * @brief Check if the second argument is of a struct type and if so
 * return its type object as the first argument
 */
bool TypeAssignability::get_struct_member(const MinimalTypeObject*& ret,
                                          const CommonStructMember& member) const
{
  ACE_CDR::Octet kind = member.member_type_id.kind();
  bool is_struct = false;
  if (EK_MINIMAL == kind) {
    const MinimalTypeObject& tobj = lookup_minimal(member.member_type_id);
    if (TK_STRUCTURE == tobj.kind) {
      ret = &tobj;
      is_struct = true;
    } else if (TK_ALIAS == tobj.kind) {
      const TypeIdentifier& base = get_base_type(tobj);
      if (EK_MINIMAL == base.kind()) {
        const MinimalTypeObject& base_obj = lookup_minimal(base);
        if (TK_STRUCTURE == base_obj.kind) {
          ret = &base_obj;
          is_struct = true;
        }
      }
    }
  }
  return is_struct;
}

/**
 * @brief Check if the second argument is of a union type and if so
 * return its type object as the first argument
 */
bool TypeAssignability::get_union_member(const MinimalTypeObject*& ret,
                                         const CommonStructMember& member) const
{
  ACE_CDR::Octet kind = member.member_type_id.kind();
  bool is_union = false;
  if (EK_MINIMAL == kind) {
    const MinimalTypeObject& tobj = lookup_minimal(member.member_type_id);
    if (TK_UNION == tobj.kind) {
      ret = &tobj;
      is_union = true;
    } else if (TK_ALIAS == tobj.kind) {
      const TypeIdentifier& base = get_base_type(tobj);
      if (EK_MINIMAL == base.kind()) {
        const MinimalTypeObject& base_obj = lookup_minimal(base);
        if (TK_UNION == base_obj.kind) {
          ret = &base_obj;
          is_union = true;
        }
      }
    }
  }
  return is_union;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
