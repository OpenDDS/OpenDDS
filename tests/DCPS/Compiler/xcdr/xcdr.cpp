#include "xcdrTypeSupportImpl.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

#include <string>
#include <cstring>

using namespace OpenDDS::DCPS;

const Encoding xcdr1(Encoding::KIND_XCDR1, ENDIAN_BIG);
const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);
// Big Endian just so it's easier to write expected CDR
const Encoding xcdr2_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);

void change_endianness(
  unsigned char* strm, const unsigned* layout, const size_t layout_size)
{
  size_t begin = 0;
  for (size_t i = 0; i < layout_size; ++i) {
    unsigned length = layout[i];
    // Swap bytes
    for (size_t j = 0; j < length/2; ++j) {
      std::swap(strm[begin + j], strm[begin + length - 1 - j]);
    }
    begin += length;
  }
}

template <typename Type>
void set_base_values(Type& value)
{
  value.short_field = 0x7fff;
  value.long_field = 0x7fffffff;
  value.octet_field = 0x01;
  value.long_long_field = 0x7fffffffffffffff;
}

template <typename Type>
void set_base_values_union(Type& value, UnionDisc disc)
{
  switch (disc) {
  case E_SHORT_FIELD:
    value.short_field(0x7fff);
    break;
  case E_LONG_FIELD:
    value.long_field(0x7fffffff);
    break;
  case E_OCTET_FIELD:
    value.octet_field(0x01);
    break;
  case E_LONG_LONG_FIELD:
    value.long_long_field(0x7fffffffffffffff);
    break;
  }
}

template <typename Type>
void set_values(Type& value)
{
  set_base_values(value);
}

template <typename Type>
void set_values_union(Type& value, UnionDisc disc)
{
  set_base_values_union(value, disc);
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base(const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a.short_field, b.short_field);
  EXPECT_EQ(a.long_field, b.long_field);
  EXPECT_EQ(a.octet_field, b.octet_field);
  EXPECT_EQ(a.long_long_field, b.long_long_field);
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base_union(const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a._d(), b._d());
  switch (a._d()) {
  case E_SHORT_FIELD:
    EXPECT_EQ(a.short_field(), b.short_field());
    break;
  case E_LONG_FIELD:
    EXPECT_EQ(a.long_field(), b.long_field());
    break;
  case E_OCTET_FIELD:
    EXPECT_EQ(a.octet_field(), b.octet_field());
    break;
  case E_LONG_LONG_FIELD:
    EXPECT_EQ(a.long_long_field(), b.long_long_field());
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

struct DataView {
  template <size_t array_size>
  DataView(const unsigned char (&array)[array_size])
  : data(reinterpret_cast<const char*>(&array[0]))
  , size(array_size)
  {
  }

  DataView(const ACE_Message_Block& mb)
  : data(mb.base())
  , size(mb.length())
  {
  }

  DataView(const unsigned char* const array, const size_t array_size)
  : data(reinterpret_cast<const char*>(array))
  , size(array_size)
  {
  }

  const char* const data;
  const size_t size;
};

bool operator==(const DataView& a, const DataView& b)
{
  return a.size == b.size ? !std::memcmp(a.data, b.data, a.size) : false;
}

::testing::AssertionResult assert_DataView(
  const char* a_expr, const char* b_expr,
  const DataView& a, const DataView& b)
{
  if (a == b) {
    return ::testing::AssertionSuccess();
  }

  std::string a_line;
  std::stringstream astrm(to_hex_dds_string(a.data, a.size, '\n', 8));
  std::string b_line;
  std::stringstream bstrm(to_hex_dds_string(b.data, b.size, '\n', 8));
  std::stringstream result;
  bool got_a = true;
  bool got_b = true;
  bool diff = true;
  while (got_a || got_b) {
    result.width(16);
    result << std::left;
    if (got_a && std::getline(astrm, a_line, '\n')) {
      result << a_line;
    } else {
      got_a = false;
      result << ' ';
    }
    if (got_b && std::getline(bstrm, b_line, '\n')) {
      result << " " << b_line;
    } else {
      got_b = false;
    }
    if (diff && a_line != b_line) {
      result << " <- Different Starting Here";
      diff = false;
    }
    result << std::endl;
  }
  return ::testing::AssertionFailure()
    << a_expr << " (size " << a.size << ") isn't the same as "
    << b_expr << " (size " << b.size << ").\n"
    << a_expr << " is on the left, " << b_expr << " is on the right:\n"
    << result.str();
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

template<typename TypeA, typename TypeB>
void amalgam_serializer_test(
  const Encoding& encoding, const DataView& expected_cdr, TypeA& value, TypeB& result)
{
  ACE_Message_Block buffer(1024);

  // Serialize and Compare CDR
  {
    Serializer serializer(&buffer, encoding);
    ASSERT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

  // Deserialize and Compare C++ Values
  {
    Serializer serializer(&buffer, encoding);
    ASSERT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(assert_values, value, result);
  }
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

template<typename Type>
void baseline_checks(const Encoding& encoding, const DataView& expected_cdr)
{
  Type value;
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);
  // TODO(iguessthislldo): Does not work for XCDR1 and XCDR2 Mutable and XCDR2 Appendable
  /* EXPECT_EQ(max_serialized_size(encoding, value), expected_cdr.size); */

  serializer_test<Type>(encoding, expected_cdr);
}

template<typename Type>
void baseline_checks_union(const Encoding& encoding, const DataView& expected_cdr, UnionDisc disc)
{
  Type value;
  value._d(disc);
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);
  serializer_test_union<Type>(encoding, expected_cdr, disc);
}

#define STREAM_DATA \
  static const unsigned char expected[]; \
  static const unsigned layout[];

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

template<typename TypeA, typename TypeB, typename BigEndianData>
void test_little_endian()
{
  unsigned char* strm_le = setup_little_endian<BigEndianData>();
  amalgam_serializer_test<TypeA, TypeB>(
    xcdr2_le, DataView(strm_le, sizeof(BigEndianData::expected)));
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

TEST(basic_tests, FinalXcdr1Struct)
{
  baseline_checks<FinalXcdr1Struct>(xcdr1, final_xcdr1_struct_expected);
}

const DataView appendable_xcdr1_struct_expected(final_xcdr1_struct_expected);

TEST(basic_tests, AppendableXcdr1Struct)
{
  baseline_checks<AppendableXcdr1Struct>(xcdr1, appendable_xcdr1_struct_expected);
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

TEST(basic_tests, MutableXcdr1Struct)
{
  baseline_checks<MutableXcdr1Struct>(xcdr1, mutable_xcdr1_struct_expected);
}

// XCDR2 =====================================================================

const unsigned char final_xcdr2_struct_expected[] = {
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

TEST(basic_tests, FinalXcdr2Struct)
{
  baseline_checks<FinalXcdr2Struct>(xcdr2, final_xcdr2_struct_expected);
}

const unsigned char appendable_xcdr2_struct_expected[] = {
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

TEST(basic_tests, AppendableXcdr2Struct)
{
  baseline_checks<AppendableXcdr2Struct>(xcdr2, appendable_xcdr2_struct_expected);
}

const unsigned char mutable_xcdr2_struct_expected[] = {
  0x00,0,0,0x24, // +4=4 Delimiter
  0x10,0,0,0x00, 0x7f,0xff,  (0),(0), // +4+2+(2)=12 short_field
  0x20,0,0,0x01, 0x7f,0xff,0xff,0xff, // +4+4    =20 long_field
  0x00,0,0,0x02, 0x01,   (0),(0),(0), // +4+1+(3)=28 octet_field
  0x30,0,0,0x03, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +4+8=40 long_long_field
};

TEST(basic_tests, MutableXcdr2Struct)
{
  baseline_checks<MutableXcdr2Struct>(xcdr2, mutable_xcdr2_struct_expected);
}

// XCDR1 and XCDR2 ===========================================================

TEST(basic_tests, FinalXcdr12Struct)
{
  baseline_checks<FinalXcdr12Struct>(xcdr1, final_xcdr1_struct_expected);
  baseline_checks<FinalXcdr12Struct>(xcdr2, final_xcdr2_struct_expected);
}

TEST(basic_tests, AppendableXcdr12Struct)
{
  baseline_checks<AppendableXcdr12Struct>(xcdr1, appendable_xcdr1_struct_expected);
  baseline_checks<AppendableXcdr12Struct>(xcdr2, appendable_xcdr2_struct_expected);
}

TEST(basic_tests, MutableXcdr12Struct)
{
  baseline_checks<MutableXcdr12Struct>(xcdr1, mutable_xcdr1_struct_expected);
  baseline_checks<MutableXcdr12Struct>(xcdr2, mutable_xcdr2_struct_expected);
}

// Union Tests -----------------------------------------------------

const unsigned char mutable_xcdr2_union_expected_short[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0e, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x00, // +4 = 12
  // short_field
  0x10, 0x00, 0x00, 0x00, // PID  +4 = 16
  0x7f, 0xff // +2 = 18
};

const unsigned char mutable_xcdr2_union_expected_long[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x10, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x01, // +4 = 12
  // long_field
  0x20, 0x00, 0x00, 0x01,// PID  +4 = 16
  0x7f, 0xff, 0xff, 0xff // +4 = 20
};

const unsigned char mutable_xcdr2_union_expected_octet[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0d, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x02, // +4 = 12
  // octet_field
  0x00, 0x00, 0x00, 0x02, // PID  +4 = 16
  0x01                    // +1 = 17
};

const unsigned char mutable_xcdr2_union_expected_long_long[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x14, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x03, // +4 = 12
  // long_field
  0x30, 0x00, 0x00, 0x03, // PID  +4 = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  // +8 = 24
};

template<>
void expect_values_equal(const MutableXcdr12Union& a, const MutableXcdr12Union& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(basic_tests, MutableXcdr12Union)
{
  baseline_checks_union<MutableXcdr12Union>(xcdr2, mutable_xcdr2_union_expected_short, E_SHORT_FIELD);
  baseline_checks_union<MutableXcdr12Union>(xcdr2, mutable_xcdr2_union_expected_long, E_LONG_FIELD);
  baseline_checks_union<MutableXcdr12Union>(xcdr2, mutable_xcdr2_union_expected_octet, E_OCTET_FIELD);
  baseline_checks_union<MutableXcdr12Union>(xcdr2, mutable_xcdr2_union_expected_long_long, E_LONG_LONG_FIELD);
}

// No Data-representation Annotation Tests -----------------------------------

TEST(basic_tests, NoDataRepFinalStruct)
{
  baseline_checks<NoDataRepFinalStruct>(xcdr2, final_xcdr2_struct_expected);
}

TEST(basic_tests, NoDataRepAppendableStruct)
{
  baseline_checks<NoDataRepAppendableStruct>(xcdr2, appendable_xcdr2_struct_expected);
}

TEST(basic_tests, NoDataRepMutableStruct)
{
  baseline_checks<NoDataRepMutableStruct>(xcdr2, mutable_xcdr2_struct_expected);
}
/*
template<>
void expect_values_equal(const NoDataRepFinalUnion& a,
                         const NoDataRepFinalUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(basic_tests, NoDataRepFinalUnion)
{
  baseline_checks_union<NoDataRepFinalUnion>(xcdr2, mutable_xcdr2_union_expected_short, E_SHORT_FIELD);
  baseline_checks_union<NoDataRepFinalUnion>(xcdr2, mutable_xcdr2_union_expected_long, E_LONG_FIELD);
  baseline_checks_union<NoDataRepFinalUnion>(xcdr2, mutable_xcdr2_union_expected_octet, E_OCTET_FIELD);
  baseline_checks_union<NoDataRepFinalUnion>(xcdr2, mutable_xcdr2_union_expected_long_long, E_LONG_LONG_FIELD);
}

template<>
void expect_values_equal(const NoDataRepAppendableUnion& a,
                         const NoDataRepAppendableUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(basic_tests, NoDataRepAppendableUnion)
{
  baseline_checks_union<NoDataRepAppendableUnion>(xcdr2, mutable_xcdr2_union_expected_short, E_SHORT_FIELD);
  baseline_checks_union<NoDataRepAppendableUnion>(xcdr2, mutable_xcdr2_union_expected_long, E_LONG_FIELD);
  baseline_checks_union<NoDataRepAppendableUnion>(xcdr2, mutable_xcdr2_union_expected_octet, E_OCTET_FIELD);
  baseline_checks_union<NoDataRepAppendableUnion>(xcdr2, mutable_xcdr2_union_expected_long_long, E_LONG_LONG_FIELD);
}
*/
template<>
void expect_values_equal(const NoDataRepMutableUnion& a,
                         const NoDataRepMutableUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(basic_tests, NoDataRepMutableUnion)
{
  baseline_checks_union<NoDataRepMutableUnion>(xcdr2, mutable_xcdr2_union_expected_short, E_SHORT_FIELD);
  baseline_checks_union<NoDataRepMutableUnion>(xcdr2, mutable_xcdr2_union_expected_long, E_LONG_FIELD);
  baseline_checks_union<NoDataRepMutableUnion>(xcdr2, mutable_xcdr2_union_expected_octet, E_OCTET_FIELD);
  baseline_checks_union<NoDataRepMutableUnion>(xcdr2, mutable_xcdr2_union_expected_long_long, E_LONG_LONG_FIELD);
}

// Appendable Tests ==========================================================

TEST(appendable_tests, FromNestedStruct)
{
  amalgam_serializer_test<NestedStruct, AdditionalFieldNestedStruct>(
    xcdr2, appendable_xcdr2_struct_expected);
}

const unsigned char additional_nested_expected_xcdr2[] = {
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

template<>
void set_values(AdditionalFieldNestedStruct& value)
{
  set_base_values(value);
  value.additional_field = 0x12345678;
}

TEST(appendable_tests, FromAdditionalNestedStruct)
{
  amalgam_serializer_test<AdditionalFieldNestedStruct, NestedStruct>(
    xcdr2, additional_nested_expected_xcdr2);
}

template<>
void expect_values_equal(const AdditionalFieldNestedStruct& a,
                         const AdditionalFieldNestedStruct& b)
{
  expect_values_equal_base(a, b);
  EXPECT_EQ(a.additional_field, b.additional_field);
}

TEST(appendable_tests, BothAdditionalNestedStruct)
{
  serializer_test<AdditionalFieldNestedStruct>(xcdr2, additional_nested_expected_xcdr2);
}

const unsigned char appendable_expected_xcdr2[] = {
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

template<>
void set_values(AppendableStruct& value)
{
  set_base_values(value.nested);
  set_base_values(value);
}

template<typename TypeA, typename TypeB>
void expect_equal_with_nested(const TypeA& a, const TypeB& b)
{
  expect_values_equal_base(a, b);
  expect_values_equal_base(a.nested, b.nested);
}

template<>
void expect_values_equal(const AppendableStruct& a, const AppendableStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(appendable_tests, BothAppendableStruct)
{
  serializer_test<AppendableStruct>(xcdr2, appendable_expected_xcdr2);
}

template<>
void expect_values_equal(const AppendableStruct& a,
                         const AdditionalFieldAppendableStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(appendable_tests, FromAppendableStruct)
{
  amalgam_serializer_test<AppendableStruct, AdditionalFieldAppendableStruct>(
    xcdr2, appendable_expected_xcdr2);
}

const unsigned char additional_appendable_expected_xcdr2[] = {
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

template<>
void set_values(AdditionalFieldAppendableStruct& value)
{
  set_values(value.nested);
  set_base_values(value);
  value.additional_field = 0x12345678;
}

template<>
void expect_values_equal(const AdditionalFieldAppendableStruct& a,
                         const AppendableStruct& b)
{
  expect_equal_with_nested(a, b);
}

TEST(appendable_tests, FromAdditionalAppendableStruct)
{
  amalgam_serializer_test<AdditionalFieldAppendableStruct, AppendableStruct>(
    xcdr2, additional_appendable_expected_xcdr2);
}

template<>
void expect_values_equal(const AdditionalFieldAppendableStruct& a,
                         const AdditionalFieldAppendableStruct& b)
{
  expect_equal_with_nested(a, b);
  EXPECT_EQ(a.additional_field, b.additional_field);
  EXPECT_EQ(a.nested.additional_field, b.nested.additional_field);
}

TEST(appendable_tests, BothAdditionalAppendableStruct)
{
  serializer_test<AdditionalFieldAppendableStruct>(
    xcdr2, additional_appendable_expected_xcdr2);
}

const unsigned char appendable_expected2_xcdr2[] = {
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

template<>
void set_values(AppendableStruct2& value)
{
  set_values(value.nested);
  value.string_field = "abcdefghi";
}

template<>
void expect_values_equal(const AppendableStruct2& a,
                         const AppendableStruct2& b)
{
  expect_values_equal(a.nested, b.nested);
  EXPECT_STREQ(a.string_field, b.string_field);
}

TEST(appendable_tests, BothAppendableStruct2)
{
  serializer_test<AppendableStruct2>(xcdr2, appendable_expected2_xcdr2);
}

// Mutable Tests =============================================================

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

const unsigned char mutable_struct_expected_xcdr2[] = {
  0x00,0,0,0x24, // +4=4 Delimiter
  0x10,0,0,0x04, 0x7f,0xff,  (0),(0), // +4+2+(2)=12 short_field
  0x20,0,0,0x06, 0x7f,0xff,0xff,0xff, // +4+4    =20 long_field
  0x00,0,0,0x08, 0x01,   (0),(0),(0), // +4+1+(3)=28 octet_field
  0x30,0,0,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff // +4+8=40 long_long_field
};

// Baseline ------------------------------------------------------------------

TEST(mutable_tests, baseline_xcdr1_test)
{
  baseline_checks<MutableStruct>(xcdr1, mutable_struct_expected_xcdr1);
}

TEST(mutable_tests, baseline_xcdr2_test)
{
  baseline_checks<MutableStruct>(xcdr2, mutable_struct_expected_xcdr2);
}

const unsigned char mutable_union_expected_xcdr2_short[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0e, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x00, // +4 = 12
  // short_field
  0x10, 0x00, 0x00, 0x04, // PID  +4 = 16
  0x7f, 0xff // +2 = 18
};

const unsigned char mutable_union_expected_xcdr2_long[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x10, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x01, // +4 = 12
  // long_field
  0x20, 0x00, 0x00, 0x06,// PID  +4 = 16
  0x7f, 0xff, 0xff, 0xff // +4 = 20
};

const unsigned char mutable_union_expected_xcdr2_octet[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x0d, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x02, // +4 = 12
  // octet_field
  0x00, 0x00, 0x00, 0x08, // PID  +4 = 16
  0x01                    // +1 = 17
};

const unsigned char mutable_union_expected_xcdr2_long_long[] = {
  // Delimiter
  0x00, 0x00, 0x00, 0x14, // +4 = 4
  // Discriminator EMHEADER
  0x20, 0x00, 0x00, 0x00, // PID  +4 = 8
  // Discriminator
  0x00, 0x00, 0x00, 0x03, // +4 = 12
  // long_field
  0x30, 0x00, 0x00, 0x0a, // PID  +4 = 16
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  // +8 = 24
};

template<>
void expect_values_equal(const MutableUnion& a, const MutableUnion& b)
{
  expect_values_equal_base_union(a, b);
}

TEST(mutable_tests, baseline_xcdr2_test_union)
{
  baseline_checks_union<MutableUnion>(xcdr2, mutable_union_expected_xcdr2_short, E_SHORT_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, mutable_union_expected_xcdr2_long, E_LONG_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, mutable_union_expected_xcdr2_octet, E_OCTET_FIELD);
  baseline_checks_union<MutableUnion>(xcdr2, mutable_union_expected_xcdr2_long_long, E_LONG_LONG_FIELD);
}

// Reordered Tests -----------------------------------------------------------
// Test compatibility between two structures with different field orders.

TEST(mutable_tests, to_reordered_xcdr1_test)
{
  amalgam_serializer_test<MutableStruct, ReorderedMutableStruct>(
    xcdr1, mutable_struct_expected_xcdr1);
}

TEST(mutable_tests, from_reordered_xcdr1_test)
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
  amalgam_serializer_test<ReorderedMutableStruct, MutableStruct>(xcdr1, expected);
}

TEST(mutable_tests, to_reordered_xcdr2_test)
{
  amalgam_serializer_test<MutableStruct, ReorderedMutableStruct>(
    xcdr2, mutable_struct_expected_xcdr2);
}

TEST(mutable_tests, from_reordered_xcdr2_test)
{
  const unsigned char expected[] = {
    0x00,0,0,0x22, // +4=4 Delimiter
    0x20,0,0,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0x30,0,0,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=24 long_long_field
    0x00,0,0,0x08, 0x01,   (0),(0),(0), // +4+1+(3)=32 octet_field
    0x10,0,0,0x04, 0x7f,0xff            // +4+2    =38 short_field
  };
  amalgam_serializer_test<ReorderedMutableStruct, MutableStruct>(xcdr2, expected);
}

// ---------- AdditionalFieldMutableStruct
// Test compatibility between two structures with different fields

template <>
void set_values<AdditionalFieldMutableStruct>(
  AdditionalFieldMutableStruct& value)
{
  set_base_values(value);
  value.additional_field = 0x12345678;
}

TEST(mutable_tests, to_additional_field_xcdr1_test)
{
  MutableStruct value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr1, mutable_struct_expected_xcdr1, value, result);
  /// TODO(iguessthidlldo): Test for correct try construct behavior (default
  /// value?) if we decide on that.
}

TEST(mutable_tests, from_additional_field_xcdr1_test)
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
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStruct>(xcdr1, expected);
}

TEST(mutable_tests, to_additional_field_xcdr2_test)
{
  MutableStruct value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr2, mutable_struct_expected_xcdr2, value, result);
  /// TODO(iguessthidlldo): Test for correct try construct behavior (default
  /// value?) if we decide on that.
}

TEST(mutable_tests, from_additional_field_must_understand_test)
{
  const unsigned char additional_field_must_understand[] = {
    0x00,0x00,0x00,0x2a, // +4=4 Delimiter
    0x20,0x00,0x00,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0xa0,0x00,0x00,0x01, 0x12,0x34,0x56,0x78, // +4+4=20 additional_field @must_understand
    0x30,0x00,0x00,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=32 long_long_field
    0x00,0x00,0x00,0x08, 0x01, (0),(0),(0), // +4+1+(3)=40 octet_field
    0x10,0x00,0x00,0x04, 0x7f,0xff // +4+2=46 short_field
  };

  // Deserialization should fail for unknown must_understand field
  ACE_Message_Block buffer(1024);
  buffer.copy((const char*)additional_field_must_understand, sizeof(additional_field_must_understand));
  Serializer serializer(&buffer, xcdr2);
  MutableStruct result;
  EXPECT_FALSE(serializer >> result);

  // Deserialize and Compare C++ Values
  AdditionalFieldMutableStruct expected;
  set_values(expected);
  deserialize_compare(xcdr2, additional_field_must_understand, expected);
}

TEST(mutable_tests, from_additional_field_xcdr2_test)
{
  const unsigned char expected[] = {
    0x00,0x00,0x00,0x2a, // +4=4 Delimiter
    0x20,0x00,0x00,0x06, 0x7f,0xff,0xff,0xff, // +4+4=12 long_field
    0x20,0x00,0x00,0x01, 0x12,0x34,0x56,0x78, // +4+4=20 additional_field
    0x30,0x00,0x00,0x0a, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=32 long_long_field
    0x00,0x00,0x00,0x08, 0x01, (0),(0),(0), // +4+1+(3)=40 octet_field
    0x10,0x00,0x00,0x04, 0x7f,0xff // +4+2=46 short_field
  };
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStruct>(xcdr2, expected);
}

// ---------- LengthCodeStruct
template<>
void expect_values_equal(const LengthCodeStruct& a, const LengthCodeStruct& b)
{
  EXPECT_EQ(a.o, b.o);
  EXPECT_EQ(a.s, b.s);
  EXPECT_EQ(a.l, b.l);
  EXPECT_EQ(a.ll, b.ll);

  EXPECT_EQ(a.b3.a, b.b3.a);
  EXPECT_EQ(a.b3.b, b.b3.b);
  EXPECT_EQ(a.b3.c, b.b3.c);

  EXPECT_EQ(a.o5.a, b.o5.a);
  EXPECT_EQ(a.o5.b, b.o5.b);
  EXPECT_EQ(a.o5.c, b.o5.c);
  EXPECT_EQ(a.o5.d, b.o5.d);
  EXPECT_EQ(a.o5.e, b.o5.e);

  EXPECT_EQ(a.s3.x, b.s3.x);
  EXPECT_EQ(a.s3.y, b.s3.y);
  EXPECT_EQ(a.s3.z, b.s3.z);

  EXPECT_EQ(a.t7.s3.x, b.t7.s3.x);
  EXPECT_EQ(a.t7.s3.y, b.t7.s3.y);
  EXPECT_EQ(a.t7.s3.z, b.t7.s3.z);
  EXPECT_EQ(a.t7.o, b.t7.o);

  EXPECT_EQ(a.l3.a, b.l3.a);
  EXPECT_EQ(a.l3.b, b.l3.b);
  EXPECT_EQ(a.l3.c, b.l3.c);

  EXPECT_STREQ(a.str1, b.str1);
  EXPECT_STREQ(a.str2, b.str2);
  EXPECT_STREQ(a.str3, b.str3);
  EXPECT_STREQ(a.str4, b.str4);
  EXPECT_STREQ(a.str5, b.str5);
}

TEST(mutable_tests, length_code_test)
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
  EXPECT_EQ(a.o3.length(), b.o3.length());
  EXPECT_EQ(a.o3[0], b.o3[0]);
  EXPECT_EQ(a.o3[1], b.o3[1]);
  EXPECT_EQ(a.o3[2], b.o3[2]);

  EXPECT_EQ(a.l3.length(), b.l3.length());
  EXPECT_EQ(a.l3[0], b.l3[0]);
  EXPECT_EQ(a.l3[1], b.l3[1]);
  EXPECT_EQ(a.l3[2], b.l3[2]);

  EXPECT_EQ(a.ll3.length(), b.ll3.length());
  EXPECT_EQ(a.ll3[0], b.ll3[0]);
  EXPECT_EQ(a.ll3[1], b.ll3[1]);
  EXPECT_EQ(a.ll3[2], b.ll3[2]);

  EXPECT_EQ(a.s3.length(), b.s3.length());
  EXPECT_EQ(a.s3[0], b.s3[0]);
  EXPECT_EQ(a.s3[1], b.s3[1]);

  EXPECT_STREQ(a.str4, b.str4);
  EXPECT_STREQ(a.str5, b.str5);

  EXPECT_EQ(a.ls.length(), b.ls.length());
  EXPECT_EQ(a.ls[0], b.ls[0]);
  EXPECT_EQ(a.ls[1], b.ls[1]);
}

TEST(mutable_tests, read_lc567_test)
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
  expected.o3.length(3); expected.l3.length(3); expected.ll3.length(3);
  expected.o3[0] = 1;    expected.l3[0] = 1;    expected.ll3[0] = 1;
  expected.o3[1] = 2;    expected.l3[1] = 2;    expected.ll3[1] = 2;
  expected.o3[2] = 3;    expected.l3[2] = 3;    expected.ll3[2] = 3;
  expected.s3.length(2); expected.ls.length(2);
  expected.s3[0] = 1;    expected.ls[0] = 1;
  expected.s3[1] = 2;    expected.ls[1] = 2;
  expected.str4 = "abc";
  expected.str5 = "abcd";
  expected.str7 = "abcdef";

  deserialize_compare(xcdr2, data, expected);

  // Little-endian test
  change_endianness(data, layout, sizeof(layout)/sizeof(unsigned));
  deserialize_compare(xcdr2_le, data, expected);
}

// ---------- MixedMutableStruct
template<>
void set_values(MixedMutableStruct& value)
{
  set_values(value.struct_nested);
  value.sequence_field.length(3);
  value.sequence_field[0] = value.sequence_field[1] = value.sequence_field[2] = 0x7fff;
  value.union_nested.string_field("abcdefghi");
  // Set discriminator after so it doesn't get overwritten
  value.union_nested._d(3);
  value.sequence_field2.length(3);
  value.sequence_field2[0] = "my string1";
  value.sequence_field2[1] = "my string2";
  value.sequence_field2[2] = "my string3";
}

template<>
void expect_values_equal(const MixedMutableStruct& a,
                         const MixedMutableStruct& b)
{
  expect_values_equal(a.struct_nested, b.struct_nested);
  EXPECT_EQ(a.sequence_field.length(), b.sequence_field.length());
  for (size_t i = 0; i < a.sequence_field.length(); ++i) {
    EXPECT_EQ(a.sequence_field[i], b.sequence_field[i]);
  }
  EXPECT_EQ(a.union_nested._d(), b.union_nested._d());
  EXPECT_STREQ(a.union_nested.string_field(), b.union_nested.string_field());
  EXPECT_EQ(a.sequence_field2.length(), b.sequence_field2.length());
  for (size_t i = 0; i < a.sequence_field2.length(); ++i) {
    EXPECT_STREQ(a.sequence_field2[i], b.sequence_field2[i]);
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
  0x40,0,0,2,  0,0,0,10, 0,0,0,3,0x7f,0xff,0x7f,0xff,0x7f,0xff,(0),(0), // +20 sequence_field = 72
  0x40,0,0,3,  0,0,0,34, // +8 EMHEADER1 + NEXTINT of union_nested = 80
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

TEST(mutable_tests, BothMixedMutableStruct)
{
  serializer_test<MixedMutableStruct>(xcdr2, MixedMutableStructXcdr2BE::expected);
}

TEST(mutable_tests, BothMixedMutableStructLE)
{
  test_little_endian<MixedMutableStruct, MixedMutableStructXcdr2BE>();
}

template<>
void expect_values_equal(const MixedMutableStruct& a,
                         const ModifiedMixedMutableStruct& b)
{
  expect_values_equal_base(a.struct_nested, b.struct_nested);
}

TEST(mutable_tests, FromMixedMutableStruct)
{
  amalgam_serializer_test<MixedMutableStruct, ModifiedMixedMutableStruct>(
    xcdr2, MixedMutableStructXcdr2BE::expected);
}

TEST(mutable_tests, FromMixedMutableStructLE)
{
  test_little_endian<MixedMutableStruct, ModifiedMixedMutableStruct,
                     MixedMutableStructXcdr2BE>();
}

// Mixed Extensibility Tests ================================================

// ---------- NestingFinalStruct
template<>
void set_values(NestingFinalStruct& value)
{
  value.string_field = "make sense";
  set_values(value.appendable_nested);
  value.sequence_field.length(3);
  value.sequence_field[0] = value.sequence_field[1] = value.sequence_field[2] = 0x1234;
  set_values(value.mutable_nested);
}

template<>
void expect_values_equal(const NestingFinalStruct& a,
                         const NestingFinalStruct& b)
{
  EXPECT_STREQ(a.string_field, b.string_field);
  expect_values_equal_base(a.appendable_nested, b.appendable_nested);
  EXPECT_EQ(a.sequence_field.length(), b.sequence_field.length());
  for (size_t i = 0; i < a.sequence_field.length(); ++i) {
    EXPECT_EQ(a.sequence_field[i], b.sequence_field[i]);
  }
  expect_values_equal_base(a.mutable_nested, b.mutable_nested);
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

TEST(mixed_exten_tests, NestingFinalStruct)
{
  serializer_test<NestingFinalStruct>(xcdr2, NestingFinalStructXcdr2BE::expected);
}

TEST(mixed_exten_tests, NestingFinalStructLE)
{
  test_little_endian<NestingFinalStruct, NestingFinalStructXcdr2BE>();
}

// ---------- NestingAppendableStruct
template<>
void set_values(NestingAppendableStruct& value)
{
  value.string_field = "hello world";
  set_values(value.mutable_nested);
  value.sequence_field.length(3);
  for (size_t i = 0; i < value.sequence_field.length(); ++i) {
    value.sequence_field[i] = 0x7fffffff;
  }
  set_values(value.final_nested);
}

template<>
void expect_values_equal(const NestingAppendableStruct& a,
                         const NestingAppendableStruct& b)
{
  EXPECT_STREQ(a.string_field, b.string_field);
  expect_values_equal_base(a.mutable_nested, b.mutable_nested);
  EXPECT_EQ(a.sequence_field.length(), b.sequence_field.length());
  for (size_t i = 0; i < a.sequence_field.length(); ++i) {
    EXPECT_EQ(a.sequence_field[i], b.sequence_field[i]);
  }
  expect_values_equal_base(a.final_nested, b.final_nested);
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

TEST(mixed_exten_tests, NestingAppendableStruct)
{
  serializer_test<NestingAppendableStruct>(xcdr2, NestingAppendableStructXcdr2BE::expected);
}

TEST(mixed_exten_tests, NestingAppendableStructLE)
{
  test_little_endian<NestingAppendableStruct, NestingAppendableStructXcdr2BE>();
}

// ---------- NestingMutableStruct
template<>
void set_values(NestingMutableStruct& value)
{
  value.string_field = "hello world";
  set_values(value.appendable_nested);
  value.sequence_field.length(7);
  for (size_t i = 0; i < value.sequence_field.length(); ++i) {
    value.sequence_field[i] = 0x7f;
  }
  set_values(value.final_nested);
}

template<>
void expect_values_equal(const NestingMutableStruct& a,
                         const NestingMutableStruct& b)
{
  EXPECT_STREQ(a.string_field, b.string_field);
  expect_values_equal_base(a.appendable_nested, b.appendable_nested);
  EXPECT_EQ(a.sequence_field.length(), b.sequence_field.length());
  for (size_t i = 0; i < a.sequence_field.length(); ++i) {
    EXPECT_EQ(a.sequence_field[i], b.sequence_field[i]);
  }
  expect_values_equal_base(a.final_nested, b.final_nested);
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

TEST(mixed_exten_tests, NestingMutableStruct)
{
  serializer_test<NestingMutableStruct>(xcdr2, NestingMutableStructXcdr2BE::expected);
}

TEST(mixed_exten_tests, NestingMutableStructLE)
{
  test_little_endian<NestingMutableStruct, NestingMutableStructXcdr2BE>();
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
