
#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H

#include "DynamicType.h"

#include <dds/DCPS/RcObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicTypeMember : public DCPS::RcObject {
public:
  DynamicTypeMember();
  ~DynamicTypeMember();
  void set_descriptor(const MemberDescriptor& descriptor);
  void get_descriptor(MemberDescriptor& descriptor) const;
  MemberDescriptor get_descriptor() const;
  void set_parent(const DynamicType_rch& dt);
  DynamicType_rch get_parent();
  bool equals(const DynamicTypeMember& other) const;
  MemberId get_id() const;
  DCPS::String get_name() const;

private:
  DCPS::WeakRcHandle<DynamicType> parent_;
  MemberDescriptor descriptor_;
};

bool test_equality_i(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
bool test_equality_i(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_H */
