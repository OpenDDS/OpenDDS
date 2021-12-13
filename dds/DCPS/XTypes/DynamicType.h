#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H

#include "TypeDescriptor.h"
#include "MemberDescriptor.h"

#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/RcObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace DDS {
  typedef ACE_CDR::Long ReturnCode_t;
}

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicType : public DCPS::RcObject
{
public:
  DynamicType();
  ~DynamicType();

  void set_descriptor(const TypeDescriptor& descriptor);
  void get_descriptor(TypeDescriptor& descriptor) const;
  TypeDescriptor get_descriptor() const;
  DCPS::String get_name() const;
  TypeKind get_kind() const;
  DDS::ReturnCode_t get_member_by_name(DynamicTypeMember_rch& member, const DCPS::String& name) const;
  void get_all_members_by_name(DynamicTypeMembersByName& member) const;
  DDS::ReturnCode_t get_member(DynamicTypeMember_rch& member, MemberId id) const;
  void get_all_members(DynamicTypeMembersById& member) const;
  ACE_CDR::ULong get_member_count() const;
  DDS::ReturnCode_t get_member_by_index(DynamicTypeMember_rch& member, ACE_CDR::ULong index) const;
  bool equals(const DynamicType& other) const;
  bool test_equality_i(const DynamicType& rhs, DynamicTypePtrPairSeen& dt_ptr_pair) const;
  void insert_dynamic_member(const DynamicTypeMember_rch& dtm);
  DynamicType_rch get_base_type();
private:
  DynamicTypeMembersByName member_by_name;
  DynamicTypeMembersById member_by_id;
  DynamicTypeMembersByIndex member_by_index;
  TypeDescriptor descriptor_;
};


OpenDDS_Dcps_Export bool operator==(const DynamicType& lhs, const DynamicType& rhs);
bool test_equality_i(const DynamicType_rch& lhs, const DynamicType_rch& rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_H */
