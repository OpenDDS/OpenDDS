/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeAssignability.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

  // Assuming caller always passes objects of structure or union type
  bool TypeAssignability::assignable(const TypeObject& ta, const TypeObject& tb) const
  {
    switch (ta.kind) {
    case TK_STRUCTURE:
      return assignable_struct(ta, tb);
    case TK_UNION:
      return assignable_union(ta, tb);
    default:
      return false;
    }
  }

  bool TypeAssignability::assignable_struct(const TypeObject& ta, const TypeObject& tb) const
  {
    
  }
  
  bool TypeAssignability::assignable_union(const TypeObject& ta, const TypeObject& tb) const
  {
  }
  
  bool TypeAssignability::assignable_primitive(const TypeIdentifier& ta, const TypeIdentifier& tb) const
  {
    if (ta.kind == tb.kind) {
      return true;
    }

    if (EK_COMPLETE == tb.kind || EK_MINIMAL == tb.kind) {
      const TypeObject& type_obj = lookup(tb);
      if (EK_COMPLETE == type_obj.kind) {
        if (TK_BITMASK != type_obj.complete.kind ||
            !(TK_UINT8 == ta.kind || TK_UINT16 == ta.kind ||
              TK_UINT32 == ta.kind || TK_UINT64 == ta.kind)) {
          return false;
        }

        BitBound bit_bound = type_obj.complete.bitmask_type.header.common.bit_bound;
        if (TK_UINT8 == ta.kind) {
          return 1 <= bit_bound && bit_bound <= 8;
        } else if (TK_UINT16 == ta.kind) {
          return 9 <= bit_bound && bit_bound <= 16;
        } else if (TK_UINT32 == ta.kind) {
          return 17 <= bit_bound && bit_bound <= 32;
        } else if (TK_UINT64 == ta.kind) {
          return 33 <= bit_bound && bit_bound <= 64;
        }

      } else { // EK_MINIMAL == type_obj.kind
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
    }

    return false;
  }

  bool TypeAssignability::assignable_string(const TypeIdentifier& ta, const TypeIdentifier& tb) const
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

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
