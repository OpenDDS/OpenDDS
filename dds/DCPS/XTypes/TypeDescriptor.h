
#ifndef OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H
#define OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H

#include "External.h"

#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/Serializer.h>

#include <ace/CDR_Base.h>

#include <dds/DCPS/XTypes/DynamicType.h>

#include <algorithm>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

enum ExtensibilityKind {
 FINAL,
 APPENDABLE,
 MUTABLE
};

class TypeDescriptor {
 public:
  TypeKind kind;
  OPENDDS_STRING name;
  DynamicType_rch base_type;
  DynamicType_rch discriminator_type;
  LBoundSeq bound;
  DynamicType_rch element_type;
  DynamicType_rch key_element_type;
  ExtensibilityKind extensibility_kind;
  bool is_nested;
  bool equals(const TypeDescriptor& other);
 };

inline bool operator==(const LBoundSeq& lhs, const LBoundSeq& rhs) {
  if (lhs.length() == rhs.length()) {
    for (ulong i = 0 ; i < lhs.length() ; ++i) {
      if (!(lhs[i] == rhs[i])) {
        return false;
      }
    }
  } else {
    return false;
  }
  return true;
}

inline bool operator==(const TypeDescriptor& lhs, const TypeDescriptor& rhs) {
  return
    lhs.kind == rhs.kind &&
    lhs.name == rhs.name &&
    lhs.base_type == rhs.base_type &&
    lhs.discriminator_type == rhs.discriminator_type &&
    lhs.bound == rhs.bound &&
    lhs.element_type.in() == rhs.element_type.in() &&
    lhs.key_element_type.in() == rhs.key_element_type.in() &&
    lhs.extensibility_kind == rhs.extensibility_kind &&
    lhs.is_nested == rhs.is_nested;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H */