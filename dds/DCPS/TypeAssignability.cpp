/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeAssignability.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

  bool TypeAssignability::assignable(const TypeObject& ta,
                                     const TypeObject& tb) const
  {
    if (EK_MINIMAL == ta.kind && EK_MINIMAL == tb.kind) {
      switch (ta.minimal.kind) {
      case TK_ALIAS:
        return assignable_alias(ta.minimal, tb.minimal);
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
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_annotation(const MinimalTypeObject& ta,
                                                const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_struct(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
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
    if (ta.union_type.union_flags && extensibility_mask !=
        tb.union_type.union_flags && extensibility_mask) {
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

  bool TypeAssignability::assignable_bitset(const MinimalTypeObject& ta,
                                            const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_sequence(const MinimalTypeObject& ta,
                                              const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_array(const MinimalTypeObject& ta,
                                           const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_map(const MinimalTypeObject& ta,
                                         const MinimalTypeObject& tb) const
  {
    return false; // TODO: Implement this
  }

  bool TypeAssignability::assignable_enum(const MinimalTypeObject& ta,
                                          const MinimalTypeObject& tb) const
  {
    return true; // TODO: Implement this
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

  bool TypeAssignability::assignable_extended(const MinimalTypeObject& ta,
                                              const MinimalTypeObject& tb) const
  {
    return false; // Reserved for future extensibility
  }


  bool TypeAssignability::assignable_primitive(const TypeIdentifier& ta,
                                               const TypeIdentifier& tb) const
  {
    if (ta.kind == tb.kind) {
      return true;
    }

    if (EK_COMPLETE == tb.kind || EK_MINIMAL == tb.kind) {
      const TypeObject& type_obj = lookup_minimal(tb);
      if (TK_BITMASK != type_obj.minimal.kind ||
          !(TK_UINT8 == ta.kind || TK_UINT16 == ta.kind ||
            TK_UINT32 == ta.kind || TK_UINT64 == ta.kind)) {
        return false;
      }

      BitBound bit_bound = type_obj.minimal.bitmask_type.header.common.bit_bound;
      if (TK_UINT8 == ta.kind) {
        return 1 <= bit_bound && bit_bound <= 8;
      } else if (TK_UINT16 == ta.kind) {
        return 9 <= bit_bound && bit_bound <= 16;
      } else if (TK_UINT32 == ta.kind) {
        return 17 <= bit_bound && bit_bound <= 32;
      } else if (TK_UINT64 == ta.kind) {
        return 33 <= bit_bound && bit_bound <= 64;
      }
    }

    return false;
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
