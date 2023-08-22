#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_SafetyProfileStreams, to_dds_string_double)
{
  EXPECT_EQ(to_dds_string(1.5), "1.5");
}
