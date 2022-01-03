#include <DCPS/DdsDcps_pch.h>

#include "MemberDescriptor.h"

#include "DynamicType.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

MemberDescriptor::MemberDescriptor()
  : id(MEMBER_ID_INVALID)
  , index(0)
  , try_construct_kind(DISCARD)
  , is_key(false)
  , is_optional(false)
  , is_must_understand(false)
  , is_shared(false)
  , is_default_label(false)
{}

MemberDescriptor::~MemberDescriptor()
{}

DynamicType_rch MemberDescriptor::get_type() const
{
  DynamicType_rch strong_type = type.lock();
  if (strong_type.in() == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) MemberDescriptor::get_type(): grabbing of lock failed\n")));
  }
  return strong_type;
}

bool MemberDescriptor::equals(const MemberDescriptor& other) const
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality_i(*this, other, dt_ptr_pair);
}

bool test_equality_i(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  DynamicType_rch lhs_type = lhs.get_type();
  DynamicType_rch rhs_type = rhs.get_type();
  return
    lhs.name == rhs.name &&
    lhs.id == rhs.id &&
    test_equality_i(lhs_type, rhs_type, dt_ptr_pair) &&
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
