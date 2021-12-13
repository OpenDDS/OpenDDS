#include <DCPS/DdsDcps_pch.h>

#include "TypeDescriptor.h"

#include "DynamicType.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

TypeDescriptor::TypeDescriptor()
  : kind(TK_NONE)
  , extensibility_kind(FINAL)
  , is_nested(false)
{}

TypeDescriptor::~TypeDescriptor()
{}

bool TypeDescriptor::equals(const TypeDescriptor& other) const
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality_i(*this, other, dt_ptr_pair);
}

bool test_equality_i(const TypeDescriptor& lhs, const TypeDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  return
    lhs.kind == rhs.kind &&
    lhs.name == rhs.name &&
    test_equality_i(lhs.base_type, rhs.base_type, dt_ptr_pair) &&
    test_equality_i(lhs.discriminator_type, rhs.discriminator_type, dt_ptr_pair) &&
    lhs.bound == rhs.bound &&
    test_equality_i(lhs.element_type, rhs.element_type, dt_ptr_pair) &&
    test_equality_i(lhs.key_element_type, rhs.key_element_type, dt_ptr_pair) &&
    lhs.extensibility_kind == rhs.extensibility_kind &&
    lhs.is_nested == rhs.is_nested;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
