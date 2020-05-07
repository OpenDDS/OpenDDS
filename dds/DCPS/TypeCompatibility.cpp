/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TypeCompatibility.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

  // Whether ta is-assignable-from tb.
  bool compatible(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
    switch (ta.kind) {
      // Primitive types
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
    case TK_UINT8
    case TK_CHAR8:
    case TK_CHAR16:
      return compatible_primitive(ta, tb);

      // String types
    case TK_STRING8:
    case TK_STRING16:
      return compatible_string(ta, tb);

      // Alias
    case TK_ALIAS:
      return compatible_alias(ta, tb);

      // Enumerated types
    case TK_ENUM:
    case TK_BITMASK:
      return compatible_enumerated(ta, tb);

      // Aggregated types
    case TK_STRUCTURE:
    case TK_UNION:
      return compatible_aggregated(ta, tb);

      // Collection types
    case TK_SEQUENCE:
    case TK_ARRAY:
    case TK_MAP:
      return compatible_collection(ta, tb);

      // Invalid types
    default:
      return false;
    }
  }

  bool compatible_primitive(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

  bool compatible_string(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

  bool compatible_alias(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

  bool compatible_enumerated(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

  bool compatible_aggregated(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

  bool compatible_collection(const TypeIdentifier& ta, const TypeIdentifier& tb)
  {
  }

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
