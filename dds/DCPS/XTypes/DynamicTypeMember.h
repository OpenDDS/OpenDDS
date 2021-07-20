
#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H

#include "External.h"

#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <ace/CDR_Base.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <algorithm>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class MemberDescriptor;

class DynamicTypeMember : public OpenDDS::DCPS::RcObject {
public:
  DynamicTypeMember();
  ~DynamicTypeMember();
   DDS::ReturnCode_t get_descriptor(MemberDescriptor& descriptor);
  //Spec: Two members shall be considered equal if and only if they belong to the same type and all of
  //their respective properties, as identified in Table 52 above, are equal.
  //TODO CLAYTON: The only property this type has is annotation which our plan says to ignore?
  bool equals(const DynamicTypeMember& other);
  MemberId get_id();
  OPENDDS_STRING get_name();
  MemberDescriptor* descriptor_;
};

typedef OpenDDS::DCPS::RcHandle<DynamicTypeMember> DynamicTypeMember_rch;

typedef OPENDDS_MAP(OPENDDS_STRING, DynamicTypeMember_rch) DynamicTypeMembersByName;
typedef OPENDDS_MAP(MemberId, DynamicTypeMember_rch) DynamicTypeMembersById;
typedef OPENDDS_VECTOR(DynamicTypeMember_rch) DynamicTypeMembersByIndex;

inline bool operator==(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName rhs) {
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if (!(lhs.find(it->first)->second == rhs.find(it->first)->second)) {
        return false;
      }
    }
  } else {
    return false;
  }
}

inline bool operator==(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById rhs) {
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersById::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if (!(lhs.find(it->first)->second == rhs.find(it->first)->second)) {
        return false;
      }
    }
  } else {
    return false;
  }
}
} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H */
