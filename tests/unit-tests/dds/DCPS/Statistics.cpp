#include <dds/DCPS/Statistics.h>

#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_Statistics, key_compare)
{
  typedef InternalTraits<Statistics>::KeyCompare Comparator;
  const Comparator cmp = Comparator();

  const StatisticSeq empty, nonempty(1, 1, StatisticSeq::allocbuf(1), true);
  const Statistics is1 = {"a", empty}, is2 = {"a", nonempty}, is3 = {"b", empty};

  EXPECT_TRUE(cmp(is1, is3));
  EXPECT_TRUE(cmp(is2, is3));
  EXPECT_FALSE(cmp(is1, is2));
  EXPECT_FALSE(cmp(is2, is1));
  EXPECT_FALSE(cmp(is3, is1));
  EXPECT_FALSE(cmp(is3, is2));
}
