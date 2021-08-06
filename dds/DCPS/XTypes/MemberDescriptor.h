
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
    : id(MEMBER_ID_INVALID), index(0), try_construct_kind(DISCARD), is_key(false), is_optional(false), is_must_understand(false), is_shared(false), is_default_label(false)
  {}

  bool equals(const MemberDescriptor& other);

  DCPS::String name;
  MemberId id;
  DynamicType_rch type;
  DCPS::String default_value;
  ACE_CDR::ULong index;
  UnionCaseLabelSeq label;
  TryConstructKind try_construct_kind;
  bool is_key;
  bool is_optional;
  bool is_must_understand;
  bool is_shared;
  bool is_default_label;
 };

inline bool operator==(const UnionCaseLabelSeq& lhs, const UnionCaseLabelSeq& rhs)
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

bool test_equality_i(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H */
