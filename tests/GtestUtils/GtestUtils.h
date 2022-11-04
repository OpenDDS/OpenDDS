#ifndef OPENDDS_GTEST_UTILS
#define OPENDDS_GTEST_UTILS

#include "GtestUtilsExport.h"

#include <dds/DdsDcpsInfrastructureC.h>

#include <gtest/gtest.h>

#include <vector>
#include <cstring>

OPENDDS_GTEST_UTILS_Export ::testing::AssertionResult retcodes_equal(
  const char* a_expr, const char* b_expr,
  DDS::ReturnCode_t a, DDS::ReturnCode_t b);

#define EXPECT_RC_EQ(A, B) EXPECT_PRED_FORMAT2(retcodes_equal, (A), (B))
#define ASSERT_RC_EQ(A, B) ASSERT_PRED_FORMAT2(retcodes_equal, (A), (B))
#define EXPECT_RC_OK(VALUE) EXPECT_RC_EQ(::DDS::RETCODE_OK, (VALUE))
#define ASSERT_RC_OK(VALUE) ASSERT_RC_EQ(::DDS::RETCODE_OK, (VALUE))

typedef std::vector<char> DataVec;

struct DataView {
  DataView()
  : data(0)
  , size(0)
  {
  }

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

  DataView(const DataVec& data_vec)
  : data(data_vec.size() ? &data_vec[0] : 0)
  , size(data_vec.size())
  {
  }

  void copy_to(DataVec& data_vec)
  {
    data_vec.reserve(data_vec.size() + size);
    for (size_t i = 0; i < size; ++i) {
      data_vec.push_back(data[i]);
    }
  }

  const char* data;
  size_t size;
};

inline bool operator==(const DataView& a, const DataView& b)
{
  return a.size == b.size && !std::memcmp(a.data, b.data, a.size);
}

OPENDDS_GTEST_UTILS_Export ::testing::AssertionResult data_views_equal(
  const char* a_expr, const char* b_expr,
  const DataView& a, const DataView& b);

#define EXPECT_DATA_EQ(A, B) EXPECT_PRED_FORMAT2(data_views_equal, (A), (B))
#define ASSERT_DATA_EQ(A, B) ASSERT_PRED_FORMAT2(data_views_equal, (A), (B))

#endif
