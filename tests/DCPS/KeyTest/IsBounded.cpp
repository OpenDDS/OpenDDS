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
const Encoding xcdr1_encoding(Encoding::KIND_XCDR1);
const Encoding xcdr2_encoding(Encoding::KIND_XCDR2);

template<typename Type>
bool assert_impl(
  const Encoding& enc, const char* type_name, SerializedSizeBound expected_bound)
{
  const SerializedSizeBound actual_bound = MarshalTraits<Type>::serialized_size_bound(enc);
  // SerializedSizeBound has no operator==/!=, only operator bool(); comparing
  // it directly with != silently compares only bounded-ness (via that
  // implicit conversion) and ignores the actual bound value, so check both
  // explicitly.
  const bool match = static_cast<bool>(actual_bound) == static_cast<bool>(expected_bound) &&
    (!expected_bound || actual_bound.get() == expected_bound.get());
  if (!match) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C (%C): "
      "expected to bound to be %C, but it is %C\n",
      type_name, enc.to_string().c_str(),
      expected_bound.to_string().c_str(), actual_bound.to_string().c_str()));
    return true;
  }
  return false;
}

template<typename Type>
bool assert_bounded(
  const char* type_name, size_t expected_size)
{
  return assert_impl<Type>(encoding, type_name, SerializedSizeBound(expected_size));
}

template<typename Type>
bool assert_unbounded(
  const char* type_name)
{
  return assert_impl<Type>(encoding, type_name, SerializedSizeBound());
}

// The same, but against a specific XCDR encoding, for appendable and mutable
// types whose bound depends on the DHEADER / parameter-list header overhead
// that only XCDR1/XCDR2 have.
template<typename Type>
bool assert_bounded(
  const Encoding& enc, const char* type_name, size_t expected_size)
{
  return assert_impl<Type>(enc, type_name, SerializedSizeBound(expected_size));
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

  // Appendable/mutable structs and unions of bounded fields should
  // report a bound too, not just final ones. Expected sizes are hand-derived
  // from Serializer::write_parameter_id()'s header-size rules (see the
  // comments in BoundTest2.idl) and cross-checked against opendds_idl's
  // actual generated output.
  failed |= assert_bounded<Bound::AppendableBoundedMessage>(
    xcdr1_encoding, "Bound::AppendableBoundedMessage (XCDR1)", 6);
  failed |= assert_bounded<Bound::AppendableBoundedMessage>(
    xcdr2_encoding, "Bound::AppendableBoundedMessage (XCDR2)", 10);

  failed |= assert_bounded<Bound::MutableBoundedMessage>(
    xcdr1_encoding, "Bound::MutableBoundedMessage (XCDR1)", 20);
  failed |= assert_bounded<Bound::MutableBoundedMessage>(
    xcdr2_encoding, "Bound::MutableBoundedMessage (XCDR2)", 18);

  failed |= assert_bounded<Bound::MutableExtendedIdMessage>(
    xcdr1_encoding, "Bound::MutableExtendedIdMessage (XCDR1)", 20);
  failed |= assert_bounded<Bound::MutableExtendedIdMessage>(
    xcdr2_encoding, "Bound::MutableExtendedIdMessage (XCDR2)", 12);

  failed |= assert_bounded<Bound::MutableExtendedSizeMessage>(
    xcdr1_encoding, "Bound::MutableExtendedSizeMessage (XCDR1)", 70024);
  failed |= assert_bounded<Bound::MutableExtendedSizeMessage>(
    xcdr2_encoding, "Bound::MutableExtendedSizeMessage (XCDR2)", 70017);

  failed |= assert_bounded<Bound::MutableBoundedUnionMessage>(
    xcdr1_encoding, "Bound::MutableBoundedUnionMessage (XCDR1)", 20);
  failed |= assert_bounded<Bound::MutableBoundedUnionMessage>(
    // The wrapping struct is (implicitly) appendable, adding its own 4-byte
    // XCDR2 DHEADER on top of the mutable union's own DHEADER + parameter
    // list.
    xcdr2_encoding, "Bound::MutableBoundedUnionMessage (XCDR2)", 24);

  return failed;
}
