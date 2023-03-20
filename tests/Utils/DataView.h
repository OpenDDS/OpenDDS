#ifndef TESTUTILS_DATAVIEW_H
#define TESTUTILS_DATAVIEW_H

#include <dds/DCPS/SafetyProfileStreams.h>
#include <ace/Message_Block.h>

#include <gtest/gtest.h>

#include <vector>
#include <cstring>
#include <string>
#include <sstream>

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

bool operator==(const DataView& a, const DataView& b)
{
  return a.size == b.size ? !std::memcmp(a.data, b.data, a.size) : false;
}

::testing::AssertionResult assert_DataView(const char* a_expr, const char* b_expr,
  const DataView& a, const DataView& b)
{
  using namespace OpenDDS::DCPS;
  if (a == b) {
    return ::testing::AssertionSuccess();
  }

  std::string a_line;
  // Use c_str so that it can cast to std::string on Safety Profile
  std::stringstream a_strm(to_hex_dds_string(a.data, a.size, '\n', 8).c_str());
  std::string b_line;
  std::stringstream b_strm(to_hex_dds_string(b.data, b.size, '\n', 8).c_str());
  std::stringstream result;
  bool got_a = true;
  bool got_b = true;
  while (got_a || got_b) {
    std::stringstream line;
    line.width(16);
    line << std::left;
    if (got_a) {
      if (std::getline(a_strm, a_line, '\n')) {
        line << a_line;
      } else {
        got_a = false;
        line << "(END OF LEFT)";
      }
    } else {
      line << ' ';
    }
    line << ' ';
    line.width(16);
    line << std::left;
    if (got_b) {
      if (std::getline(b_strm, b_line, '\n')) {
        line << b_line;
      } else {
        got_b = false;
        line << "(END OF RIGHT)";
      }
    } else {
      line << ' ';
    }
    if ((got_a || got_b) && a_line != b_line) {
      line << " <- This line is different";
    }
    const std::string line_str = line.str();
    if (line_str.find_first_not_of(" ") != std::string::npos) {
      result << line_str << std::endl;
    }
  }
  return ::testing::AssertionFailure()
    << a_expr << " (size " << a.size << ") isn't the same as "
    << b_expr << " (size " << b.size << ").\n"
    << a_expr << " is on the left, " << b_expr << " is on the right:\n"
    << result.str();
}

#define EXPECT_DATA_EQ(A, B) EXPECT_PRED_FORMAT2(assert_DataView, (A), (B))
#define ASSERT_DATA_EQ(A, B) ASSERT_PRED_FORMAT2(assert_DataView, (A), (B))

#endif // TESTUTILS_DATAVIEW_H
