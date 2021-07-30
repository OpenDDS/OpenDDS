#include "TypeDescriptor.h"

namespace OpenDDS {
namespace XTypes {

  bool TypeDescriptor::equals(const TypeDescriptor& other)
  {
    return true;
    //todo clayton
    // return (*this == other);
  }

  bool test_equality(const TypeDescriptor& lhs, const TypeDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    bool a = lhs.kind == rhs.kind;
    bool b = lhs.name == rhs.name;
    bool c = test_equality(lhs.base_type, rhs.base_type, dt_ptr_pair);
    bool d = test_equality(lhs.discriminator_type, rhs.discriminator_type, dt_ptr_pair);
    bool e = lhs.bound == rhs.bound;
    bool f = test_equality(lhs.element_type, rhs.element_type, dt_ptr_pair);
    bool g = test_equality(lhs.key_element_type, rhs.key_element_type, dt_ptr_pair);
    bool h = lhs.extensibility_kind == rhs.extensibility_kind;
    bool i = lhs.is_nested == rhs.is_nested;
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("TypeDescriptor: %b %b %b %b %b %b %b %b %b\n"),
    a, b, c, d, e, f, g, h, i));
    return a && b && c && d && e && f && g && h && i;
    // return
    //   lhs.kind == rhs.kind &&
    //   lhs.name == rhs.name &&
    //   lhs.base_type == rhs.base_type &&
    //   lhs.discriminator_type == rhs.discriminator_type &&
    //   lhs.bound == rhs.bound &&
    //   lhs.element_type.in() == rhs.element_type.in() &&
    //   lhs.key_element_type.in() == rhs.key_element_type.in() &&
    //   lhs.extensibility_kind == rhs.extensibility_kind &&
    //   lhs.is_nested == rhs.is_nested;
  }

} // namespace XTypes
} // namespace OpenDDS
