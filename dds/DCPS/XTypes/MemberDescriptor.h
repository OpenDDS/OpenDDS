
#ifndef OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H
#define OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H

#include "External.h"
#include "DynamicType.h"

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
  MemberDescriptor()
  : id(0), index(0), try_construct_kind(DISCARD), is_key(0), is_optional(0), is_must_understand(0), is_shared(0), is_default_label(0)
  {
  }
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

inline bool operator==(const UnionCaseLabelSeq& lhs, const UnionCaseLabelSeq& rhs)
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

OpenDDS_Dcps_Export bool test_equality(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H */
