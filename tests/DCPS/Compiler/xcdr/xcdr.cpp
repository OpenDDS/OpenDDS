#include "xcdrTypeSupportImpl.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

#include <string>
#include <cstring>

using namespace OpenDDS::DCPS;

const Encoding xcdr1(Encoding::KIND_XCDR1, ENDIAN_BIG);
const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);

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

// XCDR1 =====================================================================

const std::string xcdr1_non_mutable_expected(
  "\x7f\xff\x00\x00\x7f\xff\xff\xff"
  "\x01\x00\x00\x00\x00\x00\x00\x00"
  "\x7f\xff\xff\xff\xff\xff\xff\xff", 24);

// FinalXcdr1Type ------------------------------------------------------------
TEST(FinalXcdr1Type_tests, serialized_size_test)
{
  FinalXcdr1Type value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
}

TEST(FinalXcdr1Type_tests, serializer_test)
{
  ACE_Message_Block mb(1024);
  FinalXcdr1Type value;

  // Serialize
  {
    Serializer serializer(&mb, xcdr1);
    set_values(value);
    EXPECT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(AssertDataView, mb, xcdr1_non_mutable_expected);
  }

  // Deserialize
  {
    Serializer serializer(&mb, xcdr1);
    FinalXcdr1Type result;
    EXPECT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(AssertValues<FinalXcdr1Type>, value, result);
  }
}

// AppendableXcdr1Type -------------------------------------------------------
TEST(AppendableXcdr1Type_tests, serialized_size_test)
{
  AppendableXcdr1Type value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_non_mutable_expected.size());
}

TEST(AppendableXcdr1Type_tests, serializer_test)
{
  ACE_Message_Block mb(1024);
  AppendableXcdr1Type value;

  // Serialize
  {
    Serializer serializer(&mb, xcdr1);
    set_values(value);
    EXPECT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(AssertDataView, mb, xcdr1_non_mutable_expected);
  }

  // Deserialize
  {
    Serializer serializer(&mb, xcdr1);
    AppendableXcdr1Type result;
    EXPECT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(AssertValues<AppendableXcdr1Type>, value, result);
  }
}

// MutableXcdr1Type ----------------------------------------------------------
const std::string xcdr1_mutable_expected(
  // short_field
  "\xc0\x00\x00\x02" // PID +4 = 4
  "\x7f\xff" // +2 = 6
  "\x00\x00" // +2 pad = 8
  // long_field
  "\xc0\x01\x00\x04" // PID +4 = 12
  "\x7f\xff\xff\xff" // +4 = 16
  // octet_field
  "\xc0\x02\x00\x01" // PID +4 = 20
  "\x01" // +1 = 21
  "\x00\x00\x00" // +3 = 24
  // long_long_field
  "\xc0\x03\x00\x08" // PID +4 = 28
  "\x7f\xff\xff\xff\xff\xff\xff\xff" // +8 = 36
  // Sentinel
  "\x3f\x02\x00\x00" // +4 = 40
  , 40);

TEST(MutableXcdr1Type_tests, serialized_size_test)
{
  MutableXcdr1Type value;
  EXPECT_EQ(serialized_size(xcdr1, value), xcdr1_mutable_expected.size());
}

TEST(MutableXcdr1Type_tests, serializer_test)
{
  ACE_Message_Block mb(1024);
  MutableXcdr1Type value;

  // Serialize
  {
    Serializer serializer(&mb, xcdr1);
    set_values(value);
    EXPECT_TRUE(serializer << value);
    EXPECT_PRED_FORMAT2(AssertDataView, mb, xcdr1_mutable_expected);
  }

  // Deserialize
  {
    Serializer serializer(&mb, xcdr1);
    MutableXcdr1Type result;
    EXPECT_TRUE(serializer >> result);
    EXPECT_PRED_FORMAT2(AssertValues<MutableXcdr1Type>, value, result);
  }
}

#if 0
// XCDR2 =====================================================================

// FinalXcdr1Type ------------------------------------------------------------
TEST(FinalXcdr2Type_tests, serialized_size_test)
{
  FinalXcdr2Type value;
  EXPECT_EQ(serialized_size(xcdr2, value), size_t(20));
}

// AppendableXcdr1Type -------------------------------------------------------
TEST(AppendableXcdr2Type_tests, serialized_size_test)
{
  AppendableXcdr2Type value;
  EXPECT_EQ(serialized_size(xcdr2, value), size_t(24));
}

// MutableXcdr1Type ----------------------------------------------------------
TEST(MutableXcdr2Type_tests, serialized_size_test)
{
  MutableXcdr2Type value;
  EXPECT_EQ(serialized_size(xcdr2, value), size_t(56));
}
#endif

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
