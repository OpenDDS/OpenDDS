/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeAssignability.h"

#include <map>
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
        return assignable_sequence(ta.minimal, tb.minimal);
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
   * @brief Both input can be of any type
   */
  bool TypeAssignability::assignable(const TypeIdentifier& ta,
                                     const TypeIdentifier& tb) const
  {
    switch (ta.kind) {
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
    case EK_MINIMAL:
      const MinimalTypeObject& base_type_a = lookup_minimal(ta);
      const MinimalTypeObject& base_type_b = lookup_minimal(tb);
      TypeObject wrapper_a(base_type_a), wrapper_b(base_type_b);
      return assignable(wrapper_a, wrapper_b);
    default:
      return false; // Future extensions
    }
  }

  /**
   * @brief At least one input type object must be TK_ALIAS
   */
  bool TypeAssignability::assignable_alias(const MinimalTypeObject& ta,
                                           const MinimalTypeObject& tb) const
  {
    if (TK_ALIAS == ta.kind && TK_ALIAS != tb.kind) {
      const TypeIdentifier& tia = *ta.alias_type.body.common.related_type.in();
      switch (tia.kind) {
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
      case EK_MINIMAL:
        const MinimalTypeObject& base_type_a = lookup_minimal(tia);
        TypeObject wrapper_a(base_type_a), wrapper_b(tb);
        return assignable(wrapper_a, wrapper_b);
      default:
        return false; // Future extensions
      }
    } else if (TK_ALIAS != ta.kind && TK_ALIAS == tb.kind) {
      const TypeIdentifier& tib = *tb.alias_type.body.common.related_type.in();
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
      const TypeIdentifier& tia = *ta.alias_type.body.common.related_type.in();
      const TypeIdentifier& tib = *tb.alias_type.body.common.related_type.in();
      return assignable(tia, tib);
    }

    return false;
  }

  /**
   * @brief The first type must be TK_ANNOTATION.
   *        The second type must not be TK_ALIAS.
   */
  bool TypeAssignability::assignable_annotation(const MinimalTypeObject& ta,
                                                const MinimalTypeObject& tb) const
  {
    // No rule for annotation in the spec
    return false;
  }

  /**
   * @brief The first type must be TK_ANNOTATION.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_annotation(const MinimalTypeObject& ta,
                                                const TypeIdentifier& tb) const
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
    TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
    if (ta.struct_type.struct_flags & extensibility_mask !=
        tb.struct_type.struct_flags & extensibility_mask) {
      return false;
    }

    // Any members in T1 and T2 that have the same name also have
    // the same ID, and vice versa
    MatchedSet matched_members;
    for (size_t i = 0; i < ta.struct_type.member_seq.members.size(); ++i) {
      MemberId id_a = ta.struct_type.member_seq.members[i].common.member_id;
      NameHash h_a = ta.struct_type.member_seq.members[i].detail.name_hash;
      ACE_CDR::ULong
        name_a = (h_a[0] << 24) | (h_a[1] << 16) | (h_a[2] << 8) | (h_a[3]);
      for (size_t j = 0; j < tb.struct_type.member_seq.members.size(); ++j) {
        MemberId id_b = tb.struct_type.member_seq.members[j].common.member_id;
        NameHash h_b = tb.struct_type.member_seq.members[j].detail.name_hash;
        ACE_CDR::ULong
          name_b = (h_b[0] << 24) | (h_b[1] << 16) | (h_b[2] << 8) | (h_b[3]);
        if ((name_a == name_b && id_a != id_b) ||
            (id_a == id_b && name_a != name_b)) {
          return false;
        } else if (name_a == name_b && id_a == id_b) {
          matched_members.push_back(std::pair<const MinimalStructMember*,
                                    const MinimalStructMember*>
                                    (&ta.struct_type.member_seq.members[i],
                                     &tb.struct_type.member_seq.members[j]));
          break;
        }
      }
    }

    // There is at least one member m1 of T1 and one corresponding member
    // m2 of T2 such that m1.id == m2.id.
    if (matched_members.size() == 0) {
      return false;
    }

    // For any member m2 of T2, if there is a member m1 of T1 with the same
    // ID, then the type KeyErased(m1.type) is-assignable-from the type
    // KeyErased(m2.type)
    for (size_t i = 0; i < matched_members.size(); ++i) {
      const TypeIdentifier&
        tia = *matched_members[i].first->common.member_type_id.in();
      const TypeIdentifier&
        tib = *matched_members[i].second->common.member_type_id.in();
      // KeyErased(T) is only defined for structs and unions
      if (EK_MINIMAL == tia.kind && EK_MINIMAL == tib.kind) {
        const MinimalTypeObject& toa = lookup_minimal(tia);
        const MinimalTypeObject& tob = lookup_minimal(tib);
        if ((TK_STRUCTURE == toa.kind || TK_UNION == toa.kind) &&
            (TK_STRUCTURE == tob.kind || TK_UNION == tob.kind)) {
          MinimalTypeObject key_erased_a = toa, key_erased_b = tob;
          erase_key(key_erased_a);
          erase_key(key_erased_b);
          if (!assignable(key_erased_a, key_erased_b)) {
            return false;
          }
        }
      }
    }

    // Members for which both optional is false and must_understand is true in
    // either T1 or T2 appear in both T1 and T2.
    // Members marked as key in either T1 or T2 appear in both T1 and T2.
    for (size_t i = 0; i < ta.struct_type.member_seq.members.size(); ++i) {
      MemberFlag flags = ta.struct_type.member_seq.members[i].common.member_flags;
      MemberId id = ta.struct_type.member_seq.members[i].common.member_id;
      bool found = false;
      if (flags & IS_OPTIONAL == 0 &&
          flags & IS_MUST_UNDERSTAND == IS_MUST_UNDERSTAND) {
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
      if (flags & IS_KEY == IS_KEY) {
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

    for (size_t i = 0; i < tb.struct_type.member_seq.members.size(); ++i) {
      MemberFlag flags = tb.struct_type.member_seq.members[i].common.member_flags;
      MemberId id = tb.struct_type.member_seq.members[i].common.member_id;
      bool found = false;
      if (flags & IS_OPTIONAL == 0 &&
          flags & IS_MUST_UNDERSTAND == IS_MUST_UNDERSTAND) {
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
      if (flags & IS_KEY == IS_KEY) {
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
    // TODO: Don't need to loop over all tb members, just loop over
    // all matched members since all key members of both T1 and T2 appear
    // in the set of matched members
    for (size_t i = 0; i < tb.struct_type.member_seq.members.size(); ++i) {
      MemberFlag flags = tb.struct_type.member_seq.members[i].common.member_flags;
      MemberId id = tb.struct_type.member_seq.members[i].common.member_id;
      const TypeIdentifier&
        tib = *tb.struct_type.member_seq.members[i].common.member_type_id.in();
      // Can tib be an alias of a string type?
      if ((TI_STRING8_SMALL == tib.kind || TI_STRING16_SMALL == tib.kind ||
           TI_STRING8_LARGE == tib.kind || TI_STRING16_LARGE == tib.kind) &&
          flags & IS_KEY == IS_KEY) {
        LBound bound_b;
        if (TI_STRING8_SMALL == tib.kind || TI_STRING16_SMALL == tib.kind) {
          bound_b = static_cast<LBound> (tib.string_sdefn.bound);
        } else {
          bound_b = tib.string_ldefn.bound;
        }

        for (size_t j = 0; j < matched_members.size(); ++j) {
          if (id == matched_members[j].first->common.member_id) {
            const TypeIdentifier&
              tia =  *matched_members[j].first->common.member_type_id.in();
            // Can tia be an alias of a string type?
            if (TI_STRING8_SMALL != tia.kind && TI_STRING16_SMALL != tia.kind &&
                TI_STRING8_LARGE != tia.kind && TI_STRING16_LARGE != tia.kind) {
              return false;
            }
            LBound bound_a;
            if (TI_STRING8_SMALL == tia.kind || TI_STRING16_SMALL == tia.kind) {
              bound_a = static_cast<LBound> (tia.string_sdefn.bound);
            } else { // TI_STRING8_LARGE or TI_STRING16_LARGE
              bound_a = tia.string_ldefn.bound;
            }

            if (bound_a < bound_b) {
              return false;
            }
            break;
          }
        }
      }
    }

    // For any enumerated key member m2 in T2, the m1 member of T1 with
    // the same member ID verifies that all literals in m2.type appear as
    // literals in m1.type
    for (size_t i = 0; i < matched_members.size(); ++i) {
      const CommonStructMember& member = matched_members[i].second->common;
      MemberFlag flags = member.member_flags;
      if (flags & IS_KEY == IS_KEY &&
          EK_MINIMAL == member.member_type_id->kind) {
        const MinimalTypeObject& tob = lookup_minimal(*member.member_type_id.in());
        if (TK_ENUM == tob.kind) {
          if (!struct_rule_enum_key(tob, matched_members[i].first->common)) {
            return false;
          }
        } else if (TK_ALIAS == tob.kind) {
          const TypeIdentifier& base_b = get_base_type(tob);
          if (EK_MINIMAL == base_b.kind) {
            const MinimalTypeObject& base_obj_b = lookup_minimal(base_b);
            if (TK_ENUM == base_obj_b.kind &&
                !struct_rule_enum_key(base_obj_b,
                                            matched_members[i].first->common)) {
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
      MemberFlags flags = member.member_flags;
      ACE_CDR::Octet kind = member.member_type_id->kind;
      LBound bound_b = 0;
      bool is_sequence = false, is_map = false;
      if (flags & IS_KEY == IS_KEY) {
        const CommonStructMember& mem_a = matched_members[i].first->common;
        if (EK_MINIMAL == kind) {
          const MinimalTypeObject& tob = lookup_minimal(*member.member_type_id.in());
          if (TK_SEQUENCE == tob.kind) {
            bound_b = tob.sequence_type.header.common.bound;
            is_sequence = true;
          } else if (TK_MAP == tob.kind) {
            bound_b = tob.map_type.header.common.bound;
            is_map = true;
          } else if (TK_ALIAS == tob.kind) {
            const TypeIdentifier& base_b = get_base_type(tob);
            if (EK_MINIMAL == base_b.kind) {
              const MinimalTypeObject& base_obj_b = lookup_minimal(base_b);
              if (TK_SEQUENCE == base_obj_b.kind) {
                bound_b = base_obj_b.sequence_type.header.common.bound;
                is_sequence = true;
              } else if (TK_MAP == base_obj_b.kind) {
                bound_b = base_obj_b.map_type.header.common.bound;
                is_map = true;
              }
            } else if (TI_PLAIN_SEQUENCE_SMALL == base_b.kind) {
              bound_b = static_cast<LBound>(base_b.member_type_id->seq_sdefn.bound);
              is_sequence = true;
            } else if (TI_PLAIN_SEQUENCE_LARGE == base_b.kind) {
              bound_b = base_b.member_type_id->seq_ldefn.bound;
              is_sequence = true;
            } else if (TI_PLAIN_MAP_SMALL == base_b.kind) {
              bound_b = static_cast<LBound>(base_b.member_type_id->map_sdefn.bound);
              is_map = true;
            } else if (TI_PLAIN_MAP_LARGE == base_b.kind) {
              bound_b = base_b.member_type_id->map_ldefn.bound;
              is_map = true;
            }
          }
        } else if (TI_PLAIN_SEQUENCE_SMALL == kind) {
          bound_b = static_cast<LBound>(member.member_type_id->seq_sdefn.bound);
          is_sequence = true;
        } else if (TI_PLAIN_SEQUENCE_LARGE == kind) {
          bound_b = member.member_type_id->seq_ldefn.bound;
          is_sequence = true;
        } else if (TI_PLAIN_MAP_SMALL == kind) {
          bound_b = static_cast<LBound>(member.member_type_id->map_sdefn.bound);
          is_map = true;
        } else if (TI_PLAIN_MAP_LARGE == kind) {
          bound_b = member.member_type_id->map_ldefn.bound;
          is_map = true;
        }

        if (is_sequence && !struct_rule_seq_key(bound_b, mem_a)) {
          return false;
        }
        if (is_map && !struct_rule_map_key(bound_b, mem_a)) {
          return false;
        }
      } // IS_KEY
    }

    // For any structure or union key member m2 in T2, the m1 member
    // of T1 with the same member ID verifies that KeyHolder(m1.type)
    // is-assignable-from KeyHolder(m2.type)

    return true;
  }

  /**
   * @brief The first type must be TK_STRUCTURE.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
  {
    if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_STRUCTURE == tob.kind) {
        return assignable_struct(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_struct(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
    TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
    if (ta.union_type.union_flags & extensibility_mask !=
        tb.union_type.union_flags & extensibility_mask) {
      return false;
    }

    std::set<ACE_CDR::Long> labels_set_a;
    for (size_t i = 0; i < ta.union_type.member_seq.members.size(); ++i) {
      const UnionCaseLabelSeq&
        labels_a = ta.union_type.member_seq.members[i].common.label_seq;
      for (size_t j = 0; j < labels_a.members.size(); ++j) {
        labels_set_a.insert(labels_a.members[j]);
      }
    }

    // If extensibility is final, then the set of labels must be identical.
    // Assuming labels are mapped to values identically in both input types.
    if (ta.union_type.union_flags & extensibility_mask == IS_FINAL) {
      for (size_t i = 0; i < tb.union_type.member_seq.members.size(); ++i) {
        const UnionCaseLabelSeq&
          labels_b = tb.union_type.member_seq.members[i].common.label_seq;
        for (size_t j = 0; j < labels_b.members.size(); ++j) {
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
      // NOTE: We assume that there is a separate CommonUnionMember
      // for the default label (if it is present) in cases where
      // there are multiple labels for the default member. For instance,
      // if "LABEL1", "LABEL2", and "default" are the labels for the
      // default member, then there is one CommonUnionMember for "LABEL1"
      // and "LABEL2", and another CommonUnionMember for "default", both
      // represent the default member. The CommonUnionMember with "default"
      // label is the only one that has IS_DEFAULT flag turned on.
      bool found = false;
      for (size_t i = 0; i < tb.union_type.member_seq.members.size(); ++i) {
        const UnionMemberFlag&
          flags_b = tb.union_type.member_seq.members[i].common.member_flags;
        if (flags_b & IS_DEFAULT != IS_DEFAULT) {
          const UnionCaseLabelSeq&
            labels_b = tb.union_type.member_seq.members[i].common.label_seq;
          for (size_t j = 0; j < labels_b.members.size(); ++j) {
            if (labels_set_a.find(labels_b.members[j]) != labels_set_a.end()) {
              found = true;
              break;
            }
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
    const TypeIdentifier& tia = *ta.union_type.discriminator.common.type_id.in();
    const TypeIdentifier& tib = *tb.union_type.discriminator.common.type_id.in();
    if (!strongly_assignable(tia, tib)) {
      return false;
    }

    // Both discriminators are keys or neither are keys
    MemberFlag flags_a = ta.union_type.discriminator.common.member_flags;
    MemberFlag flags_b = tb.union_type.discriminator.common.member_flags;
    if (((flags_a & IS_KEY == IS_KEY) && (flags_b & IS_KEY != IS_KEY)) ||
        ((flags_a & IS_KEY != IS_KEY) && (flags_b & IS_KEY == IS_KEY))) {
      return false;
    }

    // Members with the same id must have the same name, and vice versa
    std::map<MemberId, NameHash> id_to_name_a;
    std::map<ACE_CDR::ULong, MemberId> name_to_id_a;
    for (size_t i = 0; i < ta.union_type.member_seq.members.size(); ++i) {
      MemberId id = ta.union_type.member_seq.members[i].common.member_id;
      NameHash h = ta.union_type.member_seq.members[i].detail.name_hash;
      ACE_CDR::ULong name = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
      id_to_name_a[id] = name;
      name_to_id_a[name] = id;
    }

    for (size_t i = 0; i < tb.union_type.member_seq.members.size(); ++i) {
      MemberId id = tb.union_type.member_seq.members[i].common.member_id;
      NameHash h = tb.union_type.member_seq.members[i].detail.name_hash;
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

    // For all non-default labels in T2 that select some member in T1,
    // the type of the selected member in T1 is assignable from the
    // type of the T2 member
    for (size_t i = 0; i < tb.union_type.member_seq.members.size(); ++i) {
      UnionMemberFlag
        flags_b = tb.union_type.member_seq.members[i].common.member_flags;
      if (flags_b & IS_DEFAULT != IS_DEFAULT) {
        const UnionCaseLabelSeq&
          label_seq_b = tb.union_type.member_seq.members[i].common.label_seq;
        for (size_t j = 0; j < ta.union_type.member_seq.members.size(); ++j) {
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
          const UnionCaseLabelSeq&
            label_seq_a = ta.union_type.member_seq.members[j].common.label_seq;
          bool matched = false;
          for (size_t k = 0; k < label_seq_b.members.size(); ++k) {
            for (size_t t = 0; t < label_seq_a.members.size(); ++t) {
              if (label_seq_b.members[k] == label_seq_a.members[t]) {
                const TypeIdentifier&
                  tia = *ta.union_type.member_seq.members[j].common.type_id.in();
                const TypeIdentifier&
                  tib = *tb.union_type.member_seq.members[i].common.type_id.in();
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
    }

    // If any non-default labels of T1 that select the default member of T2,
    // the type of the member in T1 is assignable from the type of the default
    // member in T2
    for (size_t i = 0; i < ta.union_type.member_seq.members.size(); ++i) {
      UnionMemberFlag
        flags_a = ta.union_type.member_seq.members[i].common.member_flags;
      if (flags_a & IS_DEFAULT != IS_DEFAULT) {
        const UnionCaseLabelSeq&
          label_seq_a = ta.union_type.member_seq.members[i].common.label_seq;
        for (size_t j = 0; j < tb.union_type.member_seq.members.size(); ++j) {
          UnionMemberFlag
            flags_b = tb.union_type.member_seq.members[j].common.member_flags;
          if (flags_b & IS_DEFAULT == IS_DEFAULT) {
            const UnionCaseLabelSeq&
              label_seq_b = tb.union_type.member_seq.members[j].common.label_seq;
            bool matched = false;
            for (size_t k = 0; k < label_seq_a.members.size(); ++k) {
              for (size_t t = 0; t < label_seq_b.members.size(); ++t) {
                if (label_seq_a.members[k] == label_seq_b.members[t]) {
                  const TypeIdentifier&
                    tia = *ta.union_type.member_seq.members[i].common.type_id.in();
                  const TypeIdentifier&
                    tib = *tb.union_type.member_seq.members[j].common.type_id.in();
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
      }
    }

    // If T1 and T2 both have default labels, the type of T1's default member
    // is assignable from the type of T2's default member
    for (size_t i = 0; i < ta.union_type.member_seq.members.size(); ++i) {
      UnionMemberFlag
        flags_a = ta.union_type.member_seq.members[i].common.member_flags;
      if (flags_a & IS_DEFAULT == IS_DEFAULT) {
        for (size_t j = 0; j < tb.union_type.member_seq.members.size(); ++j) {
          UnionMemberFlag
            flags_b = tb.union_type.member_seq.members[j].common.member_flags;
          if (flags_b & IS_DEFAULT == IS_DEFAULT) {
            const TypeIdentifier&
              tia = *ta.union_type.member_seq.members[i].common.type_id.in();
            const TypeIdentifier&
              tib = *tb.union_type.member_seq.members[j].common.type_id.in();
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
    if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_UNION == tob.kind) {
        return assignable_union(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_union(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
      // Assuming tb.kind of EK_COMPLETE is not supported
      return false;
    }

    return false;
  }

  /**
   * @brief The first type must be TK_BITSET.
   *        The second type must not be TK_ALIAS.
   */
  bool TypeAssignability::assignable_bitset(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
  {
    // No rule for bitset in the spec
    return false;
  }

  /**
   * @brief The first type must be TK_BITSET.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_bitset(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
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

    return strongly_assignable(*ta.sequence_type.element.common.type.in(),
                               *tb.sequence_type.element.common.type.in());
  }

  /**
   * @brief The first type must be TK_SEQUENCE.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                              const TypeIdentifier& tb) const
  {
    if (TI_PLAIN_SEQUENCE_SMALL == tb.kind) {
      return strongly_assignable(*ta.element.common.type.in(),
                                 *tb.seq_sdefn.element_identifier.in());
    } else if (TI_PLAIN_SEQUENCE_LARGE == tb.kind) {
      return strongly_assignable(*ta.element.common.type.in(),
                                 *tb.seq_ldefn.element_identifier.in());
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_SEQUENCE == tob.kind) {
        return assignable_sequence(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_sequence(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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

    for (size_t i = 0; i < bounds_a.members.size(); ++i) {
      if (bounds_a.members[i] != bounds_b.members[i]) {
        return false;
      }
    }

    return strongly_assignable(*ta.array_type.element.common.type.in(),
                               *tb.array_type.element.common.type.in());
  }

  /**
   * @brief The first type must be TK_ARRAY.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_array(const MinimalTypeObject& ta,
                                           const TypeIdentifier& tb) const
  {
    const LBoundSeq& bounds_a = ta.array_type.header.common.bound_seq;
    if (TI_PLAIN_ARRAY_SMALL == tb.kind) {
      SBoundSeq bounds_b = tb.array_sdefn.array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != static_cast<LBound>(bounds_b.members[i])) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_type.element.common.type.in(),
                                 *tb.array_sdefn.element_identifier.in());
    } else if (TI_PLAIN_ARRAY_LARGE == tb.kind) {
      LBoundSeq bounds_b = tb.array_ldefn.array_bound_seq;
      if (bounds_a.members.size() != bounds_b.members.size()) {
        return false;
      }

      for (size_t i = 0; i < bounds_a.members.size(); ++i) {
        if (bounds_a.members[i] != bounds_b.members[i]) {
          return false;
        }
      }

      return strongly_assignable(*ta.array_type.element.common.type.in(),
                                 *tb.array_ldefn.element_identifier.in());
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_ARRAY == tob.kind) {
        return assignable_array(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_array(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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

    return strongly_assignable(*ta.map_type.key.common.type.in(),
                               *tb.map_type.key.common.type.in()) &&
      strongly_assignable(*ta.map_type.element.common.type.in(),
                          *tb.map_type.element.common.type.in());
  }

  /**
   * @brief The first type must be TK_MAP.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                         const TypeIdentifier& tb) const
  {
    if (TI_PLAIN_MAP_SMALL == tb.kind) {
      return strongly_assignable(*ta.map_type.key.common.type.in(),
                                 *tb.map_sdefn.key_identifier.in()) &&
        strongly_assignable(*ta.map_type.element.common.type.in(),
                            *tb.map_sdefn.element_identifier.in());
    } else if (TI_PLAIN_MAP_LARGE == tb.kind) {
      return strongly_assignable(*ta.map_type.key.common.type.in(),
                                 *tb.map_ldefn.key_identifier.in()) &&
        strongly_assignable(*ta.map_type.element.common.type.in(),
                            *tb.map_ldefn.element_identifier.in());
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_MAP == tob.kind) {
        return assignable_map(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_map(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
    TypeFlag extensibility_mask = IS_FINAL | IS_APPENDABLE | IS_MUTABLE;
    TypeFlag ta_ext = ta.enumerated_type.enum_flags & extensibility_mask;
    TypeFlag tb_ext = tb.enumerated_type.enum_flags & extensibility_mask;
    if (ta_ext != tb_ext) {
      return false;
    }

    std::map<ACE_CDR::ULong, ACE_CDR::Long> ta_maps;

    // If extensibility is FINAL, both must have the same literals.
    if (IS_FINAL == ta_ext) {
      size_t size_a = ta.enumerated_type.literal_seq.members.size();
      size_t size_b = tb.enumerated_type.literal_seq.members.size();
      if (size_a != size_b) {
        return false;
      }

      for (size_t i = 0; i < size_a; ++i) {
        NameHash h = ta.enumerated_type.literal_seq.members[i].detail.name_hash;
        ACE_CDR::ULong key_a = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
        ta_maps[key_a] = ta.enumerated_type.literal_seq.members[i].common.value;
      }

      for (size_t i = 0; i < size_b; ++i) {
        NameHash h = tb.enumerated_type.literal_seq.members[i].detail.name_hash;
        ACE_CDR::ULong key_b = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);

        // Literals that have the same name must have the same value.
        if (ta_maps.find(key_b) == ta_maps.end() ||
            ta_maps[key_b] != tb.enumerated_type.literal_seq.members[i].common.value) {
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
    if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_ENUM == tob.kind) {
        return assignable_enum(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_enum(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
    if (TK_UINT8 == tb.kind) {
      return 1 <= ta_bit_bound && ta_bit_bound <= 8;
    } else if (TK_UINT16 == tb.kind) {
      return 9 <= ta_bit_bound && ta_bit_bound <= 16;
    } else if (TK_UINT32 == tb.kind) {
      return 17 <= ta_bit_bound && ta_bit_bound <= 32;
    } else if (TK_UINT64 == tb.kind) {
      return 33 <= ta_bit_bound && ta_bit_bound <= 64;
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_BITMASK == tob.kind) {
        return assignable_bitmask(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_bitmask(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
      // Assuming tb.kind of EK_COMPLETE is not supported
      return false;
    }

    return false;
  }

  /**
   * @brief The first type must be a future extension type kind.
   *        The second type must not be TK_ALIAS.
   */
  bool TypeAssignability::assignable_extended(const MinimalTypeObject& ta,
                                              const MinimalTypeObject& tb) const
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
    if (ta.kind == tb.kind) {
      return true;
    }

    if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_BITMASK == tob.kind) {
        return assignable_primitive(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_primitive(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
        !(TK_UINT8 == ta.kind || TK_UINT16 == ta.kind ||
          TK_UINT32 == ta.kind || TK_UINT64 == ta.kind)) {
      return false;
    }

    BitBound bit_bound = tb.bitmask_type.header.common.bit_bound;
    if (TK_UINT8 == ta.kind) {
      return 1 <= bit_bound && bit_bound <= 8;
    } else if (TK_UINT16 == ta.kind) {
      return 9 <= bit_bound && bit_bound <= 16;
    } else if (TK_UINT32 == ta.kind) {
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
    if (TI_STRING8_SMALL == tb.kind || TI_STRING8_LARGE == tb.kind) {
      if (TI_STRING8_SMALL == ta.kind || TI_STRING8_LARGE == ta.kind) {
        return true;
      }
    } else if (TI_STRING16_SMALL == tb.kind || TI_STRING16_LARGE == tb.kind) {
      if (TI_STRING16_SMALL == ta.kind || TI_STRING16_LARGE == ta.kind) {
        return true;
      }
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_string(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
      // Assuming tb.kind of EK_COMPLETE is not supported
      return false;
    }

    return false;
  }

  /**
   * @brief The first type must be a string type.
   *        The second type must not be TK_ALIAS.
   */
  bool TypeAssignability::assignable_string(const TypeIdentifier& ta,
                                            const MinimalTypeObject& tb) const
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
    if (TI_PLAIN_SEQUENCE_SMALL == tb.kind) {
      if (TI_PLAIN_SEQUENCE_SMALL == ta.kind) {
        return strongly_assignable(*ta.seq_sdefn.element_identifier.in(),
                                   *tb.seq_sdefn.element_identifier.in());
      } else { // TI_PLAIN_SEQUENCE_LARGE
        return strongly_assignable(*ta.seq_ldefn.element_identifier.in(),
                                   *tb.seq_sdefn.element_identifier.in());
      }
    } else if (TI_PLAIN_SEQUENCE_LARGE == tb.kind) {
      if (TI_PLAIN_SEQUENCE_SMALL == ta.kind) {
        return strongly_assignable(*ta.seq_sdefn.element_identifier.in(),
                                   *tb.seq_ldefn.element_identifier.in());
      } else { // TI_PLAIN_SEQUENCE_LARGE
        return strongly_assignable(*ta.seq_ldefn.element_identifier.in(),
                                   *tb.seq_ldefn.element_identifier.in());
      }
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_SEQUENCE == tob.kind) {
        return assignable_plain_sequence(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_plain_sequence(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
      // TODO: Can tb.kind be EK_COMPLETE? More generally, can a MinimalTypeObject
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
      if (TI_PLAIN_SEQUENCE_SMALL == ta.kind) {
        return strongly_assignable(*ta.seq_sdefn.element_identifier.in(),
                                   *tb.sequence_type.element.common.type.in());
      } else { // TI_PLAIN_SEQUENCE_LARGE
        return strongly_assignable(*ta.seq_ldefn.element_identifier.in(),
                                   *tb.sequence_type.element.common.type.in());
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
    if (TI_PLAIN_ARRAY_SMALL == tb.kind) {
      if (TI_PLAIN_ARRAY_SMALL == ta.kind) {
        Sequence<SBound> bounds_a = ta.array_sdefn.array_bound_seq;
        Sequence<SBound> bounds_b = tb.array_sdefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (bounds_a[i] != bounds_b[i]) {
            return false;
          }
        }

        return strongly_assignable(*ta.array_sdefn.element_identifier.in(),
                                   *tb.array_sdefn.element_identifier.in());
      } else { // TI_PLAIN_ARRAY_LARGE
        Sequence<LBound> bounds_a = ta.array_ldefn.array_bound_seq;
        Sequence<SBound> bounds_b = tb.array_sdefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (bounds_a[i] != static_cast<LBound>(bounds_b[i])) {
            return false;
          }
        }

        return strongly_assignable(*ta.array_ldefn.element_identifier.in(),
                                   *tb.array_sdefn.element_identifier.in());
      }
    } else if (TI_PLAIN_ARRAY_LARGE == tb.kind) {
      if (TI_PLAIN_ARRAY_SMALL == ta.kind) {
        Sequence<SBound> bounds_a = ta.array_sdefn.array_bound_seq;
        Sequence<LBound> bounds_b = tb.array_ldefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (static_cast<LBound>(bounds_a[i]) != bounds_b[i]) {
            return false;
          }
        }

        return strongly_assignable(*ta.array_sdefn.element_identifier.in(),
                                   *tb.array_ldefn.element_identifier.in());
      } else { // TI_PLAIN_ARRAY_LARGE
        Sequence<LBound> bounds_a = ta.array_ldefn.array_bound_seq;
        Sequence<LBound> bounds_b = tb.array_ldefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (bounds_a[i] != bounds_b[i]) {
            return false;
          }
        }

        return strongly_assignable(*ta.array_ldefn.element_identifier.in(),
                                   *tb.array_ldefn.element_identifier.in());
      }
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_ARRAY == tob.kind) {
        return assignable_plain_array(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_plain_array(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
      Sequence<LBound> bounds_b = tb.array_type.header.common.bound_seq;
      if (TI_PLAIN_ARRAY_SMALL == ta.kind) {
        Sequence<SBound> bounds_a = ta.array_sdefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (static_cast<LBound>(bounds_a[i]) != bounds_b[i]) {
              return false;
          }
        }

        return strongly_assignable(*ta.array_sdefn.element_identifier.in(),
                                   *tb.array_type.element.common.type.in());
      } else { // TI_PLAIN_ARRAY_LARGE
        Sequence<LBound> bounds_a = ta.array_ldefn.array_bound_seq;
        if (bounds_a.size() != bounds_b.size()) {
          return false;
        }

        for (size_t i = 0; i < bounds_a.size(); ++i) {
          if (bounds_a[i] != bounds_b[i]) {
            return false;
          }
        }

        return strongly_assignable(*ta.array_ldefn.element_identifier.in(),
                                   *tb.array_type.element.common.type.in());
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
    if (TI_PLAIN_MAP_SMALL == tb.kind) {
      if (TI_PLAIN_MAP_SMALL == ta.kind) {
        return strongly_assignable(*ta.map_sdefn.key_identifier.in(),
                                   *tb.map_sdefn.key_identifier.in()) &&
          strongly_assignable(*ta.map_sdefn.element_identifier.in(),
                              *tb.map_sdefn.element_identifier.in());
      } else { // TI_PLAIN_MAP_LARGE
        return strongly_assignable(*ta.map_ldefn.key_identifier.in(),
                                   *tb.map_sdefn.key_identifier.in()) &&
          strongly_assignable(*ta.map_ldefn.element_identifier.in(),
                              *tb.map_sdefn.element_identifier.in());
      }
    } else if (TI_PLAIN_MAP_LARGE == tb.kind) {
      if (TI_PLAIN_MAP_SMALL == ta.kind) {
        return strongly_assignable(*ta.map_sdefn.key_identifier.in(),
                                   *tb.map_ldefn.key_identifier.in()) &&
          strongly_assignable(*ta.map_sdefn.element_identifier.in(),
                              *tb.map_ldefn.element_identifier.in());
      } else { // TI_PLAIN_MAP_LARGE
        return strongly_assignable(*ta.map_ldefn.key_identifier.in(),
                                   *tb.map_ldefn.key_identifier.in()) &&
          strongly_assignable(*ta.map_ldefn.element_identifier.in(),
                              *tb.map_ldefn.element_identifier.in());
      }
    } else if (EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& tob = lookup_minimal(tb);
      if (TK_MAP == tob.kind) {
        return assignable_plain_map(ta, tob);
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_plain_map(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
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
      if (TI_PLAIN_MAP_SMALL == ta.kind) {
        return strongly_assignable(*ta.map_sdefn.key_identifier.in(),
                                   *tb.map_type.key.common.type.in()) &&
          strongly_assignable(*ta.map_sdefn.element_identifier.in(),
                              *tb.map_type.element.common.type.in());
      } else { // TI_PLAIN_MAP_LARGE
        return strongly_assignable(*ta.map_ldefn.key_identifier.in(),
                                   *tb.map_type.key.common.type.in()) &&
          strongly_assignable(*ta.map_ldefn.element_identifier.in(),
                              *tb.map_type.element.common.type.in());
      }
    }

    return false;
  }

  bool TypeAssignability::strongly_assignable(const TypeIdentifier& tia,
                                              const TypeIdentifier& tib) const
  {
    return false; // TODO: Implement this helper
  }

  bool TypeAssignability::is_delimited(const TypeIdentifier& ti) const
  {
    switch (ti.kind) {
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
    default:
      return true; // TODO: Add other types
    }
  }

  /**
   * @brief The input type is either structure or union
   */
  void TypeAssignability::erase_key(MinimalTypeObject& type) const
  {
    // TODO: Finish this function
    if (TK_STRUCTURE == type.kind) {

    } else if (TK_UNION == type.kind) {

    }
  }

  /**
   * @brief The input must be of type TK_ALIAS
   *        Return the non-alias base type identifier of the input
   */
  const TypeIdentifier& TypeAssignability::get_base_type(const MinimalTypeObject&
                                                         type) const
  {
    const TypeIdentifier& base = *type.alias_type.body.common.related_type.in();
    switch (base.kind) {
    case EK_COMPLETE:
    case EK_MINIMAL:
      const MinimalTypeObject& type_obj = lookup_minimal(base);
      if (TK_ALIAS == type_obj.kind) {
        return get_base_type(type_obj);
      }
      return base;
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
    if (EK_MINIMAL != ma.member_type_id.kind) {
      return false;
    }

    const MinimalEnumeratedLiteralSeq& literals_b = tb.enumerated_type.literal_seq;
    const MinimalTypeObject& toa = lookup_minimal(*ma.member_type_id.in());
    const MinimalEnumeratedLiteralSeq* literals_a = 0;
    if (TK_ENUM == toa.kind) {
      literals_a = &toa.enumerated_type.literal_seq;
    } else if (TK_ALIAS == toa.kind) {
      const TypeIdentifier& base_a = get_base_type(toa);
      if (EK_MINIMAL == base_a.kind) {
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
      NameHash h_b = literals_b.members[j].detail.name_hash;
      ACE_CDR::ULong
        key_b = (h_b[0] << 24) | (h_b[1] << 16) | (h_b[2] << 8) | (h_b[3]);
      bool found = false;
      for (size_t k = 0; k < literals_a->members.size(); ++k) {
        NameHash h_a = literals_a->members[k].detail.name_hash;
        ACE_CDR::ULong
          key_a = (h_a[0] << 24) | (h_a[1] << 16) | (h_a[2] << 8) | (h_a[3]);
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
   * @brief Return true if the second argument is of type sequence and
   * has bound greater than or equal to the first argument
   */
  bool TypeAssignability::struct_rule_seq_key(LBound bound_b,
                                              const CommonStructMember& ma) const
  {
    ACE_CDR::Octet kind = ma.member_type_id->kind;
    LBound bound_a = 0;
    bool type_matched = false;
    if (EK_MINIMAL == kind) {
      const MinimalTypeObject& toa = lookup_minimal(*ma.member_type_id.in());
      if (TK_SEQUENCE == toa.kind) {
        bound_a = toa.sequence_type.header.common.bound;
        type_matched = true;
      } else if (TK_ALIAS == toa.kind) {
        const TypeIdentifier& base_a = get_base_type(toa);
        if (EK_MINIMAL == base_a.kind) {
          const MinimalTypeObject& base_obj_a = lookup_minimal(base_a);
          if (TK_SEQUENCE == base_obj_a.kind) {
            bound_a = base_obj_a.sequence_type.header.common.bound;
            type_matched = true;
          }
        } else if (TI_PLAIN_SEQUENCE_SMALL == base_a.kind) {
          bound_a = static_cast<LBound>(base_a.seq_sdefn.bound);
          type_matched = true;
        } else if (TI_PLAIN_SEQUENCE_LARGE == base_a.kind) {
          bound_a = base_a.seq.ldefn.bound;
          type_matched = true;
        }
      }
    } else if (TI_PLAIN_SEQUENCE_SMALL == kind) {
      bound_a = static_cast<LBound>(ma.member_type_id->seq_sdefn.bound);
      type_matched = true;
    } else if (TI_PLAIN_SEQUENCE_LARGE == kind) {
      bound_a = ma.member_type_id->seq_ldefn.bound;
      type_matched = true;
    }

    if (type_matched && bound_a >= bound_b) {
      return true;
    }
    return false;
  }

  /**
   * @brief Return true if ma is a map and has bound >= first argument
   */
  bool TypeAssignability::struct_rule_map_key(LBound bound,
                                              const CommonStructMember& ma) const
  {
    // TODO
    return true;
  }

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
