
#ifndef OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H
#define OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H

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

enum TryConstructKind {
 USE_DEFAULT,
 DISCARD,
 TRIM
};

class MemberDescriptor {
public:
  OPENDDS_STRING name;
  XTypes::MemberId id;
  DynamicType_rch type;
  OPENDDS_STRING default_value;
  unsigned long index;
  XTypes::UnionCaseLabelSeq label;
  TryConstructKind try_construct_kind;
  bool is_key;
  bool is_optional;
  bool is_must_understand;
  bool is_shared;
  bool is_default_label;
  bool equals(const MemberDescriptor& descriptor);
 };

inline bool operator==(const UnionCaseLabelSeq& lhs, const UnionCaseLabelSeq& rhs) {
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

inline bool operator==(const MemberDescriptor& lhs, const MemberDescriptor& rhs) {
  return
    lhs.name == rhs.name &&
    lhs.id == rhs.id &&
    lhs.type == rhs.type &&
    lhs.default_value == rhs.default_value &&
    lhs.index == rhs.index &&
    lhs.label == rhs.label &&
    lhs.try_construct_kind == rhs.try_construct_kind &&
    lhs.is_key == rhs.is_key &&
    lhs.is_optional == rhs.is_optional &&
    lhs.is_must_understand == rhs.is_must_understand &&
    lhs.is_shared == rhs.is_shared &&
    lhs.is_default_label == rhs.is_default_label;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H */