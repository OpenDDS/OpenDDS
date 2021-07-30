#include "TypeDescriptor.h"

namespace OpenDDS {
namespace XTypes {

bool TypeDescriptor::equals(const TypeDescriptor& other)
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(*this, other, dt_ptr_pair);
}

bool test_equality(const TypeDescriptor& lhs, const TypeDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return
    lhs.kind == rhs.kind &&
    lhs.name == rhs.name &&
    test_equality(lhs.base_type, rhs.base_type, dt_ptr_pair) &&
    test_equality(lhs.discriminator_type, rhs.discriminator_type, dt_ptr_pair) &&
    lhs.bound == rhs.bound &&
    test_equality(lhs.element_type, rhs.element_type, dt_ptr_pair) &&
    test_equality(lhs.key_element_type, rhs.key_element_type, dt_ptr_pair) &&
    lhs.extensibility_kind == rhs.extensibility_kind &&
    lhs.is_nested == rhs.is_nested;
}

} // namespace XTypes
} // namespace OpenDDS
