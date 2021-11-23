#ifndef OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H
#define OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_H

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/RcHandle_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class DynamicType;
class DynamicTypeMember;
typedef DCPS::RcHandle<DynamicTypeMember> DynamicTypeMember_rch;
typedef OPENDDS_MAP(DCPS::String, DynamicTypeMember_rch) DynamicTypeMembersByName;
typedef OPENDDS_MAP(MemberId, DynamicTypeMember_rch) DynamicTypeMembersById;
typedef OPENDDS_VECTOR(DynamicTypeMember_rch) DynamicTypeMembersByIndex;
typedef std::pair<const DynamicType*, const DynamicType*> DynamicTypePtrPair;
typedef OPENDDS_SET(DynamicTypePtrPair) DynamicTypePtrPairSeen;
typedef DCPS::RcHandle<DynamicType> DynamicType_rch;

enum TryConstructKind {
  USE_DEFAULT,
  DISCARD,
  TRIM
};

class OpenDDS_Dcps_Export MemberDescriptor {
public:
  MemberDescriptor();
  ~MemberDescriptor();

  DynamicType_rch get_type() const;
  bool equals(const MemberDescriptor& other) const;

  DCPS::String name;
  MemberId id;
  DCPS::WeakRcHandle<DynamicType> type;
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
    for (ACE_CDR::ULong i = 0; i < lhs.length(); ++i) {
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
