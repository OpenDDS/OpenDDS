
#ifndef OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H
#define OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H

#include "External.h"
#include "DynamicType.h"

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
  TypeDescriptor()
  : kind(0), extensibility_kind(FINAL), is_nested(0)
  {
  }
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

inline bool operator==(const LBoundSeq& lhs, const LBoundSeq& rhs)
{
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

OpenDDS_Dcps_Export bool test_equality(const TypeDescriptor& lhs, const TypeDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H */