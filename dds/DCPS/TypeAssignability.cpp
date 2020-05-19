/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeAssignability.h"

#include <map>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

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
        return assignable_primitive(tia, tib);
      case TI_STRING8_SMALL:
      case TI_STRING8_LARGE:
      case TI_STRING16_SMALL:
      case TI_STRING16_LARGE:
        return assignable_string(tia, tib);
      case TI_PLAIN_SEQUENCE_SMALL:
      case TI_PLAIN_SEQUENCE_LARGE:
        return assignable_plain_sequence(tia, tib);
      case TI_PLAIN_ARRAY_SMALL:
      case TI_PLAIN_ARRAY_LARGE:
        return assignable_plain_array(tia, tib);
      case TI_PLAIN_MAP_SMALL:
      case TI_PLAIN_MAP_LARGE:
        return assignable_plain_map(tia, tib);
      case TI_STRONGLY_CONNECTED_COMPONENT:
        return false;
      case EK_COMPLETE:
        return false;
      case EK_MINIMAL:
        const MinimalTypeObject& base_type_a = lookup_minimal(tia);
        const MinimalTypeObject& base_type_b = lookup_minimal(tib);
        TypeObject wrapper_a(base_type_a), wrapper_b(base_type_b);
        return assignable(wrapper_a, wrapper_b);
      default:
        return false; // Future extensions
      }
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
    return false; // TODO: Implement this
  }

  /**
   * @brief The first type must be TK_STRUCTURE.
   *        The second type can be anything.
   */
  bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
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
    map<MemberId, NameHash> id_to_name_a;
    map<ACE_CDR::ULong, MemberId> name_to_id_a;
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

    // All non-default labels in T2 that select some member in T1,
    // the type of the selected member in T1 is assignable from the
    // type of the T2 member
    for (size_t i = 0; i < tb.union_type.member_seq.members.size(); ++i) {
      UnionMemberFlag
        flags_b = tb.union_type.member_seq.members[i].common.member_flags;
      if (flags_b & IS_DEFAULT == IS_DEFAULT) {
        continue;
      }
      UnionCaseLabelSeq
        label_seq_b = tb.union_type.member_seq.members[i].common.label_seq;
      
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
    return false; // TODO: Implement this
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
    } else if (EK_MINIMAL == tb.kind) {
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

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
