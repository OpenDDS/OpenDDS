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

template <typename Type>
void set_base_values(Type& value)
{
  value.short_field = 0x7fff;
  value.long_field = 0x7fffffff;
  value.octet_field = 0x01;
  value.long_long_field = 0x7fffffffffffffff;
}

template <typename Type>
void set_values(Type& value)
{
  set_base_values(value);
}

template <>
void set_values<AdditionalFieldMutableStruct>(
  AdditionalFieldMutableStruct& value)
{
  set_base_values(value);
  value.additional_field = 0x12345678;
}

template<typename TypeA, typename TypeB>
void expect_values_equal_base(
  const TypeA& a, const TypeB& b)
{
  EXPECT_EQ(a.short_field, b.short_field);
  EXPECT_EQ(a.long_field, b.long_field);
  EXPECT_EQ(a.octet_field, b.octet_field);
  EXPECT_EQ(a.long_long_field, b.long_long_field);
}

template<typename TypeA, typename TypeB>
void expect_values_equal(
  const TypeA& a, const TypeB& b)
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
  DataView(const std::string& s)
  : data(s.data())
  , size(s.size())
  {
  }

  DataView(const ACE_Message_Block& mb)
  : data(mb.base())
  , size(mb.length())
  {
  }

  const char* data;
  size_t size;
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

template<typename TypeA, typename TypeB>
void amalgam_serializer_test(
  const Encoding& encoding, const std::string expected_cdr, TypeA& value, TypeB& result)
{
  ACE_Message_Block buffer(1024);

  // Serialize and Compare CDR
  {
    Serializer serializer(&buffer, encoding);
    EXPECT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

  // Deserialize and Compare C++ Values
  {
    Serializer serializer(&buffer, encoding);
    EXPECT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(assert_values, value, result);
  }
}

template<typename TypeA, typename TypeB>
void amalgam_serializer_test(const Encoding& encoding, const std::string expected_cdr)
{
  TypeA value;
  set_values(value);
  TypeB result;
  amalgam_serializer_test<TypeA, TypeB>(encoding, expected_cdr, value, result);
}

template<typename Type>
void serializer_test(const Encoding& encoding, const std::string expected_cdr)
{
  amalgam_serializer_test<Type, Type>(encoding, expected_cdr);
}

// XCDR1 =====================================================================

const std::string xcdr1_non_mutable_expected(
  // short_field
  "\x7f\xff" // +2 = 2
  // long_field
  "\x00\x00" // +2 pad = 4
  "\x7f\xff\xff\xff" // +4 = 8
  // octet_field
  "\x01" // +1 = 9
  // long_long_field
  "\x00\x00\x00\x00\x00\x00\x00" // +7 pad = 16
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 24
  , 24);

// FinalXcdr1Struct ----------------------------------------------------------

TEST(FinalXcdr1Struct_tests, serialized_size_test)
{
  FinalXcdr1Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
}

TEST(FinalXcdr1Struct_tests, serializer_test)
{
  serializer_test<FinalXcdr1Struct>(xcdr1, xcdr1_non_mutable_expected);
}

// AppendableXcdr1Struct -----------------------------------------------------

TEST(AppendableXcdr1Struct_tests, serialized_size_test)
{
  AppendableXcdr1Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
}

TEST(AppendableXcdr1Struct_tests, serializer_test)
{
  serializer_test<AppendableXcdr1Struct>(xcdr1, xcdr1_non_mutable_expected);
}

// MutableXcdr1Struct --------------------------------------------------------

const std::string xcdr1_mutable_expected(
  // short_field
  "\xc0\x00\x00\x02" // PID +4 = 4
  "\x7f\xff" // +2 = 6
  // long_field
  "\x00\x00" // +2 pad = 8
  "\xc0\x01\x00\x04" // PID +4 = 12
  "\x7f\xff\xff\xff" // +4 = 16
  // octet_field
  "\xc0\x02\x00\x01" // PID +4 = 20
  "\x01" // +1 = 21
  // long_long_field
  "\x00\x00\x00" // +3 pad = 24
  "\xc0\x03\x00\x08" // PID +4 = 28
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 36
  // Sentinel
  "\x3f\x02\x00\x00" // +4 = 40
  , 40);

TEST(MutableXcdr1Struct_tests, serialized_size_test)
{
  MutableXcdr1Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_mutable_expected.size());
}

TEST(MutableXcdr1Struct_tests, serializer_test)
{
  serializer_test<MutableXcdr1Struct>(xcdr1, xcdr1_mutable_expected);
}

// XCDR2 =====================================================================

const std::string xcdr2_final_expected(
  // short_field
  "\x7f\xff" // +2 = 2
  // long_field
  "\x00\x00" // +2 pad = 4
  "\x7f\xff\xff\xff" // +4 = 8
  // octet_field
  "\x01" // +1 = 9
  // long_long_field
  "\x00\x00\x00" // +3 pad = 12
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 20
  , 20);

// FinalXcdr2Struct ----------------------------------------------------------

TEST(FinalXcdr2Struct_tests, serialized_size_test)
{
  FinalXcdr2Struct value;
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_final_expected.size());
}

TEST(FinalXcdr2Struct_tests, serializer_test)
{
  serializer_test<FinalXcdr2Struct>(xcdr2, xcdr2_final_expected);
}

// AppendableXcdr2Struct -----------------------------------------------------

const std::string xcdr2_appendable_expected(
  // Delimiter
  "\x00\x00\x00\x14" // +4 = 4
  // short_field
  "\x7f\xff" // +2 = 6
  // long_field
  "\x00\x00" // +2 pad = 8
  "\x7f\xff\xff\xff" // +4 = 12
  // octet_field
  "\x01" // +1 = 13
  // long_long_field
  "\x00\x00\x00" // +3 pad = 16
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 24
  , 24);

TEST(AppendableXcdr2Struct_tests, serialized_size_test)
{
  AppendableXcdr2Struct value;
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_appendable_expected.size());
}

TEST(AppendableXcdr2Struct_tests, serializer_test)
{
  serializer_test<AppendableXcdr2Struct>(xcdr2, xcdr2_appendable_expected);
}

// MutableXcdr2Struct --------------------------------------------------------

const std::string xcdr2_mutable_expected(
  // Delimiter
  "\x00\x00\x00\x24" // +4 = 4
  // short_field
  "\x00\x00\x00\x00" // PID  +4 = 8
  "\x7f\xff" // +2 = 10
  // long_field
  "\x00\x00" // +2 pad = 12
  "\x00\x00\x00\x01" // PID  +4 = 16
  "\x7f\xff\xff\xff" // +4 = 20
  // octet_field
  "\x00\x00\x00\x02" // PID  +4 = 24
  "\x01" // +1 = 25
  // long_long_field
  "\x00\x00\x00" // +3 pad = 28
  "\x00\x00\x00\x03" // PID  +4 = 32
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 40
  , 40);

TEST(MutableXcdr2Struct_tests, serialized_size_test)
{
  MutableXcdr2Struct value;
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_mutable_expected.size());
}

TEST(MutableXcdr2Struct_tests, serializer_test)
{
  serializer_test<MutableXcdr2Struct>(xcdr2, xcdr2_mutable_expected);
}

// XCDR1 and XCDR2 ===========================================================

// FinalXcdr12Struct ---------------------------------------------------------

TEST(FinalXcdr12Struct_tests, serialized_size_test)
{
  FinalXcdr12Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_final_expected.size());
}

TEST(FinalXcdr12Struct_tests, serializer_test)
{
  serializer_test<FinalXcdr12Struct>(xcdr1, xcdr1_non_mutable_expected);
  serializer_test<FinalXcdr12Struct>(xcdr2, xcdr2_final_expected);
}

// AppendableXcdr12Struct ----------------------------------------------------

TEST(AppendableXcdr12Struct_tests, serialized_size_test)
{
  AppendableXcdr12Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_appendable_expected.size());
}

TEST(AppendableXcdr12Struct_tests, serializer_test)
{
  serializer_test<AppendableXcdr12Struct>(xcdr1, xcdr1_non_mutable_expected);
  serializer_test<AppendableXcdr12Struct>(xcdr2, xcdr2_appendable_expected);
}

// MutableXcdr12Struct -------------------------------------------------------

TEST(MutableXcdr12Struct_tests, serialized_size_test)
{
  MutableXcdr12Struct value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_mutable_expected.size());
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_mutable_expected.size());
}

TEST(MutableXcdr12Struct_tests, serializer_test)
{
  serializer_test<MutableXcdr12Struct>(xcdr1, xcdr1_mutable_expected);
  serializer_test<MutableXcdr12Struct>(xcdr2, xcdr2_mutable_expected);
}

// Appendable Tests ==========================================================
// TODO(iguessthislldo)

// Mutable Tests =============================================================

const std::string mutable_struct_expected_xcdr1(
  // short_field
  "\xc0\x04\x00\x02" // PID +4 = 4
  "\x7f\xff" // +2 = 6
  // long_field
  "\x00\x00" // +2 pad = 8
  "\xc0\x06\x00\x04" // PID +4 = 12
  "\x7f\xff\xff\xff" // +4 = 16
  // octet_field
  "\xc0\x08\x00\x01" // PID +4 = 20
  "\x01" // +1 = 21
  // long_long_field
  "\x00\x00\x00" // +3 pad = 24
  "\xc0\x0a\x00\x08" // PID +4 = 28
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 36
  // Sentinel
  "\x3f\x02\x00\x00" // +4 = 40
  , 40);

const std::string mutable_struct_expected_xcdr2(
  // Delimiter
  "\x00\x00\x00\x24" // +4 = 4
  // short_field
  "\x00\x00\x00\x04" // PID +4 = 8
  "\x7f\xff" // +2 = 10
  // long_field
  "\x00\x00" // +2 pad = 12
  "\x00\x00\x00\x06" // PID +4 = 16
  "\x7f\xff\xff\xff" // +4 = 20
  // octet_field
  "\x00\x00\x00\x08" // PID +4 = 24
  "\x01" // +1 = 25
  // long_long_field
  "\x00\x00\x00" // +3 pad = 28
  "\x00\x00\x00\x0a" // PID +4 = 32
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 40
  , 40);

// Baseline ------------------------------------------------------------------

TEST(Mutable_tests, baseline_xcdr1_test)
{
  serializer_test<MutableStruct>(xcdr1, mutable_struct_expected_xcdr1);
}

TEST(Mutable_tests, baseline_xcdr2_test)
{
  serializer_test<MutableStruct>(xcdr2, mutable_struct_expected_xcdr2);
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
  amalgam_serializer_test<ReorderedMutableStruct, MutableStruct>(
    xcdr1, std::string(
      // long_field
      "\xc0\x06\x00\x04" // PID +4 = 4
      "\x7f\xff\xff\xff" // +4 = 8
      // long_long_field
      "\xc0\x0a\x00\x08" // PID +4 = 12
      "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 20
      // octet_field
      "\xc0\x08\x00\x01" // PID +4 = 24
      "\x01" // +1 = 25
      // short_field
      "\x00\x00\x00" // +3 pad = 28
      "\xc0\x04\x00\x02" // PID +4 = 32
      "\x7f\xff" // +2 = 34
      // Sentinel
      "\x00\x00" // +2 pad = 36
      "\x3f\x02\x00\x00" // +4 = 40
      , 40));
}

TEST(mutable_tests, to_reordered_xcdr2_test)
{
  amalgam_serializer_test<MutableStruct, ReorderedMutableStruct>(
    xcdr2, mutable_struct_expected_xcdr2);
}

TEST(mutable_tests, from_reordered_xcdr2_test)
{
  amalgam_serializer_test<ReorderedMutableStruct, MutableStruct>(
    xcdr2, std::string(
      // Delimiter
      "\x00\x00\x00\x22" // +4 = 4
      // long_field
      "\x00\x00\x00\x06" // PID +4 = 8
      "\x7f\xff\xff\xff" // +4 = 12
      // long_long_field
      "\x00\x00\x00\x0a" // PID +4 = 16
      "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 24
      // octet_field
      "\x00\x00\x00\x08" // PID +4 = 28
      "\x01" // +1 = 29
      // short_field
      "\x00\x00\x00" // +3 pad = 32
      "\x00\x00\x00\x04" // PID +4 = 36
      "\x7f\xff" // +2 = 38
      , 38));
}

// Additional Field Tests ----------------------------------------------------
// Test compatibility between two structures with different fields

TEST(mutable_tests, to_additional_field_xcdr1_test)
{
  MutableStruct value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr1, mutable_struct_expected_xcdr1, value, result);
  /// TODO(iguessthidlldo): Test that additional_field has been defaulted
  /* EXPECT_EQ(0, result.additional_field); */
}

TEST(mutable_tests, from_additional_field_xcdr1_test)
{
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStruct>(
    xcdr1, std::string(
      // long_field
      "\xc0\x06\x00\x04" // PID +4 = 4
      "\x7f\xff\xff\xff" // +4 = 8
      // additional_field
      "\xc0\x01\x00\x04" // PID +4 = 12
      "\x12\x34\x56\x78" // +4 = 16
      // long_long_field
      "\xc0\x0a\x00\x08" // PID +4 = 20
      "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 28
      // octet_field
      "\xc0\x08\x00\x01" // PID +4 = 32
      "\x01" // +1 = 33
      // short_field
      "\x00\x00\x00" // +3 pad = 36
      "\xc0\x04\x00\x02" // PID +4 = 40
      "\x7f\xff" // +2 = 42
      // Sentinel
      "\x00\x00" // +2 pad = 44
      "\x3f\x02\x00\x00" // +4 = 48
      , 48));
}

TEST(mutable_tests, to_additional_field_xcdr2_test)
{
  MutableStruct value;
  set_values(value);
  AdditionalFieldMutableStruct result;
  amalgam_serializer_test(xcdr2, mutable_struct_expected_xcdr2, value, result);
  /// TODO(iguessthidlldo): Test that additional_field has been defaulted
  /* EXPECT_EQ(0, result.additional_field); */
}

TEST(mutable_tests, from_additional_field_xcdr2_test)
{
  amalgam_serializer_test<AdditionalFieldMutableStruct, MutableStruct>(
    xcdr2, std::string(
      // Delimiter
      "\x00\x00\x00\x2a" // +4 = 4
      // long_field
      "\x00\x00\x00\x06" // PID +4 = 8
      "\x7f\xff\xff\xff" // +4 = 12
      // additional_field
      "\x00\x00\x00\x01" // PID +4 = 16
      "\x12\x34\x56\x78" // PID +4 = 20
      // long_long_field
      "\x00\x00\x00\x0a" // PID +4 = 24
      "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 32
      // octet_field
      "\x00\x00\x00\x08" // PID +4 = 36
      "\x01" // +1 = 37
      // short_field
      "\x00\x00\x00" // +3 pad = 40
      "\x00\x00\x00\x04" // PID +4 = 44
      "\x7f\xff" // +2 = 46
      , 46));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
