#ifndef OPENDDS_TEST_GTEST_RC_H
#define OPENDDS_TEST_GTEST_RC_H

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/LogAddr.h>

#include <gtest/gtest.h>

inline ::testing::AssertionResult retcodes_equal(
  const char* a_expr, const char* b_expr,
  DDS::ReturnCode_t a, DDS::ReturnCode_t b)
{
  if (a == b) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure() <<
    "Expected equality of these values:\n"
    "  " << a_expr << "\n"
    "    Which is: " << OpenDDS::DCPS::retcode_to_string(a) << "\n"
    "  " << b_expr << "\n"
    "    Which is: " << OpenDDS::DCPS::retcode_to_string(b) << "\n";
}

#define EXPECT_RC_EQ(A, B) EXPECT_PRED_FORMAT2(retcodes_equal, A, B)
#define ASSERT_RC_EQ(A, B) ASSERT_PRED_FORMAT2(retcodes_equal, A, B)
#define EXPECT_RC_OK(VALUE) EXPECT_RC_EQ(::DDS::RETCODE_OK, VALUE)
#define ASSERT_RC_OK(VALUE) ASSERT_RC_EQ(::DDS::RETCODE_OK, VALUE)

inline ::testing::AssertionResult addr_equal(
  const char* a_expr, const char* b_expr,
  const OpenDDS::DCPS::NetworkAddress& a, const OpenDDS::DCPS::NetworkAddress& b)
{
  const OpenDDS::DCPS::LogAddr ala(a);
  const OpenDDS::DCPS::LogAddr bla(b);
  if (ala.str() == bla.str()) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure() <<
    "Expected equality of these values:\n"
    "  " << a_expr << "\n"
    "    Which is: " << ala.str() << "\n"
    "  " << b_expr << "\n"
    "    Which is: " << bla.str() << "\n";
}

#define EXPECT_ADDR_EQ(A, B) EXPECT_PRED_FORMAT2(addr_equal, A, B)
#define ASSERT_ADDR_EQ(A, B) ASSERT_PRED_FORMAT2(addr_equal, A, B)

#endif
