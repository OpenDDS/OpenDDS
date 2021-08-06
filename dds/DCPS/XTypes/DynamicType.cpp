#include "DCPS/DdsDcps_pch.h"
#include "DynamicType.h"
#include "MemberDescriptor.h"
#include "TypeDescriptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicType::DynamicType()
  : descriptor_(new TypeDescriptor())
{}

DynamicType::~DynamicType()
{
  delete descriptor_;
}

void DynamicType::get_descriptor(TypeDescriptor& descriptor) const
{
  descriptor = *descriptor_;
}

DCPS::String DynamicType::get_name() const
{
  return this->descriptor_->name;
}

TypeKind DynamicType::get_kind() const
{
  return this->descriptor_->kind;
}

DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const DCPS::String& name)
{
  //We have a map of members in member_by_name which takes a string and gives 0 or 1 type DTMs
  //this should use the string as the key and put the returned DTM into member.
  DynamicTypeMembersByName::const_iterator pos = member_by_name.find(name);
  if (pos != member_by_name.end()) {
    member = pos->second;
    return DDS::RETCODE_OK;
  } else {
    return DDS::RETCODE_ERROR;
  }
}

void DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member)
{
  member = member_by_name;
}

DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, MemberId id)
{
  DynamicTypeMembersById::const_iterator pos = member_by_id.find(id);
  if (pos != member_by_id.end()) {
    member = pos->second;
    return DDS::RETCODE_OK;
  } else {
    return DDS::RETCODE_ERROR;
  }
}

void DynamicType::get_all_members(DynamicTypeMembersById& member)
{
  member = member_by_id;
}

ACE_CDR::ULong DynamicType::get_member_count()
{
  return member_by_name.size();
}

DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, ACE_CDR::ULong index)
{
  if (index < member_by_index.size()) {
    member = member_by_index[index];
    return DDS::RETCODE_OK;
  } else {
    return DDS::RETCODE_ERROR;
  }
}

bool DynamicType::equals(const DynamicType& other)
{
  return test_equality(*this, other);
}

bool test_equality(const DynamicType& lhs, const DynamicType& rhs)
{
//7.5.2.8.4 Operation: equals
//Two types shall be considered equal if and only if all of their respective properties, as identified
//in Table 54 above, are equal.

//Note: We are comparing the TypeDescriptor even though the spec seems to say not to
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality_i(lhs, rhs, dt_ptr_pair);
}

bool test_equality_i(const DynamicType& lhs, const DynamicType& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  //check pair seen
  DynamicTypePtrPair this_pair = std::make_pair(&lhs, &rhs);
  DynamicTypePtrPairSeen::const_iterator have_seen = dt_ptr_pair.find(this_pair);
  if (have_seen == dt_ptr_pair.end()) {
    dt_ptr_pair.insert(this_pair);
    return
      test_equality_i(*lhs.descriptor_, *rhs.descriptor_, dt_ptr_pair) &&
      test_equality_i(lhs.member_by_name, rhs.member_by_name, dt_ptr_pair) &&
      test_equality_i(lhs.member_by_id, rhs.member_by_id, dt_ptr_pair);
  }
  return true;
}

bool test_equality_i(const DynamicType_rch& lhs, const DynamicType_rch& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.in() == rhs.in()) {
    return true;
  } else if (lhs.in() == 0 || rhs.in() == 0) {
    return false;
  } else {
    return test_equality_i(*lhs.in(), *rhs.in(), dt_ptr_pair);
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
