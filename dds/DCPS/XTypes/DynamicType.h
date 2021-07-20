#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H

#include "External.h"

#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/RcObject.h>
#include <ace/CDR_Base.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/XTypes/DynamicTypeMember.h>
#include <algorithm>
#include <cstring>


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class TypeDescriptor;

class DynamicType : public OpenDDS::DCPS::RcObject {
public:
  DDS::ReturnCode_t get_descriptor(TypeDescriptor& descriptor);
  OPENDDS_STRING get_name();
  TypeKind get_kind();
  DDS::ReturnCode_t get_member_by_name(DynamicTypeMember_rch& member, const OPENDDS_STRING& name);
  DDS::ReturnCode_t get_all_members_by_name(DynamicTypeMembersByName& member);
  DDS::ReturnCode_t get_member(DynamicTypeMember_rch& member, const MemberId& id);
  DDS::ReturnCode_t get_all_members(DynamicTypeMembersById& member);
  unsigned long get_member_count();
  DDS::ReturnCode_t get_member_by_index(DynamicTypeMember_rch& member, unsigned long index);
  bool equals(const DynamicType& other);
  DynamicTypeMembersByName member_by_name;
  DynamicTypeMembersById member_by_id;
  DynamicTypeMembersByIndex member_by_index;
  TypeDescriptor* descriptor_;
};

typedef OpenDDS::DCPS::RcHandle<DynamicType> DynamicType_rch;

inline bool operator==(const DynamicType& lhs, const DynamicType& rhs) {
    return
      lhs.member_by_name == rhs.member_by_name &&
      lhs.member_by_id == rhs.member_by_id;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H */