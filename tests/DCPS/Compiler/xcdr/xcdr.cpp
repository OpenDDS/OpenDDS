// TODO: Add deserialization only tests that have unknown parameters with
// must understand that cause an expected failure.
// This should for generated deserialization code, but probably doesn't work with
// DynamicData.

#include "xcdrbasetypesTypeSupportImpl.h"
#include "appendable_mixedTypeSupportImpl.h"
#include "mutable_typesTypeSupportImpl.h"
#include "mutable_types2TypeSupportImpl.h"
#include "keyonlyTypeSupportImpl.h"
#include "optionalTypeSupportImpl.h"

#include <tests/Utils/DataView.h>
#include <tests/Utils/GtestRc.h>

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/Xcdr2ValueWriter.h>
#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h>

#include <gtest/gtest.h>

#include <string>
#include <cstring>
#include <vector>
#include <sstream>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

const Encoding xcdr1(Encoding::KIND_XCDR1, ENDIAN_BIG);
const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);
const Encoding xcdr2_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);

enum FieldFilter {
  FieldFilter_All,
  FieldFilter_NestedKeyOnly,
  FieldFilter_KeyOnly
};

bool dynamic = false;

#ifndef OPENDDS_SAFETY_PROFILE
TypeLookupService_rch tls;

template<typename TopicType>
void add_type()
{
  if (!tls) {
    tls = make_rch<TypeLookupService>();
  }
  TypeIdentifierPairSeq tid_pairs;
  TypeIdentifierPair tid_pair;
  typedef typename DDSTraits<TopicType>::XtagType Xtag;
  tid_pair.type_identifier1 = getCompleteTypeIdentifier<Xtag>();
  tid_pair.type_identifier2 = getMinimalTypeIdentifier<Xtag>();
  tid_pairs.append(tid_pair);
  tls->update_type_identifier_map(tid_pairs);

  typename DDSTraits<TopicType>::TypeSupportImplType tsi;
  tsi.add_types(tls);
}

template<typename TopicType>
DDS::DynamicType_var get_dynamic_type()
{
  typedef typename DDSTraits<TopicType>::XtagType Xtag;
  const TypeIdentifier& com_ti = getCompleteTypeIdentifier<Xtag>();
  const TypeMap& com_map = getCompleteTypeMap<Xtag>();
  TypeMap::const_iterator pos = com_map.find(com_ti);
  EXPECT_TRUE(pos != com_map.end());
  const TypeObject& com_to = pos->second;
  return tls->complete_to_dynamic(com_to.complete, GUID_UNKNOWN);
}
#endif

template <typename Type>
void set_base_values(Type& value)
{
  value.short_field() = 0x7fff;
  value.long_field() = 0x7fffffff;
  value.octet_field() = 0x01;
  value.long_long_field() = 0x7fffffffffffffff;
}

template <typename Type>
void set_base_values_union(Type& value, UnionDisc disc)
{
  switch (disc) {
  case UnionDisc::E_SHORT_FIELD:
    value.short_field(0x7fff);
    break;
  case UnionDisc::E_LONG_FIELD:
    value.long_field(0x7fffffff);
    break;
  case UnionDisc::E_OCTET_FIELD:
    value.octet_field(0x01);
    break;
  case UnionDisc::E_LONG_LONG_FIELD:
    value.long_long_field(0x7fffffffffffffff);
    break;
  default:
    value._d(disc);
    break;
  }
}

template <typename Type>
void set_values(Type& value)
{
  set_base_values(value);
}

template <typename Type, typename WrapperType>
void set_values(Type& /*value*/)
{
  ACE_ASSERT(false);
}

template <typename Type>
void set_values_union(Type& value, UnionDisc disc)
{
  set_base_values_union(value, disc);
}

template <typename Type>
void expect_arrays_are_equal(const Type& a, const Type& b)
{
  ASSERT_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base(const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a.short_field(), b.short_field());
  EXPECT_EQ(a.long_field(), b.long_field());
  EXPECT_EQ(a.octet_field(), b.octet_field());
  EXPECT_EQ(a.long_long_field(), b.long_long_field());
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base_union(const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a._d(), b._d());
  switch (a._d()) {
  case UnionDisc::E_SHORT_FIELD:
    EXPECT_EQ(a.short_field(), b.short_field());
    break;
  case UnionDisc::E_LONG_FIELD:
    EXPECT_EQ(a.long_field(), b.long_field());
    break;
  case UnionDisc::E_OCTET_FIELD:
    EXPECT_EQ(a.octet_field(), b.octet_field());
    break;
  case UnionDisc::E_LONG_LONG_FIELD:
    EXPECT_EQ(a.long_long_field(), b.long_long_field());
    break;
  default:
    break;
  }
}

template<typename TypeA, typename TypeB>
void expect_values_equal(const TypeA& a, const TypeB& b)
{
  expect_values_equal_base(a, b);
}

template<typename TypeA, typename TypeB>
::testing::AssertionResult assert_values(
  const char* a_expr, const char* b_expr,
  const TypeA& a, const TypeB& b)
{
  expect_values_equal(a, b);
  if (::testing::Test::HasFailure()) {
    return ::testing::AssertionFailure() << a_expr << " != " << b_expr;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult assert_SerializedSizeBound(
  const char* a_expr, const char* b_expr,
  const SerializedSizeBound& a, const SerializedSizeBound& b)
{
  return a == b ? ::testing::AssertionSuccess() :
    (::testing::AssertionFailure()
      << a_expr << " (" << a.to_string() << ") isn't the same as "
      << b_expr << " (" << b.to_string() << ").\n");
}

template<typename T>
void deserialize_compare(
  const Encoding& encoding, const DataView& data, const T& expected)
{
  ACE_Message_Block mb(data.size);
  mb.copy((const char*)data.data, data.size);
  Serializer serializer(&mb, encoding);
  T result;
  ASSERT_TRUE(serializer >> result);
  EXPECT_PRED_FORMAT2(assert_values, expected, result);
}

template<typename TypeA, typename RealTypeA, typename TypeB, typename RealTypeB>
void amalgam_serializer_test_base(
  const Encoding& encoding, const DataView& expected_cdr,
  const TypeA& value, TypeB& result, FieldFilter field_filter = FieldFilter_All)
{
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  const bool key_only = field_filter == FieldFilter_KeyOnly;
#else
  ACE_UNUSED_ARG(field_filter);
#endif
  ACE_Message_Block buffer(1024);

  // Serialize and Compare CDR
  {
    Serializer serializer(&buffer, encoding);
    if (dynamic) {
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
      typename DDSTraits<RealTypeA>::TypeSupportImplType tsi;
      DDS::DynamicData_var dd;
      ASSERT_RC_OK(tsi.create_dynamic_sample_rc(dd, value));

      DDS::DynamicData_ptr dd_ptr = dd.in();
      if (key_only) {
        const KeyOnly<DDS::DynamicData_ptr> key_only(dd_ptr);
        EXPECT_EQ(serialized_size(encoding, key_only), expected_cdr.size);
        ASSERT_TRUE(serializer << key_only);
      } else {
        EXPECT_EQ(serialized_size(encoding, dd_ptr), expected_cdr.size);
        ASSERT_TRUE(serializer << dd_ptr);
      }
#else
      ASSERT_TRUE(false);
#endif
    } else {
      ASSERT_TRUE(serializer << value);
    }
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

  // Deserialize
  {
    Serializer serializer(&buffer, encoding);
    if (dynamic) {
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
      add_type<RealTypeB>();
      DDS::DynamicType_var type = get_dynamic_type<RealTypeB>();
      DDS::DynamicData_var ddi = new DynamicDataXcdrReadImpl(serializer, type,
        key_only ? Sample::KeyOnly: Sample::Full);
      typename DDSTraits<RealTypeB>::TypeSupportImplType tsi;
      ASSERT_RC_OK(tsi.create_sample_rc(result, ddi));
#else
      ASSERT_TRUE(false);
#endif
    } else {
      ASSERT_TRUE(serializer >> result);
    }
  }
}

template<typename TypeA, typename TypeB>
void amalgam_serializer_test(
  const Encoding& encoding, const DataView& expected_cdr, TypeA& value, TypeB& result)
{
  amalgam_serializer_test_base<TypeA, TypeA, TypeB, TypeB>(encoding, expected_cdr, value, result);
  EXPECT_PRED_FORMAT2(assert_values, value, result);
}

template<typename TypeA, typename TypeB>
void amalgam_serializer_test(const Encoding& encoding, const DataView& expected_cdr)
{
  TypeA value;
  set_values(value);
  TypeB result;
  amalgam_serializer_test<TypeA, TypeB>(encoding, expected_cdr, value, result);
}

template<typename TypeA, typename TypeB>
void amalgam_serializer_test_union(const Encoding& encoding, const DataView& expected_cdr, UnionDisc disc)
{
  TypeA value;
  set_values_union(value, disc);
  TypeB result;
  amalgam_serializer_test<TypeA, TypeB>(encoding, expected_cdr, value, result);
}

template<typename Type>
void serializer_test(const Encoding& encoding, const DataView& expected_cdr)
{
  amalgam_serializer_test<Type, Type>(encoding, expected_cdr);
}

template<typename Type>
void serializer_test_union(const Encoding& encoding, const DataView& expected_cdr, UnionDisc disc)
{
  amalgam_serializer_test_union<Type, Type>(encoding, expected_cdr, disc);
}

template <typename Type>
void baseline_checks_vwrite(const Encoding& encoding, const Type& value, const DataView& expected_cdr)
{
  if (encoding.kind() == Encoding::KIND_XCDR2) {
    // Compute serialized size
    Xcdr2ValueWriter value_writer(encoding);
    EXPECT_TRUE(vwrite(value_writer, value));

    // Serialize
    ACE_Message_Block buffer(value_writer.get_serialized_size());
    Serializer ser(&buffer, encoding);
    value_writer.set_serializer(&ser);
    EXPECT_TRUE(vwrite(value_writer, value));
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
}

template<typename Type>
void baseline_checks(const Encoding& encoding, const DataView& expected_cdr,
  SerializedSizeBound bound = SerializedSizeBound())
{
  EXPECT_PRED_FORMAT2(assert_SerializedSizeBound,
    MarshalTraits<Type>::serialized_size_bound(encoding), bound);

  /*
   * TODO(iguessthislldo): This is a workaround for not properly implementing
   * serialized_size_bound for XCDR. See XTYPE-83.
   */
  const bool not_final = MarshalTraits<Type>::extensibility() != FINAL;
  const bool xcdr = encoding.xcdr_version() != Encoding::XCDR_VERSION_NONE;
  const SerializedSizeBound expected_key_only_bound = not_final && xcdr ?
    SerializedSizeBound() : SerializedSizeBound(0);
  EXPECT_PRED_FORMAT2(assert_SerializedSizeBound,
    MarshalTraits<Type>::key_only_serialized_size_bound(encoding), expected_key_only_bound);

  Type value;
  set_values(value);
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);
  serializer_test<Type>(encoding, expected_cdr);

  baseline_checks_vwrite(encoding, value, expected_cdr);
}

template<typename Type>
void baseline_checks_union(const Encoding& encoding, const DataView& expected_cdr, UnionDisc disc)
{
  Type value;
  set_values_union(value, disc);
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);
  serializer_test_union<Type>(encoding, expected_cdr, disc);

  baseline_checks_vwrite(encoding, value, expected_cdr);
}

#define STREAM_DATA \
  static const unsigned char expected[]; \
  static const unsigned layout[];

void change_endianness(
  unsigned char* strm, const unsigned* layout, const size_t layout_size)
{
  size_t begin = 0;
  for (size_t i = 0; i < layout_size; ++i) {
    const unsigned length = layout[i];
    // Swap bytes
    for (size_t j = 0; j < length/2; ++j) {
      std::swap(strm[begin + j], strm[begin + length - 1 - j]);
    }
    begin += length;
  }
}

template<typename BigEndianData>
unsigned char* setup_little_endian()
{
  size_t length = sizeof(BigEndianData::expected);
  unsigned char* strm_le = new unsigned char[length];
  std::memcpy(strm_le, BigEndianData::expected, length);
  change_endianness(strm_le, BigEndianData::layout,
                    sizeof(BigEndianData::layout)/sizeof(unsigned));
  return strm_le;
}

template<typename Type, typename BigEndianData>
void test_little_endian()
{
  unsigned char* strm_le = setup_little_endian<BigEndianData>();
  serializer_test<Type>(
    xcdr2_le, DataView(strm_le, sizeof(BigEndianData::expected)));
  delete[] strm_le;
}

template<typename Type, typename BigEndianData>
void test_little_endian_union(UnionDisc disc)
{
  unsigned char* strm_le = setup_little_endian<BigEndianData>();
  serializer_test_union<Type>(
    xcdr2_le, DataView(strm_le, sizeof(BigEndianData::expected)), disc);
  delete[] strm_le;
}

template<typename TypeA, typename TypeB, typename BigEndianData>
void test_little_endian()
{
  unsigned char* strm_le = setup_little_endian<BigEndianData>();
  amalgam_serializer_test<TypeA, TypeB>(
    xcdr2_le, DataView(strm_le, sizeof(BigEndianData::expected)));
  delete[] strm_le;
}

template<typename TypeA, typename TypeB, typename BigEndianData>
void test_little_endian_union(UnionDisc disc)
{
  unsigned char* strm_le = setup_little_endian<BigEndianData>();
  amalgam_serializer_test_union<TypeA, TypeB>(
    xcdr2_le, DataView(strm_le, sizeof(BigEndianData::expected)), disc);
  delete[] strm_le;
}

// XCDR1 =====================================================================

const unsigned char final_xcdr1_struct_expected[] = {
  // short_field
  0x7f, 0xff, // +2 = 2
  // long_field
  0x00, 0x00, // +2 pad = 4
  0x7f, 0xff, 0xff, 0xff, // +4 = 8
  // octet_field
  0x01, // +1 = 9
  // long_long_field
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +7 pad = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 24
};
const size_t final_xcdr1_struct_max_size = 24;

TEST(BasicTests, FinalXcdr1Struct)
{
  baseline_checks<FinalStruct>(xcdr1, final_xcdr1_struct_expected, final_xcdr1_struct_max_size);
}

const DataView appendable_xcdr1_struct_expected(final_xcdr1_struct_expected);

TEST(BasicTests, AppendableXcdr1Struct)
{
  baseline_checks<AppendableStruct>(xcdr1, appendable_xcdr1_struct_expected);
}

const unsigned char mutable_xcdr1_struct_expected[] = {
  // short_field
  0xc0, 0x00, 0x00, 0x02, // PID +4 = 4
  0x7f, 0xff, // +2 = 6
  // long_field
  0x00, 0x00, // +2 pad = 8
  0xc0, 0x01, 0x00, 0x04, // PID +4 = 12
  0x7f, 0xff, 0xff, 0xff, // +4 = 16
  // octet_field
  0xc0, 0x02, 0x00, 0x01, // PID +4 = 20
  0x01, // +1 = 21
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 24
  0xc0, 0x03, 0x00, 0x08, // PID +4 = 28
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 36
  // End of List
  0x3f, 0x02, 0x00, 0x00 // +4 = 40
};

TEST(BasicTests, MutableXcdr1Struct)
{
  baseline_checks<MutableStruct>(xcdr1, mutable_xcdr1_struct_expected);
}

// XCDR2 =====================================================================

struct FinalXcdr2StructExpectedBE {
  STREAM_DATA
};

const unsigned char FinalXcdr2StructExpectedBE::expected[] = {
  // short_field
  0x7f, 0xff, // +2 = 2
  // long_field
  0x00, 0x00, // +2 pad = 4
  0x7f, 0xff, 0xff, 0xff, // +4 = 8
  // octet_field
  0x01, // +1 = 9
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 12
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff // +8 = 20
};
const unsigned FinalXcdr2StructExpectedBE::layout[] = {2,2,4,1,3,8};
const size_t final_xcdr2_struct_max_size = 20;

TEST(BasicTests, FinalXcdr2Struct)
{
  baseline_checks<FinalStruct>(xcdr2, FinalXcdr2StructExpectedBE::expected, final_xcdr2_struct_max_size);
}

TEST(BasicTests, FinalXcdr2StructLE)
{
  test_little_endian<FinalStruct, FinalXcdr2StructExpectedBE>();
}

struct AppendableXcdr2StructExpectedBE {
  STREAM_DATA
};

const unsigned char AppendableXcdr2StructExpectedBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x14, // +4 = 4
  // short_field
  0x7f, 0xff, // +2 = 6
  // long_field
  0x00, 0x00, // +2 pad = 8
  0x7f, 0xff, 0xff, 0xff, // +4 = 12
  // octet_field
  0x01, // +1 = 13
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff // +8 = 24
};
const unsigned AppendableXcdr2StructExpectedBE::layout[] = {4,2,2,4,1,3,8};

TEST(BasicTests, AppendableXcdr2Struct)
{
  baseline_checks<AppendableStruct>(xcdr2, AppendableXcdr2StructExpectedBE::expected);
}

TEST(BasicTests, AppendableXcdr2StructLE)
{
  test_little_endian<AppendableStruct, AppendableXcdr2StructExpectedBE>();
}

struct MutableXcdr2StructExpectedBE {
  STREAM_DATA
};

const unsigned char MutableXcdr2StructExpectedBE::expected[] = {
  0x00,0,0,0x24, // +4=4 Delimiter
  0x10,0,0,0x00, 0x7f,0xff,  (0),(0), // +4+2+(2)=12 short_field
  0x20,0,0,0x01, 0x7f,0xff,0xff,0xff, // +4+4    =20 long_field
  0x00,0,0,0x02, 0x01,   (0),(0),(0), // +4+1+(3)=28 octet_field
  0x30,0,0,0x03, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +4+8=40 long_long_field
};
const unsigned MutableXcdr2StructExpectedBE::layout[] = {4,4,2,2,4,4,4,1,3,4,8};

TEST(BasicTests, MutableXcdr2Struct)
{
  baseline_checks<MutableStruct>(xcdr2, MutableXcdr2StructExpectedBE::expected);
}

TEST(BasicTests, MutableXcdr2StructLE)
{
  test_little_endian<MutableStruct, MutableXcdr2StructExpectedBE>();
}

// Union Tests -----------------------------------------------------

struct MutableXcdr2UnionExpectedShortBE {
  STREAM_DATA
};

const unsigned char MutableXcdr2UnionExpectedShortBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0e, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x00, // +4 value = 12
  // short_field
  0x10, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 16
  0x7f, 0xff // +2 value = 18
};
const unsigned MutableXcdr2UnionExpectedShortBE::layout[] = {4,4,4,4,2};

struct MutableXcdr2UnionExpectedLongBE {
  STREAM_DATA
};

const unsigned char MutableXcdr2UnionExpectedLongBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x10, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x01, // +4 value = 12
  // long_field
  0x20, 0x00, 0x00, 0x01,// +4 EMHEADER1 = 16
  0x7f, 0xff, 0xff, 0xff // +4 value = 20
};
const unsigned MutableXcdr2UnionExpectedLongBE::layout[] = {4,4,4,4,4};

struct MutableXcdr2UnionExpectedOctetBE {
  STREAM_DATA
};

const unsigned char MutableXcdr2UnionExpectedOctetBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0d, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x02, // +4 value = 12
  // octet_field
  0x00, 0x00, 0x00, 0x02, // +4 EMHEADER1 = 16
  0x01                    // +1 value = 17
};
const unsigned MutableXcdr2UnionExpectedOctetBE::layout[] = {4,4,4,4,1};

struct MutableXcdr2UnionExpectedLongLongBE {
  STREAM_DATA
};

const unsigned char MutableXcdr2UnionExpectedLongLongBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x14, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x03, // +4 value = 12
  // long_long_field
  0x30, 0x00, 0x00, 0x03, // +4 EMHEADER1 = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  // +8 value = 24
};
const unsigned MutableXcdr2UnionExpectedLongLongBE::layout[] = {4,4,4,4,8};

template<>
void expect_values_equal(const MutableUnion& a, const MutableUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(BasicTests, MutableXcdr12Union)
{
  baseline_checks_union<MutableUnion>(xcdr2, MutableXcdr2UnionExpectedShortBE::expected, UnionDisc::E_SHORT_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, MutableXcdr2UnionExpectedLongBE::expected, UnionDisc::E_LONG_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, MutableXcdr2UnionExpectedOctetBE::expected, UnionDisc::E_OCTET_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, MutableXcdr2UnionExpectedLongLongBE::expected, UnionDisc::E_LONG_LONG_FIELD);
}

TEST(BasicTests, MutableXcdr12UnionLE)
{
  test_little_endian_union<MutableUnion, MutableXcdr2UnionExpectedShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<MutableUnion, MutableXcdr2UnionExpectedLongBE>(UnionDisc::E_LONG_FIELD);
  test_little_endian_union<MutableUnion, MutableXcdr2UnionExpectedOctetBE>(UnionDisc::E_OCTET_FIELD);
  test_little_endian_union<MutableUnion, MutableXcdr2UnionExpectedLongLongBE>(UnionDisc::E_LONG_LONG_FIELD);
}

// ---------- FinalUnion
struct FinalUnionExpectedShortBE {
  STREAM_DATA
};

const unsigned char FinalUnionExpectedShortBE::expected[] = {
  0x00, 0x00, 0x00, 0x00, // +4 discriminator = 4
  0x7f, 0xff // +2 short_field = 6
};
const unsigned FinalUnionExpectedShortBE::layout[] = {4,2};

struct FinalUnionExpectedLongBE {
  STREAM_DATA
};

const unsigned char FinalUnionExpectedLongBE::expected[] = {
  0x00, 0x00, 0x00, 0x01, // +4 discriminator = 4
  0x7f, 0xff, 0xff, 0xff // +4 long_field = 8
};
const unsigned FinalUnionExpectedLongBE::layout[] = {4,4};

struct FinalUnionExpectedOctetBE {
  STREAM_DATA
};

const unsigned char FinalUnionExpectedOctetBE::expected[] = {
  0x00, 0x00, 0x00, 0x02, // +4 discriminator = 4
  0x01 // +1 octet_field = 5
};
const unsigned FinalUnionExpectedOctetBE::layout[] = {4,1};

struct FinalUnionExpectedLongLongBE {
  STREAM_DATA
};

const unsigned char FinalUnionExpectedLongLongBE::expected[] = {
  0x00, 0x00, 0x00, 0x03, // +4 discriminator = 4
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff // +8 long_long_field = 12
};
const unsigned FinalUnionExpectedLongLongBE::layout[] = {4,8};

template<>
void expect_values_equal(const FinalUnion& a, const FinalUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(BasicTests, FinalUnion)
{
  baseline_checks_union<FinalUnion>(xcdr2, FinalUnionExpectedShortBE::expected, UnionDisc::E_SHORT_FIELD);
  baseline_checks_union<FinalUnion>(xcdr2, FinalUnionExpectedLongBE::expected, UnionDisc::E_LONG_FIELD);
  baseline_checks_union<FinalUnion>(xcdr2, FinalUnionExpectedOctetBE::expected, UnionDisc::E_OCTET_FIELD);
  baseline_checks_union<FinalUnion>(xcdr2, FinalUnionExpectedLongLongBE::expected, UnionDisc::E_LONG_LONG_FIELD);
}

TEST(BasicTests, FinalUnionLE)
{
  test_little_endian_union<FinalUnion, FinalUnionExpectedShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<FinalUnion, FinalUnionExpectedLongBE>(UnionDisc::E_LONG_FIELD);
  test_little_endian_union<FinalUnion, FinalUnionExpectedOctetBE>(UnionDisc::E_OCTET_FIELD);
  test_little_endian_union<FinalUnion, FinalUnionExpectedLongLongBE>(UnionDisc::E_LONG_LONG_FIELD);
}

// Appendable Tests ==========================================================

TEST(AppendableTests, FromNestedStruct)
{
  amalgam_serializer_test<NestedStruct, AdditionalFieldNestedStruct>(
    xcdr2, AppendableXcdr2StructExpectedBE::expected);
}

// ---------- AdditionalFieldNestedStruct
struct AdditionalNestedExpectedXcdr2BE {
  STREAM_DATA
};

const unsigned char AdditionalNestedExpectedXcdr2BE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x18, // +4 = 4
  // short_field
  0x7f, 0xff, // +2 = 6
  // long_field
  0x00, 0x00, // +2 pad = 8
  0x7f, 0xff, 0xff, 0xff, // +4 = 12
  // octet_field
  0x01, // +1 = 13
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 24
  // additional_field (long)
  0x12, 0x34, 0x56, 0x78 // +4 = 28
};
const unsigned AdditionalNestedExpectedXcdr2BE::layout[] = {4,2,2,4,1,3,8,4};

template<>
void set_values(AdditionalFieldNestedStruct& value)
{
  set_base_values(value);
  value.additional_field() = 0x12345678;
}

TEST(AppendableTests, FromAdditionalNestedStruct)
{
  amalgam_serializer_test<AdditionalFieldNestedStruct, NestedStruct>(
    xcdr2, AdditionalNestedExpectedXcdr2BE::expected);
}

TEST(AppendableTests, FromAdditionalNestedStructLE)
{
  test_little_endian<AdditionalFieldNestedStruct, NestedStruct,
                     AdditionalNestedExpectedXcdr2BE>();
}

template<>
void expect_values_equal(const AdditionalFieldNestedStruct& a,
                         const AdditionalFieldNestedStruct& b)
{
  expect_values_equal_base(a, b);
  EXPECT_EQ(a.additional_field(), b.additional_field());
}

TEST(AppendableTests, BothAdditionalNestedStruct)
{
  serializer_test<AdditionalFieldNestedStruct>(
    xcdr2, AdditionalNestedExpectedXcdr2BE::expected);
}

TEST(AppendableTests, BothAdditionalNestedStructLE)
{
  test_little_endian<AdditionalFieldNestedStruct, AdditionalNestedExpectedXcdr2BE>();
}

// ---------- AppendableStruct
struct AppendableExpectedXcdr2BE {
  STREAM_DATA
};

const unsigned char AppendableExpectedXcdr2BE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x2c, // +4 = 4
  // Delimiter of the nested struct
  0x00, 0x00, 0x00, 0x14, // +4 = 8
  // Inner short_field
  0x7f, 0xff, // +2 = 10
  // Inner long_field
  0x00, 0x00, // +2 pad = 12
  0x7f, 0xff, 0xff, 0xff, // +4 = 16
  // Inner octet_field
  0x01, // +1 = 17
  // Inner long_long_field
  0x00, 0x00, 0x00, // +3 = 20
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 28
  // short_field
  0x7f, 0xff, // +2 = 30
  // long_field
  0x00, 0x00, // +2 = 32
  0x7f, 0xff, 0xff, 0xff, // +4 = 36
  // octet_field
  0x01, // +1 = 37
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 40
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 48
};
const unsigned AppendableExpectedXcdr2BE::layout[] = {4,4,2,2,4,1,3,8,2,2,4,1,3,8};

template<>
void set_values(AppendableWithNestedStruct& value)
{
  set_base_values(value.nested());
  set_base_values(value);
}

template<typename TypeA, typename TypeB>
void expect_equal_with_nested(const TypeA& a, const TypeB& b)
{
  expect_values_equal_base(a, b);
  expect_values_equal_base(a.nested(), b.nested());
}

template<>
void expect_values_equal(const AppendableWithNestedStruct& a, const AppendableWithNestedStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(AppendableTests, BothAppendableStruct)
{
  serializer_test<AppendableWithNestedStruct>(xcdr2, AppendableExpectedXcdr2BE::expected);
}

TEST(AppendableTests, BothAppendableStructLE)
{
  test_little_endian<AppendableWithNestedStruct, AppendableExpectedXcdr2BE>();
}

template<>
void expect_values_equal(const AppendableWithNestedStruct& a,
                         const AdditionalFieldAppendableStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(AppendableTests, FromAppendableStruct)
{
  amalgam_serializer_test<AppendableWithNestedStruct, AdditionalFieldAppendableStruct>(
    xcdr2, AppendableExpectedXcdr2BE::expected);
}

TEST(AppendableTests, FromAppendableStructLE)
{
  test_little_endian<AppendableWithNestedStruct, AdditionalFieldAppendableStruct,
                     AppendableExpectedXcdr2BE>();
}

// ---------- AdditionalFieldAppendableStruct
struct AdditionalAppendableExpectedXcdr2BE {
  STREAM_DATA
};

const unsigned char AdditionalAppendableExpectedXcdr2BE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x34, // +4 = 4
  // Delimiter of the nested struct
  0x00, 0x00, 0x00, 0x18, // +4 = 8
  // Inner short_field
  0x7f, 0xff, // +2 = 10
  // Inner long_field
  0x00, 0x00, // +2 pad = 12
  0x7f, 0xff, 0xff, 0xff, // +4 = 16
  // Inner octet_field
  0x01, // +1 = 17
  // Inner long_long_field
  0x00, 0x00, 0x00, // +3 pad = 20
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 28
  // Inner additional_field
  0x12, 0x34, 0x56, 0x78, // +4 = 32
  // short_field
  0x7f, 0xff, // +2 = 34
  // long_field
  0x00, 0x00, // +2 pad = 36
  0x7f, 0xff, 0xff, 0xff, // +4 = 40
  // octet_field
  0x01, // +1 = 41
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 44
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 52
  // additional_field
  0x12, 0x34, 0x56, 0x78 // +4 = 56
};
const unsigned AdditionalAppendableExpectedXcdr2BE::layout[] = {4,4,2,2,4,1,3,8,
                                                                4,2,2,4,1,3,8,4};

template<>
void set_values(AdditionalFieldAppendableStruct& value)
{
  set_values(value.nested());
  set_base_values(value);
  value.additional_field() = 0x12345678;
}

template<>
void expect_values_equal(const AdditionalFieldAppendableStruct& a,
                         const AppendableWithNestedStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(AppendableTests, FromAdditionalAppendableStruct)
{
  amalgam_serializer_test<AdditionalFieldAppendableStruct, AppendableWithNestedStruct>(
    xcdr2, AdditionalAppendableExpectedXcdr2BE::expected);
}

TEST(AppendableTests, FromAdditionalAppendableStructLE)
{
  test_little_endian<AdditionalFieldAppendableStruct, AppendableWithNestedStruct,
                     AdditionalAppendableExpectedXcdr2BE>();
}

template<>
void expect_values_equal(const AdditionalFieldAppendableStruct& a,
                         const AdditionalFieldAppendableStruct& b)
{
  expect_equal_with_nested(a, b);
  EXPECT_EQ(a.additional_field(), b.additional_field());
  EXPECT_EQ(a.nested().additional_field(), b.nested().additional_field());
}

TEST(AppendableTests, BothAdditionalAppendableStruct)
{
  serializer_test<AdditionalFieldAppendableStruct>(
    xcdr2, AdditionalAppendableExpectedXcdr2BE::expected);
}

TEST(AppendableTests, BothAdditionalAppendableStructLE)
{
  test_little_endian<AdditionalFieldAppendableStruct,
                     AdditionalAppendableExpectedXcdr2BE>();
}

// ---------- AppendableWithNestedStruct2
struct AppendableExpected2Xcdr2BE {
  STREAM_DATA
};

const unsigned char AppendableExpected2Xcdr2BE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x28, // +4 = 4
  // string_field
  0x00, 0x00, 0x00, 0x0a, // + 4 = 8
  0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x00,  // +10 = 18
  // Delimiter of the nested struct
  0x00, 0x00, // +2 pad = 20
  0x00, 0x00, 0x00, 0x14, // +4 = 24
  // Inner short_field
  0x7f, 0xff, // +2 = 26
  // Inner long_field
  0x00, 0x00, // +2 pad = 28
  0x7f, 0xff, 0xff, 0xff, // +4 = 32
  // Inner octet_field
  0x01, // +1 = 33
  // Inner long_long_field
  0x00, 0x00, 0x00, // +3 pad = 36
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff // +8 = 44
};
const unsigned AppendableExpected2Xcdr2BE::layout[] = {4,4,1,1,1,1,1,1,
                                                       1,1,1,1,2,4,2,2,4,1,3,8};

template<>
void set_values(AppendableWithNestedStruct2& value)
{
  set_values(value.nested());
  value.string_field() = "abcdefghi";
}

template<>
void expect_values_equal(const AppendableWithNestedStruct2& a,
                         const AppendableWithNestedStruct2& b)
{
  expect_values_equal(a.nested(), b.nested());
  EXPECT_EQ(a.string_field(), b.string_field());
}

TEST(AppendableTests, BothAppendableWithNestedStruct2)
{
  serializer_test<AppendableWithNestedStruct2>(xcdr2, AppendableExpected2Xcdr2BE::expected);
}

TEST(AppendableTests, BothAppendableWithNestedStruct2LE)
{
  test_little_endian<AppendableWithNestedStruct2, AppendableExpected2Xcdr2BE>();
}

// ---------- AppendableUnion
struct AppendableUnionXcdr2ExpectedShortBE {
  STREAM_DATA
};

const unsigned char AppendableUnionXcdr2ExpectedShortBE::expected[] = {
  0x00, 0x00, 0x00, 0x06, // +4 Dheader = 4
  0x00, 0x00, 0x00, 0x00, // +4 discriminator = 8
  0x7f, 0xff // +2 short_field = 10
};
const unsigned AppendableUnionXcdr2ExpectedShortBE::layout[] = {4,4,2};

struct AppendableUnionXcdr2ExpectedLongBE {
  STREAM_DATA
};

const unsigned char AppendableUnionXcdr2ExpectedLongBE::expected[] = {
  0x00, 0x00, 0x00, 0x08, // +4 Dheader = 4
  0x00, 0x00, 0x00, 0x01, // +4 discriminator = 8
  0x7f, 0xff, 0xff, 0xff  // +4 long_field = 12
};
const unsigned AppendableUnionXcdr2ExpectedLongBE::layout[] = {4,4,4};

struct AppendableUnionXcdr2ExpectedOctetBE {
  STREAM_DATA
};

const unsigned char AppendableUnionXcdr2ExpectedOctetBE::expected[] = {
  0x00, 0x00, 0x00, 0x05, // +4 Dheader = 4
  0x00, 0x00, 0x00, 0x02, // +4 discriminator = 8
  0x01 // +1 octet_field = 9
};
const unsigned AppendableUnionXcdr2ExpectedOctetBE::layout[] = {4,4,1};

struct AppendableUnionXcdr2ExpectedLongLongBE {
  STREAM_DATA
};

const unsigned char AppendableUnionXcdr2ExpectedLongLongBE::expected[] = {
  0x00, 0x00, 0x00, 0x0c, // +4 Dheader = 4
  0x00, 0x00, 0x00, 0x03, // +4 discriminator = 8
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff // +8 long_long_field = 9
};
const unsigned AppendableUnionXcdr2ExpectedLongLongBE::layout[] = {4,4,8};

template<>
void expect_values_equal(const AppendableUnion& a, const ModifiedAppendableUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(AppendableTests, FromAppendableUnion)
{
  amalgam_serializer_test_union<AppendableUnion, ModifiedAppendableUnion>(
    xcdr2, AppendableUnionXcdr2ExpectedShortBE::expected, UnionDisc::E_SHORT_FIELD);
  amalgam_serializer_test_union<AppendableUnion, ModifiedAppendableUnion>(
    xcdr2, AppendableUnionXcdr2ExpectedLongBE::expected, UnionDisc::E_LONG_FIELD);
  amalgam_serializer_test_union<AppendableUnion, ModifiedAppendableUnion>(
    xcdr2, AppendableUnionXcdr2ExpectedOctetBE::expected, UnionDisc::E_OCTET_FIELD);
  amalgam_serializer_test_union<AppendableUnion, ModifiedAppendableUnion>(
    xcdr2, AppendableUnionXcdr2ExpectedLongLongBE::expected, UnionDisc::E_LONG_LONG_FIELD);
}

TEST(AppendableTests, FromAppendableUnionLE)
{
  test_little_endian_union<AppendableUnion, ModifiedAppendableUnion,
                           AppendableUnionXcdr2ExpectedShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<AppendableUnion, ModifiedAppendableUnion,
                           AppendableUnionXcdr2ExpectedLongBE>(UnionDisc::E_LONG_FIELD);
  test_little_endian_union<AppendableUnion, ModifiedAppendableUnion,
                           AppendableUnionXcdr2ExpectedOctetBE>(UnionDisc::E_OCTET_FIELD);
  test_little_endian_union<AppendableUnion, ModifiedAppendableUnion,
                           AppendableUnionXcdr2ExpectedLongLongBE>(UnionDisc::E_LONG_LONG_FIELD);
}

// Mutable Tests =============================================================

// ---------- MutableStruct
const unsigned char mutable_struct_expected_xcdr1[] = {
  // short_field
  0xc0, 0x04, 0x00, 0x02, // PID +4 = 4
  0x7f, 0xff, // +2 = 6
  // long_field
  0x00, 0x00, // +2 pad = 8
  0xc0, 0x06, 0x00, 0x04, // PID +4 = 12
  0x7f, 0xff, 0xff, 0xff, // +4 = 16
  // octet_field
  0xc0, 0x08, 0x00, 0x01, // PID +4 = 20
  0x01, // +1 = 21
  // long_long_field
  0x00, 0x00, 0x00, // +3 pad = 24
  0xc0, 0x0a, 0x00, 0x08, // PID +4 = 28
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 36
  // End of List
  0x3f, 0x02, 0x00, 0x00 // +4 = 40
};

struct MutableStructExpectedXcdr2BE {
  STREAM_DATA
};

const unsigned char MutableStructExpectedXcdr2BE::expected[] = {
  0x00,0,0,0x24, // +4=4 Delimiter
  0x10,0,0,0x04, 0x7f,0xff,  (0),(0), // +4+2+(2)=12 short_field
  0x20,0,0,0x06, 0x7f,0xff,0xff,0xff, // +4+4    =20 long_field
  0x00,0,0,0x08, 0x01,   (0),(0),(0), // +4+1+(3)=28 octet_field
  0x30,0,0,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +4+8=40 long_long_field
};

const unsigned MutableStructExpectedXcdr2BE::layout[] = {4,4,2,2,4,4,4,1,3,4,8};

TEST(MutableTests, BaselineXcdr1Test)
{
  baseline_checks<MutableStructWithExplicitIDs>(xcdr1, mutable_struct_expected_xcdr1);
}

TEST(MutableTests, BaselineXcdr2Test)
{
  baseline_checks<MutableStructWithExplicitIDs>(xcdr2, MutableStructExpectedXcdr2BE::expected);
}

TEST(MutableTests, BaselineXcdr2TestLE)
{
  test_little_endian<MutableStructWithExplicitIDs, MutableStructExpectedXcdr2BE>();
}

// ---------- MutableUnion
struct MutableUnionExpectedXcdr2ShortBE {
  STREAM_DATA
};

const unsigned char MutableUnionExpectedXcdr2ShortBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0e, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x00, // +4 value = 12
  // short_field
  0x10, 0x00, 0x00, 0x04, // +4 EMHEADER1 = 16
  0x7f, 0xff // +2 value = 18
};
const unsigned MutableUnionExpectedXcdr2ShortBE::layout[] = {4,4,4,4,2};

struct MutableUnionExpectedXcdr2LongBE {
  STREAM_DATA
};

const unsigned char MutableUnionExpectedXcdr2LongBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x10, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x01, // +4 value = 12
  // long_field
  0x20, 0x00, 0x00, 0x06,// +4 EMHEADER1 = 16
  0x7f, 0xff, 0xff, 0xff // +4 value = 20
};
const unsigned MutableUnionExpectedXcdr2LongBE::layout[] = {4,4,4,4,4};

struct MutableUnionExpectedXcdr2AdditionalBE {
  STREAM_DATA
};

const unsigned char MutableUnionExpectedXcdr2AdditionalBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x08, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x04, // +4 value = 12
};
const unsigned MutableUnionExpectedXcdr2AdditionalBE::layout[] = {4,4,4};

struct MutableUnionExpectedXcdr2OctetBE {
  STREAM_DATA
};

const unsigned char MutableUnionExpectedXcdr2OctetBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0d, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x02, // +4 value = 12
  // octet_field
  0x00, 0x00, 0x00, 0x08, // +4 EMHEADER1 = 16
  0x01                    // +1 value = 17
};
const unsigned MutableUnionExpectedXcdr2OctetBE::layout[] = {4,4,4,4,1};

struct MutableUnionExpectedXcdr2LongLongBE {
  STREAM_DATA
};

const unsigned char MutableUnionExpectedXcdr2LongLongBE::expected[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x14, // +4 = 4
  // Discriminator
  0x20, 0x00, 0x00, 0x00, // +4 EMHEADER1 = 8
  0x00, 0x00, 0x00, 0x03, // +4 value = 12
  // long_field
  0x30, 0x00, 0x00, 0x0a, // +4 EMHEADER1 = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  // +8 value = 24
};
const unsigned MutableUnionExpectedXcdr2LongLongBE::layout[] = {4,4,4,4,8};

template<>
void expect_values_equal(const MutableUnionWithExplicitIDs& a, const MutableUnionWithExplicitIDs& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(MutableTests, BaselineXcdr2TestUnion)
{
  baseline_checks_union<MutableUnionWithExplicitIDs>(xcdr2, MutableUnionExpectedXcdr2ShortBE::expected, UnionDisc::E_SHORT_FIELD);
  baseline_checks_union<MutableUnionWithExplicitIDs>(xcdr2, MutableUnionExpectedXcdr2LongBE::expected, UnionDisc::E_LONG_FIELD);
  baseline_checks_union<MutableUnionWithExplicitIDs>(xcdr2, MutableUnionExpectedXcdr2OctetBE::expected, UnionDisc::E_OCTET_FIELD);
  baseline_checks_union<MutableUnionWithExplicitIDs>(xcdr2, MutableUnionExpectedXcdr2LongLongBE::expected, UnionDisc::E_LONG_LONG_FIELD);
}

TEST(MutableTests, BaselineXcdr2TestUnionLE)
{
  test_little_endian_union<MutableUnionWithExplicitIDs, MutableUnionExpectedXcdr2ShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<MutableUnionWithExplicitIDs, MutableUnionExpectedXcdr2LongBE>(UnionDisc::E_LONG_FIELD);
  test_little_endian_union<MutableUnionWithExplicitIDs, MutableUnionExpectedXcdr2OctetBE>(UnionDisc::E_OCTET_FIELD);
  test_little_endian_union<MutableUnionWithExplicitIDs, MutableUnionExpectedXcdr2LongLongBE>(UnionDisc::E_LONG_LONG_FIELD);
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base_union2(const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a._d(), b._d());
  switch(a._d()) {
  case UnionDisc::E_SHORT_FIELD:
    EXPECT_EQ(a.short_field(), b.short_field());
    break;
  case UnionDisc::E_LONG_FIELD:
    EXPECT_EQ(a.long_field(), b.long_field());
    break;
  default:
    break;
  }
}

template<>
void expect_values_equal(const MutableUnionWithExplicitIDs& a, const ModifiedMutableUnion& b)
{
  expect_values_equal_base_union2(a, b);
}

TEST(MutableTests, FromMutableUnion)
{
  amalgam_serializer_test_union<MutableUnionWithExplicitIDs, ModifiedMutableUnion>(
    xcdr2, MutableUnionExpectedXcdr2ShortBE::expected, UnionDisc::E_SHORT_FIELD);
  amalgam_serializer_test_union<MutableUnionWithExplicitIDs, ModifiedMutableUnion>(
    xcdr2, MutableUnionExpectedXcdr2LongBE::expected, UnionDisc::E_LONG_FIELD);
  serializer_test_union<MutableUnionWithExplicitIDs>(
    xcdr2, MutableUnionExpectedXcdr2AdditionalBE::expected, UnionDisc::E_ADDITIONAL_FIELD);
}

TEST(MutableTests, FromMutableUnionLE)
{
  test_little_endian_union<MutableUnionWithExplicitIDs, ModifiedMutableUnion,
                           MutableUnionExpectedXcdr2ShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<MutableUnionWithExplicitIDs, ModifiedMutableUnion,
                           MutableUnionExpectedXcdr2LongBE>(UnionDisc::E_LONG_FIELD);
  test_little_endian_union<MutableUnionWithExplicitIDs,
                           MutableUnionExpectedXcdr2AdditionalBE>(UnionDisc::E_ADDITIONAL_FIELD);
}

// ---------- ModifiedMutableUnion
template<>
void set_values_union(ModifiedMutableUnion& value, UnionDisc disc)
{
  switch (disc) {
  case UnionDisc::E_SHORT_FIELD:
    value.short_field(0x7fff);
    break;
  case UnionDisc::E_LONG_FIELD:
    value.long_field(0x7fffffff);
    break;
  case UnionDisc::E_ADDITIONAL_FIELD:
    value.additional_field(0x7eeeeeee);
    break;
  default:
    break;
  }
}

template<>
void expect_values_equal(const ModifiedMutableUnion& a, const MutableUnion& b)
{
  expect_values_equal_base_union2(a, b);
}

TEST(MutableTests, FromModifiedMutableUnion)
{
  amalgam_serializer_test_union<ModifiedMutableUnion, MutableUnion>(
    xcdr2, MutableUnionExpectedXcdr2ShortBE::expected, UnionDisc::E_SHORT_FIELD);
  amalgam_serializer_test_union<ModifiedMutableUnion, MutableUnion>(
    xcdr2, MutableUnionExpectedXcdr2LongBE::expected, UnionDisc::E_LONG_FIELD);
  // TODO (sonndinh): test try-construct behavior when additional_field of
  // ModifiedMutableUnion is selected
}

TEST(MutableTests, FromModifiedMutableUnionLE)
{
  test_little_endian_union<ModifiedMutableUnion, MutableUnion,
                           MutableUnionExpectedXcdr2ShortBE>(UnionDisc::E_SHORT_FIELD);
  test_little_endian_union<ModifiedMutableUnion, MutableUnion,
                           MutableUnionExpectedXcdr2LongBE>(UnionDisc::E_LONG_FIELD);
  // TODO (sonndinh): similarly, test try-construct of additional_field here
}

// ---------- ReorderedMutableStruct
// Test compatibility between two structures with different field orders.

TEST(MutableTests, ToReorderedXcdr1Test)
{
  amalgam_serializer_test<MutableStructWithExplicitIDs, ReorderedMutableStruct>(
    xcdr1, mutable_struct_expected_xcdr1);
}

TEST(MutableTests, FromReorderedXcdr1Test)
{
  const unsigned char expected[] = {
    // long_field
    0xc0, 0x06, 0x00, 0x04, // PID +4 = 4
    0x7f, 0xff, 0xff, 0xff, // +4 = 8
    // long_long_field
    0xc0, 0x0a, 0x00, 0x08, // PID +4 = 12
    0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 20
    // octet_field
    0xc0, 0x08, 0x00, 0x01, // PID +4 = 24
    0x01, // +1 = 25
    // short_field
    0x00, 0x00, 0x00, // +3 pad = 28
    0xc0, 0x04, 0x00, 0x02, // PID +4 = 32
    0x7f, 0xff, // +2 = 34
    // End of List
    0x00, 0x00, // +2 pad = 36
    0x3f, 0x02, 0x00, 0x00 // +4 = 40
  };
  amalgam_serializer_test<ReorderedMutableStruct, MutableStructWithExplicitIDs>(xcdr1, expected);
}

TEST(MutableTests, ToReorderedXcdr2Test)
{
  amalgam_serializer_test<MutableStructWithExplicitIDs, ReorderedMutableStruct>(
    xcdr2, MutableStructExpectedXcdr2BE::expected);
}

TEST(MutableTests, ToReorderedXcdr2TestLE)
{
  test_little_endian<MutableStructWithExplicitIDs, ReorderedMutableStruct,
                     MutableStructExpectedXcdr2BE>();
}

TEST(MutableTests, FromReorderedXcdr2Test)
{
  unsigned char expected[] = {
    0x00,0,0,0x22, // +4=4 Delimiter
    0x20,0,0,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0x30,0,0,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=24 long_long_field
    0x00,0,0,0x08, 0x01,   (0),(0),(0), // +4+1+(3)=32 octet_field
    0x10,0,0,0x04, 0x7f,0xff            // +4+2    =38 short_field
  };
  const unsigned layout[] = {4,4,4,4,8,4,1,3,4,2};

  amalgam_serializer_test<ReorderedMutableStruct, MutableStructWithExplicitIDs>(xcdr2, expected);

  // Little-endian test
  change_endianness(expected, layout, sizeof(layout)/sizeof(unsigned));
  amalgam_serializer_test<ReorderedMutableStruct, MutableStructWithExplicitIDs>(xcdr2_le, expected);
}

// ---------- AdditionalFieldMutableStruct
// Test compatibility between two structures with different fields

template <>
void set_values<AdditionalFieldMutableStruct>(
  AdditionalFieldMutableStruct& value)
{
  set_base_values(value);
  value.additional_field() = 0x12345678;
}

TEST(MutableTests, ToAdditionalFieldXcdr1Test)
{
  MutableStructWithExplicitIDs value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr1, mutable_struct_expected_xcdr1, value, result);
  /// TODO(iguessthidlldo): Test for correct try construct behavior (default
  /// value?) if we decide on that.
}

TEST(MutableTests, FromAdditionalFieldXcdr1Test)
{
  const unsigned char expected[] = {
    // long_field
    0xc0, 0x06, 0x00, 0x04, // PID +4 = 4
    0x7f, 0xff, 0xff, 0xff, // +4 = 8
    // additional_field
    0xc0, 0x01, 0x00, 0x04, // PID +4 = 12
    0x12, 0x34, 0x56, 0x78, // +4 = 16
    // long_long_field
    0xc0, 0x0a, 0x00, 0x08, // PID +4 = 20
    0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // +8 = 28
    // octet_field
    0xc0, 0x08, 0x00, 0x01, // PID +4 = 32
    0x01, // +1 = 33
    // short_field
    0x00, 0x00, 0x00, // +3 pad = 36
    0xc0, 0x04, 0x00, 0x02, // PID +4 = 40
    0x7f, 0xff, // +2 = 42
    // End of List
    0x00, 0x00, // +2 pad = 44
    0x3f, 0x02, 0x00, 0x00 // +4 = 48
  };
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStructWithExplicitIDs>(xcdr1, expected);
}

TEST(MutableTests, ToAdditionalFieldXcdr2Test)
{
  MutableStructWithExplicitIDs value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr2, MutableStructExpectedXcdr2BE::expected, value, result);
  /// TODO(iguessthidlldo): Test for correct try construct behavior (default
  /// value?) if we decide on that.

  test_little_endian<MutableStructWithExplicitIDs, AdditionalFieldMutableStruct,
                     MutableStructExpectedXcdr2BE>();
}

TEST(MutableTests, FromAdditionalFieldMustUnderstandTest)
{
  unsigned char additional_field_must_understand[] = {
    0x00,0x00,0x00,0x2a, // +4=4 Delimiter
    0x20,0x00,0x00,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0xa0,0x00,0x00,0x01, 0x12,0x34,0x56,0x78, // +4+4=20 additional_field @must_understand
    0x30,0x00,0x00,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=32 long_long_field
    0x00,0x00,0x00,0x08, 0x01, (0),(0),(0), // +4+1+(3)=40 octet_field
    0x10,0x00,0x00,0x04, 0x7f,0xff // +4+2=46 short_field
  };
  unsigned layout[] = {4,4,4,4,4,4,8,4,1,3,4,2};

  // Deserialization should fail for unknown must_understand field
  ACE_Message_Block buffer(1024);
  buffer.copy((const char*)additional_field_must_understand, sizeof(additional_field_must_understand));
  Serializer serializer(&buffer, xcdr2);
  MutableStructWithExplicitIDs result;
  EXPECT_FALSE(serializer >> result);

  // Deserialize and Compare C++ Values
  AdditionalFieldMutableStruct expected;
  set_values(expected);
  deserialize_compare(xcdr2, additional_field_must_understand, expected);

  // Little-endian test
  change_endianness(additional_field_must_understand,
                    layout, sizeof(layout)/sizeof(unsigned));
  deserialize_compare(xcdr2_le, additional_field_must_understand, expected);
}

TEST(MutableTests, FromAdditionalFieldXcdr2Test)
{
  unsigned char expected[] = {
    0x00,0x00,0x00,0x2a, // +4=4 Delimiter
    0x20,0x00,0x00,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0x20,0x00,0x00,0x01, 0x12,0x34,0x56,0x78, // +4+4=20 additional_field
    0x30,0x00,0x00,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=32 long_long_field
    0x00,0x00,0x00,0x08, 0x01, (0),(0),(0), // +4+1+(3)=40 octet_field
    0x10,0x00,0x00,0x04, 0x7f,0xff // +4+2=46 short_field
  };
  unsigned layout[] = {4,4,4,4,4,4,8,4,1,3,4,2};

  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStructWithExplicitIDs>(xcdr2, expected);
  change_endianness(expected, layout, sizeof(layout)/sizeof(unsigned));
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStructWithExplicitIDs>(xcdr2_le, expected);
}

// ---------- LengthCodeStruct
template<>
void expect_values_equal(const LengthCodeStruct& a, const LengthCodeStruct& b)
{
  EXPECT_EQ(a.o(), b.o());
  EXPECT_EQ(a.s(), b.s());
  EXPECT_EQ(a.l(), b.l());
  EXPECT_EQ(a.ll(), b.ll());

  EXPECT_EQ(a.b3().a(), b.b3().a());
  EXPECT_EQ(a.b3().b(), b.b3().b());
  EXPECT_EQ(a.b3().c(), b.b3().c());

  EXPECT_EQ(a.o5().a(), b.o5().a());
  EXPECT_EQ(a.o5().b(), b.o5().b());
  EXPECT_EQ(a.o5().c(), b.o5().c());
  EXPECT_EQ(a.o5().d(), b.o5().d());
  EXPECT_EQ(a.o5().e(), b.o5().e());

  EXPECT_EQ(a.s3().x(), b.s3().x());
  EXPECT_EQ(a.s3().y(), b.s3().y());
  EXPECT_EQ(a.s3().z(), b.s3().z());

  EXPECT_EQ(a.t7().s3().x(), b.t7().s3().x());
  EXPECT_EQ(a.t7().s3().y(), b.t7().s3().y());
  EXPECT_EQ(a.t7().s3().z(), b.t7().s3().z());
  EXPECT_EQ(a.t7().o(), b.t7().o());

  EXPECT_EQ(a.l3().a(), b.l3().a());
  EXPECT_EQ(a.l3().b(), b.l3().b());
  EXPECT_EQ(a.l3().c(), b.l3().c());

  EXPECT_EQ(a.str1(), b.str1());
  EXPECT_EQ(a.str2(), b.str2());
  EXPECT_EQ(a.str3(), b.str3());
  EXPECT_EQ(a.str4(), b.str4());
  EXPECT_EQ(a.str5(), b.str5());
}

TEST(MutableTests, LengthCodeTest)
{
  unsigned char expected[] = {
    0x00,0x00,0x00,0xc1, // +4 = 4  Delimiter
  //(MU<<31)+(LC<<28)+id    NEXTINT    Value and Pad(0) // Size (Type)
  //--------------------  -----------  -------------------------------------------
    0x80,0x00,0x00,0x00,               1,  (0),(0),(0), // 1 (octet)     +4+1+3=12 @key
    0x10,0x00,0x00,0x01,               1,2,    (0),(0), // 2 (short)     +4+2+2=20
    0x20,0x00,0x00,0x02,               1,2,3,4,         // 4 (long)      +4+4=28
    0x30,0x00,0x00,0x03,               1,2,3,4,5,6,7,8, // 8 (long long) +4+8=40

    0x40,0x00,0x00,0x04,  0,0,0,0x03,  1,0,1,              (0), // b3 +8+3+1=52
    0x40,0x00,0x00,0x05,  0,0,0,0x05,  1,2,3,4,5,  (0),(0),(0), // o5 +8+5+3=68
    0xc0,0x00,0x00,0x06,  0,0,0,0x06,  0,1,0,2,0,3,    (0),(0), // s3 +8+6+2=84 @key
    0x40,0x00,0x00,0x07,  0,0,0,0x07,  0,1,0,2,0,3,4,      (0), // t7 +8+7+1=100
    0x40,0x00,0x00,0x08,  0,0,0,0x0c,  0,0,0,1,0,0,0,2,0,0,0,3, // l3 +8+12=120

    0x40,0x00,0x00,0x0b,  0,0,0,0x05,  0,0,0,0x01, '\0',(0),(0),(0),    // str1 +8+4+1+3=136
    0x40,0x00,0x00,0x0c,  0,0,0,0x06,  0,0,0,0x02, 'a','\0',(0),(0),    // str2 +8+4+2+2=152
    0x40,0x00,0x00,0x0d,  0,0,0,0x07,  0,0,0,0x03, 'a','b','\0',(0),    // str3 +8+4+3+1=168
    0x30,0x00,0x00,0x0e,               0,0,0,0x04, 'a','b','c','\0',    // str4 +4+4+4=180
    0xc0,0x00,0x00,0x0f,  0,0,0,0x09,  0,0,0,0x05, 'a','b','c','d','\0' // str5 +8+4+5=197 @key
  };

  const unsigned layout[] = {4,4,1,3,4,2,2,4,4,4,8,4,4,1,1,1,1,4,4,1,1,1,1,
                             1,3,4,4,2,2,2,2,4,4,2,2,2,1,1,4,4,4,4,4,4,4,4,
                             1,3,4,4,4,1,1,2,4,4,4,1,1,1,1,4,4,1,1,1,1,4,4,
                             4,1,1,1,1,1};

  LengthCodeStruct value = { //LC Size
    0x01,                    // 0    1
    0x0102,                  // 1    2
    0x01020304,              // 2    4
    0x0102030405060708,      // 3    8
    {true,false,true},       // 4    3
    {1,2,3,4,5},             // 4    5
    {1,2,3},                 // 4    6
    {{1,2,3},4},             // 4    7
    {1,2,3},                 // 4   12
    "",                      // 4  4+1
    "a",                     // 4  4+2
    "ab",                    // 4  4+3
    "abc",                   // 3  4+4
    "abcd"                   // 4  4+5
  };
  EXPECT_EQ(serialized_size(xcdr2, value), sizeof(expected));
  LengthCodeStruct result;
  amalgam_serializer_test<LengthCodeStruct, LengthCodeStruct>(xcdr2, expected, value, result);

  // Little-endian test
  change_endianness(expected, layout, sizeof(layout)/sizeof(unsigned));
  LengthCodeStruct result_le;
  amalgam_serializer_test<LengthCodeStruct, LengthCodeStruct>(xcdr2_le, expected, value, result_le);
}

// ---------- LC567Struct
template<>
void expect_values_equal(const LC567Struct& a, const LC567Struct& b)
{
  EXPECT_EQ(a.o3().size(), b.o3().size());
  EXPECT_EQ(a.o3()[0], b.o3()[0]);
  EXPECT_EQ(a.o3()[1], b.o3()[1]);
  EXPECT_EQ(a.o3()[2], b.o3()[2]);

  EXPECT_EQ(a.l3().size(), b.l3().size());
  EXPECT_EQ(a.l3()[0], b.l3()[0]);
  EXPECT_EQ(a.l3()[1], b.l3()[1]);
  EXPECT_EQ(a.l3()[2], b.l3()[2]);

  EXPECT_EQ(a.ll3().size(), b.ll3().size());
  EXPECT_EQ(a.ll3()[0], b.ll3()[0]);
  EXPECT_EQ(a.ll3()[1], b.ll3()[1]);
  EXPECT_EQ(a.ll3()[2], b.ll3()[2]);

  EXPECT_EQ(a.s3().size(), b.s3().size());
  EXPECT_EQ(a.s3()[0], b.s3()[0]);
  EXPECT_EQ(a.s3()[1], b.s3()[1]);

  EXPECT_EQ(a.str4(), b.str4());
  EXPECT_EQ(a.str5(), b.str5());

  EXPECT_EQ(a.ls().size(), b.ls().size());
  EXPECT_EQ(a.ls()[0], b.ls()[0]);
  EXPECT_EQ(a.ls()[1], b.ls()[1]);
}

TEST(MutableTests, ReadLc567Test)
{
  unsigned char data[] = {
    0,0,0,0x87, // Delimiter +4=4
    //MU,LC,id   NEXTINT   Value and Pad(0)
    0x50,0,0,0,  0,0,0,3,  1,2,3,(0), // o3 +4+4+3+(1)=16
    0x60,0,0,1,  0,0,0,3,  0,0,0,1, 0,0,0,2, 0,0,0,3, // l3 +4+4+4x3=36
    0x70,0,0,2,  0,0,0,3,  0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,2, 0,0,0,0,0,0,0,3, // ll3 +4+4+8x3=68
    0x30,0,0,3,            0,0,0,2,  0,1, 0,2, // s3 +4+4+4=80 (bound=3; only 2 elements)
    0x30,0,0,4,            0,0,0,4, 'a','b','c','\0', // str4 +4+4+4=92
    0x50,0,0,5,  0,0,0,5, 'a','b','c','d','\0',(0),(0),(0), // str5 +4+4+5+(3)=108
    0x60,0,0,6,  0,0,0,2,  0,0,0,1, 0,0,0,2, // ls +4+4+4x2=124
    0xd0,0,0,7,  0,0,0,7, 'a','b','c','d','e','f','\0' // str7 +4+4+7=139 @key
  };

  const unsigned layout[] = {4,4,4,1,1,1,1,4,4,4,4,4,4,4,8,8,
                             8,4,4,2,2,4,4,1,1,1,1,4,4,1,1,1,
                             1,1,1,1,1,4,4,4,4,4,4,1,1,1,1,1,1,1};

  LC567Struct expected;
  expected.o3().resize(3); expected.l3().resize(3); expected.ll3().resize(3);
  expected.o3()[0] = 1;    expected.l3()[0] = 1;    expected.ll3()[0] = 1;
  expected.o3()[1] = 2;    expected.l3()[1] = 2;    expected.ll3()[1] = 2;
  expected.o3()[2] = 3;    expected.l3()[2] = 3;    expected.ll3()[2] = 3;
  expected.s3().resize(2); expected.ls().resize(2);
  expected.s3()[0] = 1;    expected.ls()[0] = 1;
  expected.s3()[1] = 2;    expected.ls()[1] = 2;
  expected.str4() = "abc";
  expected.str5() = "abcd";
  expected.str7() = "abcdef";

  deserialize_compare(xcdr2, data, expected);

  // Little-endian test
  change_endianness(data, layout, sizeof(layout)/sizeof(unsigned));
  deserialize_compare(xcdr2_le, data, expected);
}

// ---------- MixedMutableStruct
template<>
void set_values(MixedMutableStruct& value)
{
  set_values(value.struct_nested());
  value.sequence_field().resize(3);
  value.sequence_field()[0] = value.sequence_field()[1] = value.sequence_field()[2] = 0x7fff;
  value.union_nested().string_field("abcdefghi");
  // Set discriminator after so it doesn't get overwritten
  value.union_nested()._d(3);
  value.sequence_field2().resize(3);
  value.sequence_field2()[0] = "my string1";
  value.sequence_field2()[1] = "my string2";
  value.sequence_field2()[2] = "my string3";
}

template<>
void expect_values_equal(const MixedMutableStruct& a,
                         const MixedMutableStruct& b)
{
  expect_values_equal(a.struct_nested(), b.struct_nested());
  EXPECT_EQ(a.sequence_field().size(), b.sequence_field().size());
  for (unsigned i = 0; i < a.sequence_field().size(); ++i) {
    EXPECT_EQ(a.sequence_field()[i], b.sequence_field()[i]);
  }
  EXPECT_EQ(a.union_nested()._d(), b.union_nested()._d());
  EXPECT_EQ(a.union_nested().string_field(), b.union_nested().string_field());
  EXPECT_EQ(a.sequence_field2().size(), b.sequence_field2().size());
  for (unsigned i = 0; i < a.sequence_field2().size(); ++i) {
    EXPECT_EQ(a.sequence_field2()[i], b.sequence_field2()[i]);
  }
}

struct MixedMutableStructXcdr2BE {
  STREAM_DATA
};

const unsigned char MixedMutableStructXcdr2BE::expected[] = {
  0x00, 0x00, 0x00, 0xaf, // +4 DHEADER = 4
  //MU,LC,ID   NEXTINT
  0x40,0,0,1,  0,0,0,0x28, // +8 EMHEADER1 + NEXTINT of struct_nested = 12
  // <<<<<< Begin struct_nested
  0x00, 0x00, 0x00, 0x24, // +4 DHEADER of struct_nested = 16
  //MU,LC,ID   NEXTINT   Value and Pad(0)
  0x10,0,0,4,            0x7f,0xff,(0),(0),    // +8 short_field = 24
  0x20,0,0,6,            0x7f,0xff,0xff,0xff,  // +8 long_field = 32
  0x00,0,0,8,            0x01,(0),(0),(0),     // +8 octet_field = 40
  0x30,0,0,10,           0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +12 long_long_field = 52
  // End struct_nested >>>>>>
  0x40,0,0,2,  0,0,0,0xa, 0,0,0,3,0x7f,0xff,0x7f,0xff,0x7f,0xff,(0),(0), // +20 sequence_field = 72
  0x40,0,0,3,  0,0,0,0x22, // +8 EMHEADER1 + NEXTINT of union_nested = 80
  // <<<<<< Begin union_nested
  0x00, 0x00, 0x00, 0x1e, // +4 DHEADER of union_nested = 84
  0x10,0,0,0,            0x00,0x03,(0),(0),     // +8 discriminator = 92
  0x40,0,0,3,  0,0,0,14, 0,0,0,10,'a','b','c','d','e','f','g','h','i','\0',(0),(0),
  // +24 string_field = 116
  // End union_nested >>>>>>
  0x40,0,0,4,  0,0,0,0x37, // +8 EMHEADER1 + NEXTINT of sequence_field2 = 124
  0x00, 0x00, 0x00, 0x33, 0,0,0,3, // +8 DHEADER & length of sequence_field2 = 132
  0,0,0,11,'m','y',' ','s','t','r','i','n','g','1','\0',(0), // +16 1st elem of sequence_field2 = 148
  0,0,0,11,'m','y',' ','s','t','r','i','n','g','2','\0',(0), // +16 2nd elem of sequence_field2 = 164
  0,0,0,11,'m','y',' ','s','t','r','i','n','g','3','\0'  // +15 3rd elem of sequence_field2 = 179
};

const unsigned MixedMutableStructXcdr2BE::layout[] = {4,4,4,4,4,2,2,4,4,4,1,3,4,8,4,4,
                                                      4,2,2,2,2,4,4,4,4,2,2,4,4,4,1,1,
                                                      1,1,1,1,1,1,1,1,2,4,4,4,4,4,1,1,
                                                      1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,
                                                      1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1};

TEST(MutableTests, BothMixedMutableStruct)
{
  serializer_test<MixedMutableStruct>(xcdr2, MixedMutableStructXcdr2BE::expected);
}

TEST(MutableTests, BothMixedMutableStructLE)
{
  test_little_endian<MixedMutableStruct, MixedMutableStructXcdr2BE>();
}

template<>
void expect_values_equal(const MixedMutableStruct& a,
                         const ModifiedMixedMutableStruct& b)
{
  expect_values_equal_base(a.struct_nested(), b.struct_nested());
}

TEST(MutableTests, FromMixedMutableStruct)
{
  amalgam_serializer_test<MixedMutableStruct, ModifiedMixedMutableStruct>(
    xcdr2, MixedMutableStructXcdr2BE::expected);
}

TEST(MutableTests, FromMixedMutableStructLE)
{
  test_little_endian<MixedMutableStruct, ModifiedMixedMutableStruct,
                     MixedMutableStructXcdr2BE>();
}

// Mixed Extensibility Tests ================================================

// ---------- NestingFinalStruct
template<>
void set_values(NestingFinalStruct& value)
{
  value.string_field() = "make sense";
  set_values(value.appendable_nested());
  value.sequence_field().resize(3);
  value.sequence_field()[0] = value.sequence_field()[1] = value.sequence_field()[2] = 0x1234;
  set_values(value.mutable_nested());
}

template<>
void expect_values_equal(const NestingFinalStruct& a,
                         const NestingFinalStruct& b)
{
  EXPECT_EQ(a.string_field(), b.string_field());
  expect_values_equal_base(a.appendable_nested(), b.appendable_nested());
  EXPECT_EQ(a.sequence_field().size(), b.sequence_field().size());
  for (unsigned i = 0; i < a.sequence_field().size(); ++i) {
    EXPECT_EQ(a.sequence_field()[i], b.sequence_field()[i]);
  }
  expect_values_equal_base(a.mutable_nested(), b.mutable_nested());
}

struct NestingFinalStructXcdr2BE {
  STREAM_DATA
};

const unsigned char NestingFinalStructXcdr2BE::expected[] = {
  0,0,0,11,'m','a','k','e',' ','s','e','n','s','e','\0',(0), // +16 string_field = 16
  // <<<<<< Begin appendable_nested
  0x00, 0x00, 0x00, 0x14, // +4 DHEADER of appendable_nested = 20
  0x7f,0xff,(0),(0),    // +4 short_field = 24
  0x7f,0xff,0xff,0xff,  // +4 long_field = 28
  0x01,(0),(0),(0),     // +4 octet_field = 32
  0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8 long_long_field = 40
  // End appendable_nested >>>>>>
  0,0,0,3,0x12,0x34,0x12,0x34,0x12,0x34,(0),(0), // +12 sequence_field = 52
  // <<<<<< Begin mutable_nested
  0x00, 0x00, 0x00, 0x24, // +4 DHEADER of mutable_nested = 56
  //MU,LC,ID   NEXTINT   Value and Pad(0)
  0x10,0,0,0,            0x7f,0xff,(0),(0),    // +8 short_field = 64
  0x20,0,0,1,            0x7f,0xff,0xff,0xff,  // +8 long_field = 72
  0x00,0,0,2,            0x01,(0),(0),(0),     // +8 octet_field = 80
  0x30,0,0,3,            0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +12 long_long_field = 92
  // End mutable_nested >>>>>>
};

const unsigned NestingFinalStructXcdr2BE::layout[] = {4,1,1,1,1,1,1,1,1,1,1,1,1,
                                                      4,2,2,4,1,3,8,4,2,2,2,2,4,
                                                      4,2,2,4,4,4,1,3,4,8};

TEST(MixedExtenTests, NestingFinalStruct)
{
  serializer_test<NestingFinalStruct>(xcdr2, NestingFinalStructXcdr2BE::expected);
}

TEST(MixedExtenTests, NestingFinalStructLE)
{
  test_little_endian<NestingFinalStruct, NestingFinalStructXcdr2BE>();
}

// ---------- NestingAppendableStruct
template<>
void set_values(NestingAppendableStruct& value)
{
  value.string_field() = "hello world";
  set_values(value.mutable_nested());
  value.sequence_field().resize(3);
  for (unsigned i = 0; i < value.sequence_field().size(); ++i) {
    value.sequence_field()[i] = 0x7fffffff;
  }
  set_values(value.final_nested());
}

template<>
void expect_values_equal(const NestingAppendableStruct& a,
                         const NestingAppendableStruct& b)
{
  EXPECT_EQ(a.string_field(), b.string_field());
  expect_values_equal_base(a.mutable_nested(), b.mutable_nested());
  EXPECT_EQ(a.sequence_field().size(), b.sequence_field().size());
  for (unsigned i = 0; i < a.sequence_field().size(); ++i) {
    EXPECT_EQ(a.sequence_field()[i], b.sequence_field()[i]);
  }
  expect_values_equal_base(a.final_nested(), b.final_nested());
}

struct NestingAppendableStructXcdr2BE {
  STREAM_DATA
};

const unsigned char NestingAppendableStructXcdr2BE::expected[] = {
  0x00, 0x00, 0x00, 0x5c, // +4 DHEADER = 4
  0,0,0,12,'h','e','l','l','o',' ','w','o','r','l','d','\0', // +16 string_field = 20
  // <<<<<< Begin mutable_nested
  0x00, 0x00, 0x00, 0x24, // +4 DHEADER of mutable_nested = 24
  //MU,LC,ID   NEXTINT   Value and Pad(0)
  0x10,0,0,0,            0x7f,0xff,(0),(0),    // +8 short_field = 32
  0x20,0,0,1,            0x7f,0xff,0xff,0xff,  // +8 long_field = 40
  0x00,0,0,2,            0x01,(0),(0),(0),     // +8 octet_field = 48
  0x30,0,0,3,            0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +12 long_long_field = 60
  // End mutable_nested >>>>>>
  0,0,0,3,0x7f,0xff,0xff,0xff,0x7f,0xff,0xff,0xff,0x7f,0xff,0xff,0xff, // +16 sequence_field = 76
  // <<<<<< Begin final_nested
  0x7f,0xff,(0),(0),    // +4 short_field = 80
  0x7f,0xff,0xff,0xff,  // +4 long_field = 84
  0x01,(0),(0),(0),     // +4 octet_field = 88
  0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +8 long_long_field = 96
  // End final_nested >>>>>>
};

const unsigned NestingAppendableStructXcdr2BE::layout[] = {4,4,1,1,1,1,1,1,1,1,1,1,1,1,
                                                           4,4,2,2,4,4,4,1,3,4,8,4,4,4,
                                                           4,2,2,4,1,3,8};

TEST(MixedExtenTests, NestingAppendableStruct)
{
  serializer_test<NestingAppendableStruct>(xcdr2, NestingAppendableStructXcdr2BE::expected);
}

TEST(MixedExtenTests, NestingAppendableStructLE)
{
  test_little_endian<NestingAppendableStruct, NestingAppendableStructXcdr2BE>();
}

// ---------- NestingMutableStruct
template<>
void set_values(NestingMutableStruct& value)
{
  value.string_field() = "hello world";
  set_values(value.appendable_nested());
  value.sequence_field().resize(7);
  for (unsigned i = 0; i < value.sequence_field().size(); ++i) {
    value.sequence_field()[i] = 0x7f;
  }
  set_values(value.final_nested());
}

template<>
void expect_values_equal(const NestingMutableStruct& a,
                         const NestingMutableStruct& b)
{
  EXPECT_EQ(a.string_field(), b.string_field());
  expect_values_equal_base(a.appendable_nested(), b.appendable_nested());
  EXPECT_EQ(a.sequence_field().size(), b.sequence_field().size());
  for (unsigned i = 0; i < a.sequence_field().size(); ++i) {
    EXPECT_EQ(a.sequence_field()[i], b.sequence_field()[i]);
  }
  expect_values_equal_base(a.final_nested(), b.final_nested());
}

struct NestingMutableStructXcdr2BE {
  STREAM_DATA
};

const unsigned char NestingMutableStructXcdr2BE::expected[] = {
  0x00, 0x00, 0x00, 0x68, // +4 DHEADER = 4
  //MU,LC,ID   NEXTINT   Value and pad(0)
  0x40,0,0,0,  0,0,0,16, 0,0,0,12,'h','e','l','l','o',' ','w','o','r','l','d','\0', // +24 string_field = 28
  0x40,0,0,1,  0,0,0,0x18, // +8 EMHEADER1 + NEXTINT of appendable_nested = 36
  // <<<<<< Begin appendable_nested
  0x00, 0x00, 0x00, 0x14, // +4 DHEADER of appendable_nested = 40
  0x7f,0xff,(0),(0),    // +4 short_field = 44
  0x7f,0xff,0xff,0xff,  // +4 long_field = 48
  0x01,(0),(0),(0),     // +4 octet_field = 52
  0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8 long_long_field = 60
  // End appendable_nested >>>>>>
  0x40,0,0,2,  0,0,0,0x0b, 0,0,0,7,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,(0), // +20 sequence_field = 80
  0x40,0,0,3,  0,0,0,0x14, // +8 EMHEADER1 + NEXTINT of final_nested = 88
  // <<<<<< Begin final_nested
  0x7f,0xff,(0),(0),    // +4 short_field = 92
  0x7f,0xff,0xff,0xff,  // +4 long_field = 96
  0x01,(0),(0),(0),     // +4 octet_field = 100
  0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +8 long_long_field = 108
  // End final_nested >>>>>>
};

const unsigned NestingMutableStructXcdr2BE::layout[] = {4,4,4,4,1,1,1,1,1,1,1,1,1,1,1,1,
                                                        4,4,4,2,2,4,1,3,8,4,4,4,1,1,1,1,
                                                        1,1,1,1,4,4,2,2,4,1,3,8};

TEST(MixedExtenTests, NestingMutableStruct)
{
  serializer_test<NestingMutableStruct>(xcdr2, NestingMutableStructXcdr2BE::expected);
}

TEST(MixedExtenTests, NestingMutableStructLE)
{
  test_little_endian<NestingMutableStruct, NestingMutableStructXcdr2BE>();
}

// IdVsDeclOrder ==============================================================

template <>
void expect_values_equal<IdVsDeclOrder, IdVsDeclOrder>(
  const IdVsDeclOrder& a, const IdVsDeclOrder& b)
{
  EXPECT_EQ(a.first_id2(), b.first_id2());
  EXPECT_EQ(a.second_id1(), b.second_id1());
}

template <>
void set_values<IdVsDeclOrder>(IdVsDeclOrder& value)
{
  value.first_id2() = 0x12345678;
  value.second_id1() = 0x9abc;
}

const unsigned char id_vs_decl_order_expected[] = {
  // first_id2
  0x12, 0x34, 0x56, 0x78, // +4 = 4
  // second_id1
  0x9a, 0xbc, // +2 = 6
};

TEST(IdVsDeclOrder, test)
{
  serializer_test<IdVsDeclOrder>(xcdr2, id_vs_decl_order_expected);
}

// KeyOnly Serialization ======================================================

template <typename Type>
void build_expected(DataVec& /*expected*/, FieldFilter /*field_filter*/)
{
  ASSERT_TRUE(false);
}

template <typename Type>
void not_key_only_test()
{
  DataVec expected;
  build_expected<Type>(expected, FieldFilter_All);
  serializer_test<Type>(xcdr2, expected);
};

template <typename Type, typename Wrapper, typename ConstWrapper>
void key_only_test(bool topic_type)
{
  if (!topic_type && dynamic) {
    // Can't do nested key only with DynamicData
    return;
  }
  Type default_value;
  set_default(default_value);
  const FieldFilter field_filter = topic_type ? FieldFilter_KeyOnly : FieldFilter_NestedKeyOnly;
  DataVec expected;
  build_expected<Type>(expected, field_filter);
  Type value = default_value;
  set_values<Type, Wrapper>(value);
  ConstWrapper wrapped_value(value);
  Type result = default_value;
  Wrapper wrapped_result(result);
  amalgam_serializer_test_base<ConstWrapper, Type, Wrapper, Type>(
    xcdr2, expected, wrapped_value, wrapped_result, field_filter);
  EXPECT_PRED_FORMAT2(assert_values, wrapped_value, wrapped_result);

  // Test vwrite with KeyOnly or NestedKeyOnly samples
  Xcdr2ValueWriter value_writer(xcdr2);
  EXPECT_TRUE(vwrite(value_writer, wrapped_value));

  ACE_Message_Block buffer(value_writer.get_serialized_size());
  Serializer ser(&buffer, xcdr2);
  value_writer.set_serializer(&ser);
  EXPECT_TRUE(vwrite(value_writer, wrapped_value));
  EXPECT_PRED_FORMAT2(assert_DataView, expected, buffer);
}

void serialize_u32(DataVec& data_vec, size_t value)
{
  unsigned char bytes[4];
  if (ENDIAN_BIG == ENDIAN_NATIVE) {
    bytes[0] = static_cast<unsigned char>(value);
    bytes[1] = static_cast<unsigned char>(value >>= 8);
    bytes[2] = static_cast<unsigned char>(value >>= 8);
    bytes[3] = static_cast<unsigned char>(value >> 8);
  } else {
    bytes[3] = static_cast<unsigned char>(value);
    bytes[2] = static_cast<unsigned char>(value >>= 8);
    bytes[1] = static_cast<unsigned char>(value >>= 8);
    bytes[0] = static_cast<unsigned char>(value >> 8);
  }
  DataView(bytes).copy_to(data_vec);
}

template<typename Type>
void key_only_set_base_values(Type& value,
  FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed =
    field_filter == FieldFilter_All || (!keyed && field_filter == FieldFilter_NestedKeyOnly);

  set_default(value);
  if (include_possible_keyed) {
    value.long_value() = 0x7fffffff;
    value.long_array_value()[0] = 1;
    value.long_array_value()[1] = 2;
    /* TODO(iguessthislldo): See IDL Def
    value.long_seq_value.length(1);
    value.long_seq_value[1] = 3;
    */
    value.string_value() = "STRINGY";
  }
  if (include_unkeyed) {
    value.extra_value() = 0x2020;
  }
}

template<>
void set_values(BasicUnkeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_All, false);
}

template<>
void set_values(BasicKeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_All, true);
}

template<>
void set_values<BasicUnkeyedStruct, NestedKeyOnly<BasicUnkeyedStruct> >(
  BasicUnkeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_NestedKeyOnly, false);
}

template<>
void set_values<BasicKeyedStruct, NestedKeyOnly<BasicKeyedStruct> >(BasicKeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_NestedKeyOnly, true);
}

template<>
void set_values<BasicUnkeyedStruct, KeyOnly<BasicUnkeyedStruct> >(BasicUnkeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_KeyOnly, false);
}

template<>
void set_values<BasicKeyedStruct, KeyOnly<BasicKeyedStruct> >(BasicKeyedStruct& value)
{
  key_only_set_base_values(value, FieldFilter_KeyOnly, true);
}

template<typename Type>
void key_only_union_set_base_values(Type& value,
  FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed = field_filter == FieldFilter_All;

  if (include_unkeyed) {
    value.default_value(0x74181);
  } else {
    value.default_value(0);
  }
  if (include_possible_keyed) {
    value._d(0x7400);
  }
}

template<>
void set_values(UnkeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_All, false);
}

template<>
void set_values(KeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_All, true);
}

template<>
void set_values<UnkeyedUnion, NestedKeyOnly<UnkeyedUnion> >(UnkeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_NestedKeyOnly, false);
}

template<>
void set_values<KeyedUnion, NestedKeyOnly<KeyedUnion> >(KeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_NestedKeyOnly, true);
}

template<>
void set_values<UnkeyedUnion, KeyOnly<UnkeyedUnion> >(UnkeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_KeyOnly, false);
}

template<>
void set_values<KeyedUnion, KeyOnly<KeyedUnion> >(KeyedUnion& value)
{
  key_only_union_set_base_values(value, FieldFilter_KeyOnly, true);
}

template<typename Type>
void key_only_complex_set_base_values(Type& value,
  FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed =
    field_filter == FieldFilter_All || (!keyed && field_filter == FieldFilter_NestedKeyOnly);
  const FieldFilter nested_field_filter =
    field_filter == FieldFilter_All ? FieldFilter_All : FieldFilter_NestedKeyOnly;

  set_default(value);

  if (include_possible_keyed) {
    key_only_set_base_values(
      value.unkeyed_struct_value(), nested_field_filter, false);
    key_only_set_base_values(
      value.unkeyed_struct_array_value()[0], nested_field_filter, false);
    key_only_set_base_values(
      value.unkeyed_struct_array_value()[1], nested_field_filter, false);
    /* TODO(iguessthislldo): See IDL Def
    value.unkeyed_struct_seq_value.length(1);
    key_only_set_base_values(
      value.unkeyed_struct_seq_value[0], nested_field_filter, false);
    */
    key_only_set_base_values(
      value.keyed_struct_value(), nested_field_filter, true);
    key_only_set_base_values(
      value.keyed_struct_array_value()[0], nested_field_filter, true);
    key_only_set_base_values(
      value.keyed_struct_array_value()[1], nested_field_filter, true);
    /* TODO(iguessthislldo): See IDL Def
    value.keyed_struct_seq_value.length(1);
    key_only_set_base_values(
      value.keyed_struct_seq_value[0], nested_field_filter, true);
    */

    key_only_union_set_base_values(
      value.unkeyed_union_value(), nested_field_filter, false);
    key_only_union_set_base_values(
      value.unkeyed_union_array_value()[0], nested_field_filter, false);
    key_only_union_set_base_values(
      value.unkeyed_union_array_value()[1], nested_field_filter, false);
    /* TODO(iguessthislldo): See IDL Def
    value.unkeyed_union_seq_value.length(1);
    key_only_union_set_base_values(
      value.unkeyed_union_seq_value[0], nested_field_filter, false);
    */

    key_only_union_set_base_values(
      value.keyed_union_value(), nested_field_filter, true);
    key_only_union_set_base_values(
      value.keyed_union_array_value()[0], nested_field_filter, true);
    key_only_union_set_base_values(
      value.keyed_union_array_value()[1], nested_field_filter, true);
    /* TODO(iguessthislldo): See IDL Def
    value.keyed_union_seq_value.length(1);
    key_only_union_set_base_values(
      value.keyed_union_seq_value[0], nested_field_filter, true);
    */
  }

  if (include_unkeyed) {
    value.extra_value() = 0x2020;
  }
}

template<>
void set_values(ComplexUnkeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_All, false);
}

template<>
void set_values(ComplexKeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_All, true);
}

template<>
void set_values<
  ComplexUnkeyedStruct, NestedKeyOnly<ComplexUnkeyedStruct> >(ComplexUnkeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_NestedKeyOnly, false);
}

template<>
void set_values<
  ComplexKeyedStruct, NestedKeyOnly<ComplexKeyedStruct> >(ComplexKeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_NestedKeyOnly, true);
}

template<>
void set_values<
  ComplexUnkeyedStruct, KeyOnly<ComplexUnkeyedStruct> >(ComplexUnkeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_KeyOnly, false);
}

template<>
void set_values<ComplexKeyedStruct, KeyOnly<ComplexKeyedStruct> >(ComplexKeyedStruct& value)
{
  key_only_complex_set_base_values(value, FieldFilter_KeyOnly, true);
}

template<typename TypeA, typename TypeB>
void key_only_tests_struct_expect_values_equal(
  const TypeA& a, const TypeB& b,
  FieldFilter field_filter, bool keyed)
{
  if ((field_filter != FieldFilter_KeyOnly) || keyed) {
    EXPECT_EQ(a.long_value(), b.long_value());
    expect_arrays_are_equal(a.long_array_value(), b.long_array_value());
    /* TODO(iguessthislldo): See IDL Def
    EXPECT_EQ(a.long_seq_value, b.long_seq_value);
    */
    EXPECT_EQ(a.string_value(), b.string_value());
  }
  if (field_filter == FieldFilter_All ||
      (!keyed && field_filter == FieldFilter_NestedKeyOnly)) {
    EXPECT_EQ(a.extra_value(), b.extra_value());
  }
}

template<>
void expect_values_equal(const BasicUnkeyedStruct& a, const BasicUnkeyedStruct& b)
{
  key_only_tests_struct_expect_values_equal(a, b, FieldFilter_All, false);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const BasicUnkeyedStruct>& a,
  const NestedKeyOnly<BasicUnkeyedStruct>& b)
{
  key_only_tests_struct_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, false);
}

template<>
void expect_values_equal(
  const KeyOnly<const BasicUnkeyedStruct>& a,
  const KeyOnly<BasicUnkeyedStruct>& b)
{
  key_only_tests_struct_expect_values_equal(
    a.value, b.value, FieldFilter_KeyOnly, false);
}

template<>
void expect_values_equal(const BasicKeyedStruct& a, const BasicKeyedStruct& b)
{
  key_only_tests_struct_expect_values_equal(a, b, FieldFilter_All, true);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const BasicKeyedStruct>& a,
  const NestedKeyOnly<BasicKeyedStruct>& b)
{
  key_only_tests_struct_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, true);
}

template<>
void expect_values_equal(
  const KeyOnly<const BasicKeyedStruct>& a,
  const KeyOnly<BasicKeyedStruct>& b)
{
  key_only_tests_struct_expect_values_equal(
    a.value, b.value, FieldFilter_KeyOnly, true);
}

template<typename TypeA, typename TypeB>
void key_only_tests_union_expect_values_equal(
  const TypeA& a, const TypeB& b,
  FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = field_filter == FieldFilter_All || keyed;
  const bool include_unkeyed = field_filter == FieldFilter_All;

  if (include_possible_keyed) {
    EXPECT_EQ(a._d(), b._d());
  }
  if (include_unkeyed) {
    EXPECT_EQ(a.default_value(), b.default_value());
  }
}

template<>
void expect_values_equal(const UnkeyedUnion& a, const UnkeyedUnion& b)
{
  key_only_tests_union_expect_values_equal(a, b, FieldFilter_All, false);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const UnkeyedUnion>& a,
  const NestedKeyOnly<UnkeyedUnion>& b)
{
  key_only_tests_union_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, false);
}

template<>
void expect_values_equal(
  const KeyOnly<const UnkeyedUnion>& a,
  const KeyOnly<UnkeyedUnion>& b)
{
  key_only_tests_union_expect_values_equal(a.value, b.value, FieldFilter_KeyOnly, false);
}

template<>
void expect_values_equal(const KeyedUnion& a, const KeyedUnion& b)
{
  key_only_tests_union_expect_values_equal(a, b, FieldFilter_All, true);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const KeyedUnion>& a,
  const NestedKeyOnly<KeyedUnion>& b)
{
  key_only_tests_union_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, true);
}

template<>
void expect_values_equal(
  const KeyOnly<const KeyedUnion>& a,
  const KeyOnly<KeyedUnion>& b)
{
  key_only_tests_union_expect_values_equal(a.value, b.value, FieldFilter_KeyOnly, true);
}

template<typename TypeA, typename TypeB>
void key_only_tests_complex_expect_values_equal(const TypeA& a, const TypeB& b,
  FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed =
    field_filter == FieldFilter_All || (!keyed && field_filter == FieldFilter_NestedKeyOnly);
  const FieldFilter child =
    field_filter == FieldFilter_All ? FieldFilter_All : FieldFilter_NestedKeyOnly;

  if (include_possible_keyed) {
    key_only_tests_struct_expect_values_equal(
      a.unkeyed_struct_value(), b.unkeyed_struct_value(), child, false);
    key_only_tests_struct_expect_values_equal(
      a.unkeyed_struct_array_value()[0], b.unkeyed_struct_array_value()[0], child, false);
    key_only_tests_struct_expect_values_equal(
      a.unkeyed_struct_array_value()[1], b.unkeyed_struct_array_value()[1], child, false);
    /* expect_values_equal(a.unkeyed_struct_seq_value, b.unkeyed_struct_seq_value, child); */
    key_only_tests_struct_expect_values_equal(
      a.keyed_struct_value(), b.keyed_struct_value(), child, true);
    key_only_tests_struct_expect_values_equal(
      a.keyed_struct_array_value()[0], b.keyed_struct_array_value()[0], child, true);
    key_only_tests_struct_expect_values_equal(
      a.keyed_struct_array_value()[1], b.keyed_struct_array_value()[1], child, true);
    /* expect_values_equal(a.keyed_struct_seq_value, b.keyed_struct_seq_value, child); */
    key_only_tests_union_expect_values_equal(
      a.keyed_union_value(), b.keyed_union_value(), child, true);
    key_only_tests_union_expect_values_equal(
      a.keyed_union_array_value()[0], b.keyed_union_array_value()[0], child, true);
    key_only_tests_union_expect_values_equal(
      a.keyed_union_array_value()[1], b.keyed_union_array_value()[1], child, true);
    /* expect_values_equal(a.keyed_union_seq_value, b.keyed_union_seq_value, child); */
  }
  if (include_unkeyed) {
    key_only_tests_union_expect_values_equal(
      a.unkeyed_union_value(), b.unkeyed_union_value(), child, false);
    key_only_tests_union_expect_values_equal(
      a.unkeyed_union_array_value()[0], b.unkeyed_union_array_value()[0], child, false);
    key_only_tests_union_expect_values_equal(
      a.unkeyed_union_array_value()[1], b.unkeyed_union_array_value()[1], child, false);
    /* expect_values_equal(a.unkeyed_union_seq_value, b.unkeyed_union_seq_value); */
    EXPECT_EQ(a.extra_value(), b.extra_value());
  }
}

template<>
void expect_values_equal(const ComplexUnkeyedStruct& a, const ComplexUnkeyedStruct& b)
{
  key_only_tests_complex_expect_values_equal(a, b, FieldFilter_All, false);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const ComplexUnkeyedStruct>& a,
  const NestedKeyOnly<ComplexUnkeyedStruct>& b)
{
  key_only_tests_complex_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, false);
}

template<>
void expect_values_equal(
  const KeyOnly<const ComplexUnkeyedStruct>& a,
  const KeyOnly<ComplexUnkeyedStruct>& b)
{
  key_only_tests_complex_expect_values_equal(
    a.value, b.value, FieldFilter_KeyOnly, false);
}

template<>
void expect_values_equal(const ComplexKeyedStruct& a, const ComplexKeyedStruct& b)
{
  key_only_tests_complex_expect_values_equal(a, b, FieldFilter_All, true);
}

template<>
void expect_values_equal(
  const NestedKeyOnly<const ComplexKeyedStruct>& a,
  const NestedKeyOnly<ComplexKeyedStruct>& b)
{
  key_only_tests_complex_expect_values_equal(
    a.value, b.value, FieldFilter_NestedKeyOnly, true);
}

template<>
void expect_values_equal(
  const KeyOnly<const ComplexKeyedStruct>& a,
  const KeyOnly<ComplexKeyedStruct>& b)
{
  key_only_tests_complex_expect_values_equal(
    a.value, b.value, FieldFilter_KeyOnly, true);
}

const unsigned char key_only_keys_expected_base[] = {
  // long_value
  0x7f, 0xff, 0xff, 0xff,
  // long_array_value
  0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x02,
  /* TODO(iguessthislldo): See IDL Def
  // long_array_value
  0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x03,
  */
  // string_value
  0x00, 0x00, 0x00, 0x08,
  'S', 'T', 'R', 'I', 'N', 'G', 'Y', '\0'
};

const unsigned char key_only_non_keys_expected_base[] = {
  // extra_value
  0x00, 0x00, 0x20, 0x20
};

void build_expected_basic_struct(
  DataVec& expected, FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed =
    field_filter == FieldFilter_All || (!keyed && field_filter == FieldFilter_NestedKeyOnly);

  DataView keys;
  if (include_possible_keyed) {
    keys = DataView(key_only_keys_expected_base);
  }
  DataView non_keys;
  if (include_unkeyed) {
    non_keys = DataView(key_only_non_keys_expected_base);
  }
  serialize_u32(expected, keys.size + non_keys.size);
  keys.copy_to(expected);
  non_keys.copy_to(expected);
}

template <>
void build_expected<BasicUnkeyedStruct>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_basic_struct(expected, field_filter, false);
}

template <>
void build_expected<BasicKeyedStruct>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_basic_struct(expected, field_filter, true);
}

const unsigned char key_only_union_keys_expected_base[] = {
  0x00, 0x00, 0x74, 0x00
};

const unsigned char key_only_union_non_keys_expected_base[] = {
  0x00, 0x07, 0x41, 0x81
};


void build_expected_union(DataVec& expected, FieldFilter field_filter, bool keyed)
{
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed = field_filter == FieldFilter_All;

  DataView keys;
  if (include_possible_keyed) {
    keys = DataView(key_only_union_keys_expected_base);
  }
  DataView non_keys;
  if (include_unkeyed) {
    non_keys = DataView(key_only_union_non_keys_expected_base);
  }
  serialize_u32(expected, keys.size + non_keys.size);
  keys.copy_to(expected);
  non_keys.copy_to(expected);
}

template <>
void build_expected<UnkeyedUnion>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_union(expected, field_filter, false);
}

template <>
void build_expected<KeyedUnion>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_union(expected, field_filter, true);
}

void build_expected_complex_struct(DataVec& expected, FieldFilter field_filter, bool keyed)
{
  DataVec all_contents;
  const bool include_possible_keyed = (field_filter != FieldFilter_KeyOnly) || keyed;
  const bool include_unkeyed =
    field_filter == FieldFilter_All || (!keyed && field_filter == FieldFilter_NestedKeyOnly);
  const FieldFilter nested_field_filter =
    field_filter == FieldFilter_All ? FieldFilter_All : FieldFilter_NestedKeyOnly;

  if (include_possible_keyed) {
    build_expected_basic_struct(all_contents, nested_field_filter, false);
    {
      DataVec array_contents;
      build_expected_basic_struct(array_contents, nested_field_filter, false);
      build_expected_basic_struct(array_contents, nested_field_filter, false);
      serialize_u32(all_contents, array_contents.size());
      DataView(array_contents).copy_to(all_contents);
    }
    // TODO(iguessthislldo): unkeyed_struct_seq_value would go here

    build_expected_basic_struct(all_contents, nested_field_filter, true);
    {
      DataVec array_contents;
      build_expected_basic_struct(array_contents, nested_field_filter, true);
      build_expected_basic_struct(array_contents, nested_field_filter, true);
      serialize_u32(all_contents, array_contents.size());
      DataView(array_contents).copy_to(all_contents);
    }
    // TODO(iguessthislldo): keyed_struct_seq_value would go here

    build_expected_union(all_contents, nested_field_filter, false);
    {
      DataVec array_contents;
      build_expected_union(array_contents, nested_field_filter, false);
      build_expected_union(array_contents, nested_field_filter, false);
      serialize_u32(all_contents, array_contents.size());
      DataView(array_contents).copy_to(all_contents);
    }
    // TODO(iguessthislldo): unkeyed_union_seq_value would go here

    build_expected_union(all_contents, nested_field_filter, true);
    {
      DataVec array_contents;
      build_expected_union(array_contents, nested_field_filter, true);
      build_expected_union(array_contents, nested_field_filter, true);
      serialize_u32(all_contents, array_contents.size());
      DataView(array_contents).copy_to(all_contents);
    }
    // TODO(iguessthislldo): keyed_union_seq_value would go here
  }

  if (include_unkeyed) {
    DataView(key_only_non_keys_expected_base).copy_to(all_contents);
  }

  serialize_u32(expected, all_contents.size());
  DataView(all_contents).copy_to(expected);
}

template <>
void build_expected<ComplexUnkeyedStruct>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_complex_struct(expected, field_filter, false);
}

template <>
void build_expected<ComplexKeyedStruct>(DataVec& expected, FieldFilter field_filter)
{
  build_expected_complex_struct(expected, field_filter, true);
}

TEST(KeyTests, normal_BasicUnkeyedStruct)
{
  not_key_only_test<BasicUnkeyedStruct>();
}

TEST(KeyTests, normal_BasicKeyedStruct)
{
  not_key_only_test<BasicKeyedStruct>();
}

TEST(KeyTests, normal_UnkeyedUnion)
{
  not_key_only_test<UnkeyedUnion>();
}

TEST(KeyTests, normal_KeyedUnion)
{
  not_key_only_test<KeyedUnion>();
}

TEST(KeyTests, normal_ComplexUnkeyedStruct)
{
  not_key_only_test<ComplexUnkeyedStruct>();
}

TEST(KeyTests, normal_ComplexKeyedStruct)
{
  not_key_only_test<ComplexKeyedStruct>();
}

TEST(KeyTests, NestedKeyOnly_BasicUnkeyedStruct)
{
  key_only_test<BasicUnkeyedStruct,
    NestedKeyOnly<BasicUnkeyedStruct>, NestedKeyOnly<const BasicUnkeyedStruct> >(false);
}

TEST(KeyTests, NestedKeyOnly_BasicKeyedStruct)
{
  key_only_test<BasicKeyedStruct,
    NestedKeyOnly<BasicKeyedStruct>, NestedKeyOnly<const BasicKeyedStruct> >(false);
}

TEST(KeyTests, NestedKeyOnly_UnkeyedUnion)
{
  key_only_test<UnkeyedUnion,
    NestedKeyOnly<UnkeyedUnion>, NestedKeyOnly<const UnkeyedUnion> >(false);
}

TEST(KeyTests, NestedKeyOnly_KeyedUnion)
{
  key_only_test<KeyedUnion,
    NestedKeyOnly<KeyedUnion>, NestedKeyOnly<const KeyedUnion> >(false);
}

TEST(KeyTests, NestedKeyOnly_ComplexUnkeyedStruct)
{
  key_only_test<ComplexUnkeyedStruct,
    NestedKeyOnly<ComplexUnkeyedStruct>, NestedKeyOnly<const ComplexUnkeyedStruct> >(false);
}

TEST(KeyTests, NestedKeyOnly_ComplexKeyedStruct)
{
  key_only_test<ComplexKeyedStruct,
    NestedKeyOnly<ComplexKeyedStruct>, NestedKeyOnly<const ComplexKeyedStruct> >(false);
}

TEST(KeyTests, KeyOnly_BasicUnkeyedStruct)
{
  key_only_test<BasicUnkeyedStruct,
    KeyOnly<BasicUnkeyedStruct>, KeyOnly<const BasicUnkeyedStruct> >(true);
}

TEST(KeyTests, KeyOnly_BasicKeyedStruct)
{
  key_only_test<BasicKeyedStruct,
    KeyOnly<BasicKeyedStruct>, KeyOnly<const BasicKeyedStruct> >(true);
}

TEST(KeyTests, KeyOnly_UnkeyedUnion)
{
  key_only_test<UnkeyedUnion,
    KeyOnly<UnkeyedUnion>, KeyOnly<const UnkeyedUnion> >(true);
}

TEST(KeyTests, KeyOnly_KeyedUnion)
{
  key_only_test<KeyedUnion,
    KeyOnly<KeyedUnion>, KeyOnly<const KeyedUnion> >(true);
}

TEST(KeyTests, KeyOnly_ComplexUnkeyedStruct)
{
  key_only_test<ComplexUnkeyedStruct,
    KeyOnly<ComplexUnkeyedStruct>, KeyOnly<const ComplexUnkeyedStruct> >(true);
}

TEST(KeyTests, KeyOnly_ComplexKeyedStruct)
{
  key_only_test<ComplexKeyedStruct,
    KeyOnly<ComplexKeyedStruct>, KeyOnly<const ComplexKeyedStruct> >(true);
}

// ----------------------------------------------------------------------------

template<>
void expect_values_equal(const Optional::OptionalMembers& a, const Optional::OptionalMembers& b)
{
  EXPECT_EQ(a.bool_field(), b.bool_field());
  EXPECT_EQ(a.short_field(), b.short_field());
  EXPECT_EQ(a.int32_field(), b.int32_field());
  EXPECT_EQ(a.int64_field(), b.int64_field());
  EXPECT_EQ(a.str_field(), b.str_field());
  EXPECT_EQ(a.seq_field(), b.seq_field());
  ASSERT_EQ(a.struct_field().has_value(), b.struct_field().has_value());
  if (a.struct_field().has_value()) {
    EXPECT_EQ(a.struct_field()->octet_field(), b.struct_field()->octet_field());
  }
}

TEST(OptionalTests, NotPresent)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x07, // +4 = 4

    // bool_field
    0x00, // +1 = 5

    // short_field
    0x00, // +1 = 6

    // int32_field
    0x00, // +1 = 7

    // int64_field
    0x00, // +1 = 8

    // str_field
    0x00, // +1 = 9

    0x00,

    0x00
  };

  Optional::OptionalMembers empty{};
  Optional::OptionalMembers result;
  amalgam_serializer_test(xcdr2, expected, empty, result);
}

TEST(OptionalTests, Present)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x1a, // +4 = 4

    // bool_field
    0x00,

    // short_field
    0x01, // +1 is_present = 5
    0x7f, 0xff, // +2 = 8

    // int32_field
    0x00, // +1 is_present = 9
    0x00,

    // str_field
    0x01, // +1 = 10
    0x00, // ?
    0x00, 0x00, 0x00, 0x0c, // +4 = 14
    'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '\0', // +12 = 26
    0x00,

    // struct_field
    0x00
  };

  Optional::OptionalMembers value{};
  value.short_field(0x7fff);
  value.str_field(OPENDDS_OPTIONAL_NS::optional<std::string>("Hello World"));
  Optional::OptionalMembers result;
  amalgam_serializer_test(xcdr2, expected, value, result);
}

int main(int argc, char* argv[])
{
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--dynamic")) {
      dynamic = true;
    }
  }
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
