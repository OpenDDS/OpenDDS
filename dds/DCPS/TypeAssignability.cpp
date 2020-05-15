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
        return assignable_structure(ta, tib);
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

  bool TypeAssignability::assignable_annotation(const MinimalTypeObject& ta,
                                                const MinimalTypeObject& tb) const
  {
    // No rule for annotation in the spec
    return false;
  }

  bool TypeAssignability::assignable_annotation(const MinimalTypeObject& ta,
                                                const TypeIdentifier& tb) const
  {
    // No rule for annotation in the spec
    return false;
  }

  bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

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

    // Discriminator type must be strongly assignable
    const TypeIdentifier& tia = *ta.union_type.discriminator.common.type_id.in();
    const TypeIdentifier& tib = *tb.union_type.discriminator.common.type_id.in();
    ACE_CDR::Octet a_disc = tia.kind;
    ACE_CDR::Octet b_disc = tib.kind;
    if (TK_BOOLEAN == a_disc || TK_BYTE == a_disc || TK_INT16 == a_disc ||
        TK_INT32 == a_disc || TK_INT64 == a_disc || TK_UINT16 == a_disc ||
        TK_UINT32 == a_disc || TK_UINT64 == a_disc || TK_INT8 == a_disc ||
        TK_UINT8 == a_disc || TK_CHAR8 == a_disc || TK_CHAR16 == a_disc) {
      if (!(a_disc == b_disc ||
            (assignable_primitive(tia, tib) && is_delimited(tib)))) {
        return false;
      }
    } else if (EK_COMPLETE == a_disc || EK_MINIMAL == a_disc) {
      if ((b_disc == a_disc && tia.equivalence_hash != tib.equivalence_hash) ||
          b_disc != a_disc) {
        // TODO: Check if tia is-assignable-from tib and tib is delimited
      }
    } else { // Invalid types for discriminator
      return false;
    }

    return true;
  }

  bool TypeAssignability::assignable_union(const MinimalTypeObject& ta,
                                           const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_bitset(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_bitset(const MinimalTypeObject& ta,
                                            const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                              const MinimalTypeObject& tb) const
  {
    if (TK_SEQUENCE != tb.kind) {
      return false;
    }

    // Element types must be strongly assignable
    if (!strongly_assignable(*ta.sequence_type.element.common.type.in(),
                             *tb.sequence_type.element.common.type.in())) {
      return false;
    }

    return true;
  }

  bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                              const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

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

    // Element types must be strongly assignable
    if (!strongly_assignable(*ta.array_type.element.common.type.in(),
                             *tb.array_type.element.common.type.in())) {
      return false;
    }

    return true;
  }

  bool TypeAssignability::assignable_array(const MinimalTypeObject& ta,
                                           const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                         const MinimalTypeObject& tb) const
  {
    if (TK_MAP != tb.kind) {
      return false;
    }

    // Key element types must be strongly assignable
    // Value element types must also be strongly assignable
    if (!strongly_assignable(*ta.map_type.key.common.type.in(),
                             *tb.map_type.key.common.type.in()) ||
        !strongly_assignable(*ta.map_type.element.common.type.in(),
                             *tb.map_type.element.common.type.in())) {
      return false;
    }

    return true;
  }

  bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                         const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

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
        const ACE_CDR::Octet*
          h = ta.enumerated_type.literal_seq.members[i].detail.name_hash;
        ACE_CDR::ULong key_a = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | (h[3]);
        ta_maps[key_a] = ta.enumerated_type.literal_seq.members[i].common.value;
      }

      for (size_t i = 0; i < size_b; ++i) {
        const ACE_CDR::Octet*
          h = tb.enumerated_type.literal_seq.members[i].detail.name_hash;
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

  bool TypeAssignability::assignable_enum(const MinimalTypeObject& ta,
                                          const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_bitmask(const MinimalTypeObject& ta,
                                             const MinimalTypeObject& tb) const
  {
    BitBound ta_bit_bound = ta.bitmask_type.header.common.bit_bound;
    if (TK_BITMASK == tb.kind) {
      return ta_bit_bound == tb.bitmask.type.header.common.bit_bound;
    } else if (TK_UINT8 == tb.kind) {
      return 1 <= ta_bit_bound && ta_bit_bound <= 8;
    } else if (TK_UINT16 == tb.kind) {
      return 9 <= ta_bit_bound && ta_bit_bound <= 16;
    } else if (TK_UINT32 == tb.kind) {
      return 17 <= ta_bit_bound && ta_bit_bound <= 32;
    } else if (TK_UINT64 == tb.kind) {
      return 33 <= ta_bit_bound && ta_bit_bound <= 64;
    }

    return false;
  }

  bool TypeAssignability::assignable_bitmask(const MinimalTypeObject& ta,
                                             const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_extended(const MinimalTypeObject& ta,
                                              const MinimalTypeObject& tb) const
  {
    // Future extensions
    return false;
  }


  bool TypeAssignability::assignable_primitive(const TypeIdentifier& ta,
                                               const TypeIdentifier& tb) const
  {
    if (ta.kind == tb.kind) {
      return true;
    }

    if (EK_COMPLETE == tb.kind || EK_MINIMAL == tb.kind) {
      const MinimalTypeObject& type_obj = lookup_minimal(tb);
      return assignable_primitive(ta, type_obj);
    }

    return false;
  }

  /**
   * @brief The second type must not be TK_ALIAS
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
    } else { // TK_UINT64 == ta.kind
      return 33 <= bit_bound && bit_bound <= 64;
    }
  }

  bool TypeAssignability::assignable_string(const TypeIdentifier& ta,
                                            const TypeIdentifier& tb) const
  {
    if (TI_STRING8_SMALL == ta.kind || TI_STRING8_LARGE == ta.kind) {
      if (TI_STRING8_SMALL == tb.kind || TI_STRING8_LARGE == tb.kind) {
        return true;
      }
      return false;

    } else { // TI_STRING16_SMALL == ta.kind || TI_STRING16_LARGE == ta.kind
      if (TI_STRING16_SMALL == tb.kind || TI_STRING16_LARGE == tb.kind) {
        return true;
      }
      return false;
    }
  }

  /**
   * @brief The second type must not be TK_ALIAS
   */
  bool TypeAssignability::assignable_string(const TypeIdentifier& ta,
                                            const MinimalTypeObject& tb) const
  {
    // The second type cannot be string since string type does not have
    // type object. Thus the first type is not assignable from the second type.
    return false;
  }

  /**
   * @brief The first type is a plain sequence. The second type can be anything.
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
        if (TI_PLAIN_SEQUENCE_SMALL == ta.kind) {
          return strongly_assignable(*ta.seq_sdefn.element_identifier.in(),
                                     *tob.sequence_type.element.common.type.in());
        } else { // TI_PLAIN_SEQUENCE_LARGE
          return strongly_assignable(*ta.seq_ldefn.element_identifier.in(),
                                     *tob.sequence_type.element.common.type.in());
        }
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
   * @brief The first type must be a plain sequence.
   * The second type must not be TK_ALIAS.
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
                                   *tb.sequence_type.element.common.type.i());
      }
    }

    return false;
  }

  /**
   * @brief The first type must be a plain array. The second type can be anything.
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
        Sequence<LBound> bounds_b = tob.array_type.header.common.bound_seq;
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
                                     *tob.array_type.element.common.type.in());
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
                                     *tob.array_type.element.common.type.in());
        }
      } else if (TK_ALIAS == tob.kind) {
        const TypeIdentifier& base = *tob.alias_type.body.common.related_type.in();
        return assignable_plain_array(ta, base);
      }
    } else if (EK_COMPLETE == tb.kind) {
      // Assuming tb.kind of EK_COMPLETE is not supported (similar to
      // the way assignability for plain sequences are being handled)
      return false;
    }

    return false;
  }

  /**
   * @brief The second type must not be TK_ALIAS
   */
  bool TypeAssignability::assignable_plain_array(const TypeIdentifier& ta,
                                                 const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_plain_map(const TypeIdentifier& ta,
                                               const TypeIdentifier& tb) const
  {
    return false; // TODO: Implement this
  }

  /**
   * @brief The second type must not be TK_ALIAS
   */
  bool TypeAssignability::assignable_plain_map(const TypeIdentifier& ta,
                                               const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
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
