/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/SequenceNumber.h"

using namespace OpenDDS::DCPS;

namespace {
  const SequenceNumber::Value INITIAL  = SequenceNumber::INITIAL_VALUE;
  const SequenceNumber::Value SN_MAX   = SequenceNumber::MAX_VALUE;
  const SequenceNumber::Value SN_MIN   = SequenceNumber::MIN_VALUE;
  const SequenceNumber::Value SN_SEAM  = ACE_UINT32_MAX;
}

TEST(SequenceNumber, maintest)
{
  // Construction (default)
  EXPECT_TRUE(SequenceNumber(INITIAL) == SequenceNumber());

  EXPECT_TRUE(SequenceNumber::ZERO().getValue() == 0);
  EXPECT_TRUE(SequenceNumber::ZERO() < SequenceNumber());
  EXPECT_TRUE(++SequenceNumber(SequenceNumber::ZERO()) == SequenceNumber());

  // testing numerical sequence
  EXPECT_TRUE(SequenceNumber(SN_MIN) < SequenceNumber(SN_MIN+1));
  EXPECT_TRUE(!(SequenceNumber(SN_MIN+1) < SequenceNumber(SN_MIN)));
  EXPECT_TRUE(SequenceNumber(SN_SEAM) < SequenceNumber(SN_SEAM+1));
  EXPECT_TRUE(!(SequenceNumber(SN_SEAM+1) < SequenceNumber(SN_SEAM)));
  EXPECT_TRUE(SequenceNumber(SN_MAX-1) < SequenceNumber(SN_MAX));
  EXPECT_TRUE(!(SequenceNumber(SN_MAX) < SequenceNumber(SN_MAX-1)));

  // testing values and increment operator
  {
    SequenceNumber num(SN_MIN);
    EXPECT_TRUE(num.getValue() == SN_MIN);
    EXPECT_TRUE((++num).getValue() == SN_MIN+1);
  }

  {
    SequenceNumber num(SN_SEAM);
    EXPECT_TRUE(num.getValue() == SN_SEAM);
    EXPECT_TRUE((++num).getValue() == SN_SEAM+1);
    EXPECT_TRUE((++num).getValue() == SN_SEAM+2);
  }

  {
    SequenceNumber num(SN_MAX);
    EXPECT_TRUE(num.getValue() == SN_MAX);
    EXPECT_TRUE((++num).getValue() == INITIAL);
    // test post-incrementer
    EXPECT_TRUE((num++).getValue() == INITIAL);
    EXPECT_TRUE(num.getValue() == INITIAL+1);
  }

  // Test SEQUENCENUMBER_UNKNOWN
  {
    SequenceNumber num = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
    EXPECT_TRUE(num.getValue() == ACE_INT64(0xffffffff) << 32);
    SequenceNumber min;
    EXPECT_TRUE(num != min);
    EXPECT_TRUE(num == SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  }

  // Test previous() member function
  {
    SequenceNumber num(SN_MIN);
    EXPECT_TRUE(num.previous() == SN_MAX);
  }

  {
    SequenceNumber num(SN_SEAM+1);
    EXPECT_TRUE(num.previous() == SN_SEAM);
  }

  {
    SequenceNumber num(99);
    EXPECT_TRUE(num.previous() == 98);
  }

  {
    SequenceNumber num(SN_MAX);
    EXPECT_TRUE(num.previous() == SN_MAX-1);
  }
}
