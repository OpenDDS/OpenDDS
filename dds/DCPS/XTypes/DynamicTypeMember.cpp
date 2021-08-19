#include "DCPS/DdsDcps_pch.h"
#include "DynamicTypeMember.h"

#include "MemberDescriptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicTypeMember::DynamicTypeMember()
  : descriptor_(DCPS::make_rch<XTypes::MemberDescriptor>())
{}

void DynamicTypeMember::get_descriptor(MemberDescriptor_rch& descriptor) const
{
  descriptor = descriptor_;
}

bool DynamicTypeMember::equals(const DynamicTypeMember& other) const
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality_i(*this, other, dt_ptr_pair);
}

MemberId DynamicTypeMember::get_id() const
{
  return descriptor_->id;
}

DCPS::String DynamicTypeMember::get_name() const
{
  return descriptor_->name;
}

bool test_equality_i(const DynamicTypeMember& lhs, const DynamicTypeMember& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return test_equality_i(*lhs.descriptor_, *rhs.descriptor_, dt_ptr_pair);
}

bool test_equality_i(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs.size() == rhs.size()) {
    for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
      if (rhs.find(it->first) != rhs.end()) {
        if (!test_equality_i(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair)) {
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
        if (!test_equality_i(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair)) {
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
