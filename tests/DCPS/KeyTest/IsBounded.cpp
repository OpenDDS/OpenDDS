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

// Rather than just checking the reported bound against a hand-derived
// constant, actually serialize a sample into a buffer sized to that bound:
// a too-small bound (e.g. one missing a mutable member's parameter-list
// header, or one sized off a variable-size member's maximum payload when a
// smaller payload actually needs a bigger XCDR2 member header) will fail to
// hold a real sample even though it agrees with a wrong expectation.
template<typename Type>
bool assert_serializes_within_bound(
  const Encoding& enc, const char* type_name, const Type& value, bool key_only)
{
  const SerializedSizeBound bound = key_only
    ? MarshalTraits<Type>::key_only_serialized_size_bound(enc)
    : MarshalTraits<Type>::serialized_size_bound(enc);
  if (!bound) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C (%C): "
      "expected a finite %Cbound, but the type is unbounded\n",
      type_name, enc.to_string().c_str(), key_only ? "key-only " : ""));
    return true;
  }
  ACE_Message_Block buffer(bound.get());
  Serializer serializer(&buffer, enc);
  const bool ok = key_only ? (serializer << KeyOnly<const Type>(value)) : (serializer << value);
  if (!ok) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C (%C): "
      "failed to serialize %Cinto a buffer sized to the reported bound (%C)\n",
      type_name, enc.to_string().c_str(), key_only ? "key-only " : "", bound.to_string().c_str()));
    return true;
  }
  return false;
}

// _OpenDDS_KeyLessThan (DDSTraits<Type>::LessThanType) is the actual
// DataReader InstanceMap ordering, not just a convenience: samples with
// different keys must compare distinct, and samples with equal keys
// (including, for a sequence key, equal content reached through different
// allocations, e.g. after a copy) must compare equal.
template<typename Type>
bool assert_distinct_by_key(const char* type_name, const Type& a, const Type& b)
{
  const typename DDSTraits<Type>::LessThanType less;
  if (!(less(a, b) || less(b, a))) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C: "
      "expected samples with different keys to compare distinct, but the "
      "LessThan comparator treats them as equal\n", type_name));
    return true;
  }
  return false;
}

template<typename Type>
bool assert_same_by_key(const char* type_name, const Type& a, const Type& b)
{
  const typename DDSTraits<Type>::LessThanType less;
  if (less(a, b) || less(b, a)) {
    ACE_ERROR((LM_ERROR, "KeyTest/IsBounded: ERROR: %C: "
      "expected samples with equal keys to compare equal, but the "
      "LessThan comparator treats them as distinct\n", type_name));
    return true;
  }
  return false;
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

  // A mutable key-only bound has to include each key field's own
  // parameter-list header, not just concatenate key field sizes.
  {
    Bound::MutableKeyMessage value;
    value.key_field = 1;
    value.other_field = 2;
    failed |= assert_serializes_within_bound(
      xcdr1_encoding, "Bound::MutableKeyMessage key-only (XCDR1)", value, true);
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::MutableKeyMessage key-only (XCDR2)", value, true);
  }

  // A variable-size mutable member's bound has to account for XCDR2's
  // non-monotonic member-header overhead: a shorter payload than the
  // member's maximum can need the bigger 8-byte NEXTINT header even when
  // the maximum payload itself would fit the compact 4-byte form.
  {
    Bound::VariableMutableMessage shorter, longest;
    shorter.value = "ab";
    longest.value = "abc";
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::VariableMutableMessage \"ab\" (XCDR2)", shorter, false);
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::VariableMutableMessage \"abc\" (XCDR2)", longest, false);
  }

  // The same non-monotonic header case, for a mutable union branch.
  {
    Bound::VariableMutableUnion shorter, longest;
    shorter.value("ab");
    longest.value("abc");
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::VariableMutableUnion \"ab\" (XCDR2)", shorter, false);
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::VariableMutableUnion \"abc\" (XCDR2)", longest, false);
  }

  // A mutable union's key-only (here, the whole union, since the
  // discriminator is its only content) bound has to include the XCDR1 list
  // end sentinel that its key-only serializer always emits.
  {
    Bound::MutableKeyUnion value;
    value.value(1);
    failed |= assert_serializes_within_bound(
      xcdr1_encoding, "Bound::MutableKeyUnion key-only (XCDR1)", value, true);
    failed |= assert_serializes_within_bound(
      xcdr2_encoding, "Bound::MutableKeyUnion key-only (XCDR2)", value, true);
  }

  // A sequence key element's own sequence key field must still contribute
  // to the ordering, not be silently skipped just for being nested.
  {
    Bound::NestedSeqKeyElement elem_a, elem_b;
    elem_a.value.length(1);
    elem_a.value[0] = 1;
    elem_b.value.length(1);
    elem_b.value[0] = 2;

    Bound::NestedSeqKeyMessage a, b;
    a.value.length(1);
    a.value[0] = elem_a;
    b.value.length(1);
    b.value[0] = elem_b;
    failed |= assert_distinct_by_key("Bound::NestedSeqKeyMessage", a, b);
  }

  // In the classic C++ mapping, a sequence<string>/sequence<wstring> key's
  // element access returns a raw pointer, not TAO::String_Manager, so
  // comparing it with plain `<` would compare addresses instead of content.
  {
    Bound::StringSeqKeyMessage same_a, same_b, different;
    same_a.value.length(1);
    same_a.value[0] = "same";
    same_b = same_a; // copy: same content, different allocation
    different.value.length(1);
    different.value[0] = "different";
    failed |= assert_same_by_key("Bound::StringSeqKeyMessage (equal content)", same_a, same_b);
    failed |= assert_distinct_by_key(
      "Bound::StringSeqKeyMessage (different content)", same_a, different);
  }

#ifndef OPENDDS_SAFETY_PROFILE
  {
    Bound::WStringSeqKeyMessage same_a, same_b, different;
    same_a.value.length(1);
    same_a.value[0] = L"same";
    same_b = same_a;
    different.value.length(1);
    different.value[0] = L"different";
    failed |= assert_same_by_key("Bound::WStringSeqKeyMessage (equal content)", same_a, same_b);
    failed |= assert_distinct_by_key(
      "Bound::WStringSeqKeyMessage (different content)", same_a, different);
  }
#endif

  return failed;
}
