#include <dds/DCPS/InternalStatistics.h>

#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_InternalStatistics, key_compare)
{
  typedef InternalTraits<InternalStatistics>::KeyCompare Comparator;
  const Comparator cmp = Comparator();

  const InternalStatisticSeq empty, nonempty(1, 1, InternalStatisticSeq::allocbuf(1), true);
  const InternalStatistics is1 = {"a", empty}, is2 = {"a", nonempty}, is3 = {"b", empty};

  EXPECT_TRUE(cmp(is1, is3));
  EXPECT_TRUE(cmp(is2, is3));
  EXPECT_FALSE(cmp(is1, is2));
  EXPECT_FALSE(cmp(is2, is1));
  EXPECT_FALSE(cmp(is3, is1));
  EXPECT_FALSE(cmp(is3, is2));
}
