#include <DCPS/DdsDcps_pch.h>

#include "DynamicType.h"

#include "DynamicTypeMember.h"

#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicType::DynamicType()
{}

DynamicType::~DynamicType()
{}

void DynamicType::set_descriptor(const TypeDescriptor& descriptor)
{
  descriptor_ = descriptor;
}

void DynamicType::get_descriptor(TypeDescriptor& descriptor) const
{
  descriptor = descriptor_;
}

TypeDescriptor DynamicType::get_descriptor() const
{
  return descriptor_;
}

DCPS::String DynamicType::get_name() const
{
  return descriptor_.name;
}

TypeKind DynamicType::get_kind() const
{
  return descriptor_.kind;
}

DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const DCPS::String& name) const
{
  const DynamicTypeMembersByName::const_iterator pos = member_by_name.find(name);
  if (pos != member_by_name.end()) {
    member = pos->second;
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
}

void DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member) const
{
  member = member_by_name;
}

DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, MemberId id) const
{
  DynamicTypeMembersById::const_iterator pos = member_by_id.find(id);
  if (pos != member_by_id.end()) {
    member = pos->second;
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
}

void DynamicType::get_all_members(DynamicTypeMembersById& member) const
{
  member = member_by_id;
}

ACE_CDR::ULong DynamicType::get_member_count() const
{
  return ACE_CDR::ULong(member_by_index.size());
}

DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, ACE_CDR::ULong index) const
{
  if (index < member_by_index.size()) {
    member = member_by_index[index];
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
}

bool DynamicType::equals(const DynamicType& other) const
{
  //7.5.2.8.4 Operation: equals
  //Two types shall be considered equal if and only if all of their respective properties, as identified
  //in Table 54 above, are equal.

  //In addition to what the spec says to compare, we will be comparing the TypeDescriptors of both
  //DynamicTypes. The spec seems to assume this is the case, despite not listing the TypeDescriptor
  //as a property in table 54. This is done to allow the recursive comparison required by the changes
  //to DynamicTypeMember::equals.
  return *this == other;
}

void DynamicType::insert_dynamic_member(const DynamicTypeMember_rch& dtm)
{
  member_by_index.push_back(dtm);
  if (dtm->get_descriptor().id != MEMBER_ID_INVALID) {
    member_by_id.insert(std::make_pair(dtm->get_descriptor().id , dtm));
  }
  member_by_name.insert(std::make_pair(dtm->get_descriptor().name , dtm));
}

bool DynamicType::test_equality_i(const DynamicType& rhs, DynamicTypePtrPairSeen& dt_ptr_pair) const
{
  //check pair seen
  DynamicTypePtrPair this_pair = std::make_pair(this, &rhs);
  DynamicTypePtrPairSeen::const_iterator have_seen = dt_ptr_pair.find(this_pair);
  if (have_seen == dt_ptr_pair.end()) {
    dt_ptr_pair.insert(this_pair);
    return
      OpenDDS::XTypes::test_equality_i(descriptor_, rhs.descriptor_, dt_ptr_pair) &&
      OpenDDS::XTypes::test_equality_i(member_by_name, rhs.member_by_name, dt_ptr_pair) &&
      OpenDDS::XTypes::test_equality_i(member_by_id, rhs.member_by_id, dt_ptr_pair);
  }
  return true;
}

DynamicType_rch DynamicType::get_base_type()
{
  if (get_kind() != TK_ALIAS) {
    return DynamicType_rch(this, OpenDDS::DCPS::inc_count());
  }
  return get_descriptor().base_type->get_base_type();
}

bool operator==(const DynamicType& lhs, const DynamicType& rhs)
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return lhs.test_equality_i(rhs, dt_ptr_pair);
}

bool test_equality_i(const DynamicType_rch& lhs, const DynamicType_rch& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.in() == rhs.in()) {
    return true;
  } else if (lhs.in() == 0 || rhs.in() == 0) {
    return false;
  } else {
    return lhs->test_equality_i(*rhs, dt_ptr_pair);
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
