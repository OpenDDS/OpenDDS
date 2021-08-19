
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

class TypeDescriptor : public DCPS::RcObject {
public:
  TypeDescriptor()
    : kind(TK_NONE), extensibility_kind(FINAL), is_nested(false)
  {}

  bool equals(const TypeDescriptor& other) const;

  TypeKind kind;
  DCPS::String name;
  DynamicType_rch base_type;
  DynamicType_rch discriminator_type;
  LBoundSeq bound;
  DynamicType_rch element_type;
  DynamicType_rch key_element_type;
  ExtensibilityKind extensibility_kind;
  bool is_nested;
};

inline bool operator==(const LBoundSeq& lhs, const LBoundSeq& rhs)
{
  if (lhs.length() == rhs.length()) {
    for (ACE_CDR::ULong i = 0 ; i < lhs.length() ; ++i) {
      if (lhs[i] != rhs[i]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool test_equality_i(const TypeDescriptor& lhs, const TypeDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_H */
