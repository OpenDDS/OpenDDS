/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/FibonacciSequence.h"
#include "dds/DCPS/TimeDuration.h"

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_FibonacciSequence, size_t_test)
{
  FibonacciSequence<size_t> fs(1u);

  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);

  fs.advance();
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);

  fs.advance();
  EXPECT_EQ(fs.get(), 2u);
  EXPECT_EQ(fs.get(), 2u);

  fs.advance();
  EXPECT_EQ(fs.get(), 3u);
  EXPECT_EQ(fs.get(), 3u);

  fs.advance();
  EXPECT_EQ(fs.get(), 5u);
  EXPECT_EQ(fs.get(), 5u);

  fs.advance();
  EXPECT_EQ(fs.get(), 8u);
  EXPECT_EQ(fs.get(), 8u);

  fs.set(1u);

  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);

  fs.advance();
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);
  EXPECT_EQ(fs.get(), 1u);

  fs.advance();
  EXPECT_EQ(fs.get(), 2u);
  EXPECT_EQ(fs.get(), 2u);

  fs.advance();
  EXPECT_EQ(fs.get(), 3u);
  EXPECT_EQ(fs.get(), 3u);

  fs.advance();
  EXPECT_EQ(fs.get(), 5u);
  EXPECT_EQ(fs.get(), 5u);

  fs.advance();
  EXPECT_EQ(fs.get(), 8u);
  EXPECT_EQ(fs.get(), 8u);
}

TEST(dds_DCPS_FibonacciSequence, TimeDuration_test)
{
  FibonacciSequence<TimeDuration> fs(TimeDuration(0, 500000));

  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(1, 0));
  EXPECT_EQ(fs.get(), TimeDuration(1, 0));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(1, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(1, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(2, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(2, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(4, 0));
  EXPECT_EQ(fs.get(), TimeDuration(4, 0));

  fs.set(TimeDuration(0, 500000));

  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(0, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(1, 0));
  EXPECT_EQ(fs.get(), TimeDuration(1, 0));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(1, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(1, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(2, 500000));
  EXPECT_EQ(fs.get(), TimeDuration(2, 500000));

  fs.advance();
  EXPECT_EQ(fs.get(), TimeDuration(4, 0));
  EXPECT_EQ(fs.get(), TimeDuration(4, 0));
}

TEST(dds_DCPS_FibonacciSequence, advance_with_max)
{
  FibonacciSequence<size_t> fs(1);

  EXPECT_EQ(fs.get(), 1u);
  fs.advance(3);
  EXPECT_EQ(fs.get(), 1u);
  fs.advance(3);
  EXPECT_EQ(fs.get(), 2u);
  fs.advance(3);
  EXPECT_EQ(fs.get(), 3u);
  fs.advance(3);
  EXPECT_EQ(fs.get(), 3u);
}

TEST(dds_DCPS_FibonacciSequence, set_with_value)
{
  FibonacciSequence<size_t> fs(1);

  EXPECT_EQ(fs.get(), 1u);
  fs.set(3);
  EXPECT_EQ(fs.get(), 3u);
  fs.advance();
  EXPECT_EQ(fs.get(), 3u);
  fs.advance();
  EXPECT_EQ(fs.get(), 6u);
}

TEST(dds_DCPS_FibonacciSequence, set_with_two_values)
{
  FibonacciSequence<size_t> fs(1);

  EXPECT_EQ(fs.get(), 1u);
  fs.set(5, 2);
  EXPECT_EQ(fs.get(), 5u);
  fs.advance();
  EXPECT_EQ(fs.get(), 7u);
  fs.advance();
  EXPECT_EQ(fs.get(), 12u);
}
