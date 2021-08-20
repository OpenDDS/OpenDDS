#include "DCPS/DdsDcps_pch.h"
#include "DynamicTypeMember.h"

#include "MemberDescriptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicTypeMember::DynamicTypeMember()
  : parent_(DCPS::make_rch<XTypes::DynamicType>()),
    descriptor_(DCPS::make_rch<XTypes::MemberDescriptor>())
{}

void DynamicTypeMember::get_descriptor(MemberDescriptor_rch& descriptor) const
{
  descriptor = descriptor_;
}

MemberDescriptor_rch& DynamicTypeMember::get_descriptor()
{
  return descriptor_;
}

DynamicType_rch DynamicTypeMember::get_parent()
{
  return parent_;
}

bool DynamicTypeMember::equals(const DynamicTypeMember& other) const
{
  //7.5.2.6.3 Operation: equals
  //Two members shall be considered equal if and only if they belong to the same type and all of
  //their respective properties, as identified in Table 52 above, are equal.

  //In addition to what the spec says to compare, we will be comparing the MemberDescriptors of both
  //DynamicTypeMembers. The spec seems to assume this is the case, despite not listing the MemberDescriptor
  //as a property in table 52. If this were not the case, any two members within a type would be considered
  //equal, regardless of whether they are actually the same member.
  DynamicTypePtrPairSeen dt_ptr_pair;
  return 
    test_equality_i(this->parent_, other.parent_, dt_ptr_pair) &&
    OpenDDS::XTypes::test_equality_i(*this->descriptor_.in(), *other.descriptor_.in(), dt_ptr_pair);
}

MemberId DynamicTypeMember::get_id() const
{
  return descriptor_->id;
}

DCPS::String DynamicTypeMember::get_name() const
{
  return descriptor_->name;
}

bool test_equality_i(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if (rhs.find(it->first) != rhs.end()) {
        if (!test_equality_i(*lhs.find(it->first)->second->get_descriptor(), *rhs.find(it->first)->second->get_descriptor(), dt_ptr_pair)) {
          return false;
        }
      } else {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

bool test_equality_i(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersById::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if(rhs.find(it->first) != rhs.end()) {
        if (!test_equality_i(*lhs.find(it->first)->second->get_descriptor(), *rhs.find(it->first)->second->get_descriptor(), dt_ptr_pair)) {
          return false;
        }
      } else {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
