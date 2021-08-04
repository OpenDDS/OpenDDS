#include "DCPS/DdsDcps_pch.h"
#include "DynamicTypeMember.h"

#include "MemberDescriptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicTypeMember::DynamicTypeMember()
  : descriptor_(new MemberDescriptor())
{}

DynamicTypeMember::~DynamicTypeMember()
{
  delete descriptor_;
}

DDS::ReturnCode_t DynamicTypeMember::get_descriptor(MemberDescriptor& descriptor) const
{
  descriptor = *descriptor_;
  return DDS::RETCODE_OK;
}

bool DynamicTypeMember::equals(const DynamicTypeMember& other)
{
//7.5.2.6.3 Operation: equals
//Two members shall be considered equal if and only if they belong to the same type and all of
//their respective properties, as identified in Table 52 above, are equal.

//This spec implementation is not currently possible so I have done what I consider to be a deep compare
//8/3/2021 Clayton Calabrese
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(*this, other, dt_ptr_pair);
}

MemberId DynamicTypeMember::get_id()
{
  return descriptor_->id;
}

DCPS::String DynamicTypeMember::get_name()
{
  return descriptor_->name;
}

bool test_equality(const DynamicTypeMember& lhs, const DynamicTypeMember& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return test_equality(*lhs.descriptor_, *rhs.descriptor_, dt_ptr_pair);
}

bool test_equality(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if(rhs.find(it->first) != rhs.end()) {
        if (!test_equality(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair)) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Members are not the same\n")));
          return false;
        }
      } else {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Member from lhs not found in rhs\n")));
        return false;
      }
    }
    return true;
  } else {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Member count is different\n")));
    return false;
  }
}

bool test_equality(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersById::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if(rhs.find(it->first) != rhs.end()) {
        if (!test_equality(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair)) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Members are not the same\n")));
          return false;
        }
      } else {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Member from lhs not found in rhs\n")));
        return false;
      }
    }
    return true;
  } else {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Member count is different\n")));
    return false;
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
