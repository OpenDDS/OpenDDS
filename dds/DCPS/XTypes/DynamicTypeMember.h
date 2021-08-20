
#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H

#include "External.h"
#include "TypeObject.h"

#include <dds/DdsDcpsInfrastructureC.h>

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/RcHandle_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class MemberDescriptor;
class DynamicType;
typedef DCPS::RcHandle<MemberDescriptor> MemberDescriptor_rch;
typedef DCPS::RcHandle<DynamicType> DynamicType_rch;

class OpenDDS_Dcps_Export DynamicTypeMember : public DCPS::RcObject {
public:
  DynamicTypeMember();

  void get_descriptor(MemberDescriptor_rch& descriptor) const;
  MemberDescriptor_rch& get_descriptor();
  DynamicType_rch get_parent();
  bool equals(const DynamicTypeMember& other) const;
  MemberId get_id() const;
  DCPS::String get_name() const;

private:
  DynamicType_rch parent_;
  MemberDescriptor_rch descriptor_;
};

typedef std::pair<const DynamicType*, const DynamicType*> DynamicTypePtrPair;
typedef OPENDDS_SET(DynamicTypePtrPair) DynamicTypePtrPairSeen;
typedef DCPS::RcHandle<DynamicTypeMember> DynamicTypeMember_rch;
typedef OPENDDS_MAP(DCPS::String, DynamicTypeMember_rch) DynamicTypeMembersByName;
typedef OPENDDS_MAP(MemberId, DynamicTypeMember_rch) DynamicTypeMembersById;
typedef OPENDDS_VECTOR(DynamicTypeMember_rch) DynamicTypeMembersByIndex;

bool test_equality_i(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
bool test_equality_i(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H */
