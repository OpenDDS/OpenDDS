#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/MemberDescriptor.h>
#include <dds/DCPS/XTypes/TypeDescriptor.h>

namespace OpenDDS {
namespace XTypes {

  DDS::ReturnCode_t DynamicType::get_descriptor(TypeDescriptor& descriptor) {
    descriptor = *descriptor_;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const OPENDDS_STRING& name) {
    //We have a map of members in member_by_name which takes a string and gives 0 or 1 type DTMs
    //this should use the string as the key and put the returned DTM into member.
    member = member_by_name.find(name)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member) {
    member = member_by_name;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, const MemberId& id) {
    member = member_by_id.find(id)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members(DynamicTypeMembersById& member) {
    member = member_by_id;
    return DDS::RETCODE_OK;
  }
  unsigned long DynamicType::get_member_count() {
    return member_by_name.size();
  }
  DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, unsigned long index) {
    member = member_by_index[index];
  }
  bool DynamicType::equals(const DynamicType& other) {
    return (*this == other);
  }

} // namespace XTypes
} // namespace OpenDDS
