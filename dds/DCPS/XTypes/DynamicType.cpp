#include "DynamicType.h"

#include "MemberDescriptor.h"
#include "TypeDescriptor.h"

namespace OpenDDS {
namespace XTypes {

  DDS::ReturnCode_t DynamicType::get_descriptor(TypeDescriptor& descriptor)
  {
    descriptor = *descriptor_;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const OPENDDS_STRING& name)
  {
    //We have a map of members in member_by_name which takes a string and gives 0 or 1 type DTMs
    //this should use the string as the key and put the returned DTM into member.
    member = member_by_name.find(name)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member)
  {
    member = member_by_name;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, const MemberId& id)
  {
    member = member_by_id.find(id)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members(DynamicTypeMembersById& member)
  {
    member = member_by_id;
    return DDS::RETCODE_OK;
  }
  unsigned long DynamicType::get_member_count()
  {
    return member_by_name.size();
  }
  DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, unsigned long index)
  {
    member = member_by_index[index];
  }

  bool DynamicType::equals(const DynamicType& other)
  {
    return true;
    //TODO CLAYTON
    //return (*this == other);
  }
  bool test_equality_i(const DynamicType& lhs, const DynamicType& rhs)
  {
    DynamicTypePtrPairSeen dt_ptr_pair;
    return test_equality(lhs, rhs, dt_ptr_pair);
  }
  bool test_equality(const DynamicType& lhs, const DynamicType& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    //check pair seen
    DynamicTypePtrPair this_pair = std::make_pair(&lhs, &rhs);
    DynamicTypePtrPairSeen::iterator have_seen = dt_ptr_pair.find(this_pair);
    if (have_seen == dt_ptr_pair.end()) {
      dt_ptr_pair.insert(this_pair);
      return
        test_equality(*lhs.descriptor_, *rhs.descriptor_, dt_ptr_pair) &&
        test_equality(lhs.member_by_name, rhs.member_by_name, dt_ptr_pair) &&
        test_equality(lhs.member_by_id, rhs.member_by_id, dt_ptr_pair);
    }
    return true;
  }

  bool test_equality(const DynamicType_rch& lhs, const DynamicType_rch& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    if (lhs.in() == rhs.in()) {
      return true;
    } else if (lhs.in() == 0 || rhs.in() == 0) {
      return false;
    } else {
      return test_equality(*lhs.in(), *rhs.in(), dt_ptr_pair);
    }
  }

} // namespace XTypes
} // namespace OpenDDS
