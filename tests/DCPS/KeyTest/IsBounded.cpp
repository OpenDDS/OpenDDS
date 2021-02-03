/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BoundTestTypeSupportImpl.h"
#include "BoundTest2TypeSupportImpl.h"

#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/Serializer.h>

#include <ace/OS_main.h>

#include <iostream>

using namespace OpenDDS::DCPS;

const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);

template<typename Type>
bool assert_impl(
  const char* type_name, SerializedSizeBound expected_bound)
{
  const SerializedSizeBound actual_bound = MarshalTraits<Type>::serialized_size_bound(encoding);
  if (actual_bound != expected_bound) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C: "
      "expected to bound to be %C, but it is %C\n",
      type_name, expected_bound.to_string().c_str(), actual_bound.to_string().c_str()));
    return true;
  }
  return false;
}

template<typename Type>
bool assert_bounded(
  const char* type_name, size_t expected_size)
{
  return assert_impl<Type>(type_name, SerializedSizeBound(expected_size));
}

template<typename Type>
bool assert_unbounded(
  const char* type_name)
{
  return assert_impl<Type>(type_name, SerializedSizeBound());
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  bool failed = false;

  failed |= assert_bounded<Bound::SimpleBoundedMessage>("Bound::SimpleBoundedMessage", 65);
  failed |= assert_unbounded<Bound::StringMessage>("Bound::StringMessage");

#ifndef OPENDDS_SAFETY_PROFILE
  failed |= assert_unbounded<Bound::WStringMessage>("Bound::WStringMessage");
#endif

  failed |= assert_bounded<Bound::SimpleBoundedArrayMessage>("Bound::SimpleBoundedArrayMessage", 7350);
  failed |= assert_unbounded<Bound::StringArrayMessage>("Bound::StringArrayMessage");

#ifndef OPENDDS_SAFETY_PROFILE
  failed |= assert_unbounded<Bound::WStringArrayMessage>("Bound::WStringArrayMessage");
#endif

  failed |= assert_bounded<Bound::BoundedNestedMessage>("Bound::BoundedNestedMessage", 10);
  failed |= assert_unbounded<Bound::UnboundedNestedMessage>("Bound::UnboundedNestedMessage");
  failed |= assert_bounded<Bound::BoundedSequenceOfBoundedMessage>("Bound::BoundedSequenceOfBoundedMessage", 84);
  failed |= assert_unbounded<Bound::UnboundedSequenceOfBoundedMessage>("Bound::UnboundedSequenceOfBoundedMessage");
  failed |= assert_unbounded<Bound::BoundedSequenceOfUnboundedMessage>("Bound::BoundedSequenceOfUnboundedMessage");
  failed |= assert_unbounded<Bound::UnboundedSequenceOfUnboundedMessage>("Bound::UnboundedSequenceOfUnboundedMessage");
  failed |= assert_bounded<Bound::BoundedUnionMessage>("Bound::BoundedUnionMessage", 6);
  failed |= assert_unbounded<Bound::UnboundedUnionMessage>("Bound::UnboundedUnionMessage");
  failed |= assert_unbounded<Bound::RecursiveMessage>("Bound::RecursiveMessage");

  return failed;
}
