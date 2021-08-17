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

TEST(FibonacciSequence, size_t_test)
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

  fs.reset();

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

TEST(FibonacciSequence, TimeDuration_test)
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

  fs.reset();

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
