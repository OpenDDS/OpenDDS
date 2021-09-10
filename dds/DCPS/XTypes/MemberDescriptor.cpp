#include "MemberDescriptor.h"

#include "DynamicType.h"

#include "DCPS/DdsDcps_pch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

MemberDescriptor::MemberDescriptor()
  : id(MEMBER_ID_INVALID), index(0), try_construct_kind(DISCARD), is_key(false), is_optional(false), is_must_understand(false), is_shared(false), is_default_label(false)
{}

MemberDescriptor::~MemberDescriptor()
{}

bool MemberDescriptor::equals(const MemberDescriptor& other) const
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality_i(*this, other, dt_ptr_pair);
}

bool test_equality_i(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return
    lhs.name == rhs.name &&
    lhs.id == rhs.id &&
    test_equality_i(lhs.type, rhs.type, dt_ptr_pair) &&
    lhs.default_value == rhs.default_value &&
    lhs.index == rhs.index &&
    lhs.label == rhs.label &&
    lhs.try_construct_kind == rhs.try_construct_kind &&
    lhs.is_key == rhs.is_key &&
    lhs.is_optional == rhs.is_optional &&
    lhs.is_must_understand == rhs.is_must_understand &&
    lhs.is_shared == rhs.is_shared &&
    lhs.is_default_label == rhs.is_default_label;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
