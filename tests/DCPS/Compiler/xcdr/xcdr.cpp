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
void set_values(Type& value)
{
  value.short_field = 0x7fff;
  value.long_field = 0x7fffffff;
  value.octet_field = 0x01;
  value.long_long_field = 0x7fffffffffffffff;
}

template <typename Type>
::testing::AssertionResult AssertValues(
  const char* a_expr, const char* b_expr,
  const Type& a, const Type& b)
{
  EXPECT_EQ(a.short_field, b.short_field);
  EXPECT_EQ(a.long_field, b.long_field);
  EXPECT_EQ(a.octet_field, b.octet_field);
  EXPECT_EQ(a.long_long_field, b.long_long_field);
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

::testing::AssertionResult AssertDataView(
  const char* a_expr, const char* b_expr,
  const DataView& a, const DataView& b)
{
  return a == b ?
    ::testing::AssertionSuccess() :
    ::testing::AssertionFailure()
      << a_expr << " (size " << a.size << ") isn't the same as "
        << b_expr << " (size " << b.size << ").\n"
      << a_expr << ":\n" << to_hex_dds_string(a.data, a.size, '\n', 8) << "\n"
      << b_expr << ":\n" << to_hex_dds_string(b.data, b.size, '\n', 8);
}

template<typename Type>
void serializer_test(const Encoding& encoding, const std::string expected_cdr)
{
  ACE_Message_Block mb(1024);
  Type value;
  set_values(value);

  // Serialize and Compare CDR
  {
    Serializer serializer(&mb, encoding);
    EXPECT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(AssertDataView, mb, expected_cdr);
  }

  // Deserialize and Compare C++ Values
  {
    Serializer serializer(&mb, encoding);
    Type result;
    EXPECT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(AssertValues<Type>, value, result);
  }
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

// FinalXcdr1Struct ----------------------------------------------------------

TEST(FinalXcdr2Struct_tests, serialized_size_test)
{
  FinalXcdr2Struct value;
  EXPECT_EQ(serialized_size(xcdr2, value), xcdr2_final_expected.size());
}

TEST(FinalXcdr2Struct_tests, serializer_test)
{
  serializer_test<FinalXcdr2Struct>(xcdr2, xcdr2_final_expected);
}

// AppendableXcdr1Struct -----------------------------------------------------

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

// MutableXcdr1Struct --------------------------------------------------------

const std::string xcdr2_mutable_expected(
  // Delimiter
  "\x00\x00\x00\x24" // +4 = 4
  // short_field
  "\x00\x00\x00\x00" // +4 = 8
  "\x7f\xff" // +2 = 10
  // long_field
  "\x00\x00" // +2 pad = 12
  "\x00\x00\x00\x01" // +4 = 16
  "\x7f\xff\xff\xff" // +4 = 20
  // octet_field
  "\x00\x00\x00\x02" // +4 = 24
  "\x01" // +1 = 25
  // long_long_field
  "\x00\x00\x00" // +3 pad = 28
  "\x00\x00\x00\x03" // +4 = 32
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

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
