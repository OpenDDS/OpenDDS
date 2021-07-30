#include "MemberDescriptor.h"

namespace OpenDDS {
namespace XTypes {

bool MemberDescriptor::equals(const MemberDescriptor& other)
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(*this, other, dt_ptr_pair);
}

bool test_equality(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return
    lhs.name == rhs.name &&
    lhs.id == rhs.id &&
    test_equality(lhs.type, rhs.type, dt_ptr_pair) &&
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
