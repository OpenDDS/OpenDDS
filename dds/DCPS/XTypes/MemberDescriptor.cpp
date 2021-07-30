#include "MemberDescriptor.h"

namespace OpenDDS {
namespace XTypes {
  bool MemberDescriptor::equals(const MemberDescriptor& descriptor)
  {
    return true;
    //todo clayton
    //return (*this == descriptor);
  }

  bool test_equality(const MemberDescriptor& lhs, const MemberDescriptor& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    //TODO CLAYTON : Uncomment and delete the code used for debugging upon completion of test
    bool a = lhs.name == rhs.name;
    bool b =  lhs.id == rhs.id;
    bool c =  test_equality(lhs.type, rhs.type, dt_ptr_pair);
    bool d =  lhs.default_value == rhs.default_value;
    bool e =  lhs.index == rhs.index;
    bool f =  lhs.label == rhs.label;
    bool g =  lhs.try_construct_kind == rhs.try_construct_kind;
    bool h =  lhs.is_key == rhs.is_key;
    bool i =  lhs.is_optional == rhs.is_optional;
    bool j =  lhs.is_must_understand == rhs.is_must_understand;
    bool k =  lhs.is_shared == rhs.is_shared;
    bool l =  lhs.is_default_label == rhs.is_default_label;
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("MemberDescriptor: %b %b %b %b %b %b %b %b %b %b %b %b\n"),
    a, b, c, d, e, f, g, h, i, j, k, l));
    return a && b && c && d && e && f && g && h && i && j && k && l;
    // return
    //   lhs.name == rhs.name &&
    //   lhs.id == rhs.id &&
    //   lhs.type == rhs.type &&
    //   lhs.default_value == rhs.default_value &&
    //   lhs.index == rhs.index &&
    //   lhs.label == rhs.label &&
    //   lhs.try_construct_kind == rhs.try_construct_kind &&
    //   lhs.is_key == rhs.is_key &&
    //   lhs.is_optional == rhs.is_optional &&
    //   lhs.is_must_understand == rhs.is_must_understand &&
    //   lhs.is_shared == rhs.is_shared &&
    //   lhs.is_default_label == rhs.is_default_label;
  }
} // namespace XTypes
} // namespace OpenDDS
