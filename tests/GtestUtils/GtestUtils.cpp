#include "GtestUtils.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <string>
#include <sstream>

::testing::AssertionResult retcodes_equal(
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

::testing::AssertionResult data_views_equal(
  const char* a_expr, const char* b_expr,
  const DataView& a, const DataView& b)
{
  using OpenDDS::DCPS::to_hex_dds_string;

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
