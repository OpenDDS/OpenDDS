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
  const char* type_name, bool expected_bounded, size_t expected_size)
{
  bool failed = false;
  Type value;

  const bool actual_bounded = MarshalTraits<Type>::gen_is_bounded_size();
  if (actual_bounded != expected_bounded) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C: "
      "expected to%C be bounded, but it is%C\n",
      type_name, expected_bounded ? "" : " not", actual_bounded ? "" : "n't"));
    failed = true;
  }

  const size_t actual_size = max_serialized_size(encoding, value);
  if (actual_size != expected_size) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C: "
      "expected max size to be %B, but it is %B\n",
      type_name, expected_size, actual_size));
    failed = true;
  }

  return failed;
}

template<typename Type>
bool assert_bounded(
  const char* type_name, size_t expected_size)
{
  return assert_impl<Type>(type_name, true, expected_size);
}

template<typename Type>
bool assert_unbounded(
  const char* type_name)
{
  return assert_impl<Type>(type_name, false, 0);
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  bool failed = false;

  failed |= assert_bounded<Bound::SimpleBoundedMessage>("Bound::SimpleBoundedMessage", 65);
  failed |= assert_unbounded<Bound::StringMessage>("Bound::StringMessage");
  failed |= assert_unbounded<Bound::WStringMessage>("Bound::WStringMessage");
  failed |= assert_bounded<Bound::SimpleBoundedArrayMessage>("Bound::SimpleBoundedArrayMessage", 7350);
  failed |= assert_unbounded<Bound::StringArrayMessage>("Bound::StringArrayMessage");
  failed |= assert_unbounded<Bound::WStringArrayMessage>("Bound::WStringArrayMessage");
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
