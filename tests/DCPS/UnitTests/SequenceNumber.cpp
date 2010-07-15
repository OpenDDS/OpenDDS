/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Definitions.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

namespace {
  const SequenceNumber::Value POSITIVE_RANGE = ACE_INT32_MAX;
  const SequenceNumber::Value MAX_POSITIVE = ACE_INT32_MAX-1;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Construction (default)
  TEST_CHECK(SequenceNumber(ACE_INT32_MIN) == SequenceNumber());

  // testing numerical sequence
  TEST_CHECK(SequenceNumber(ACE_INT32_MIN) < SequenceNumber(ACE_INT32_MIN+1));
  TEST_CHECK(!(SequenceNumber(ACE_INT32_MIN+1) < SequenceNumber(ACE_INT32_MIN)));
  TEST_CHECK(SequenceNumber(-1) < SequenceNumber(0));
  TEST_CHECK(!(SequenceNumber(0) < SequenceNumber(-1)));
  TEST_CHECK(SequenceNumber(0) < SequenceNumber(1));
  TEST_CHECK(!(SequenceNumber(1) < SequenceNumber(0)));
  TEST_CHECK(SequenceNumber(MAX_POSITIVE-1) < SequenceNumber(MAX_POSITIVE));
  TEST_CHECK(!(SequenceNumber(MAX_POSITIVE) < SequenceNumber(MAX_POSITIVE-1)));

  // testing wide ranges
  TEST_CHECK(SequenceNumber(ACE_INT32_MIN) < SequenceNumber(0));
  TEST_CHECK(!(SequenceNumber(0) < SequenceNumber(ACE_INT32_MIN)));
  TEST_CHECK(SequenceNumber(0) < SequenceNumber(POSITIVE_RANGE/2));
  TEST_CHECK(!(SequenceNumber(POSITIVE_RANGE/2) < SequenceNumber(0)));
  TEST_CHECK(SequenceNumber(POSITIVE_RANGE/2+1) < SequenceNumber(0));
  TEST_CHECK(!(SequenceNumber(0) < SequenceNumber(POSITIVE_RANGE/2+1)));
  TEST_CHECK(SequenceNumber(POSITIVE_RANGE/2) < SequenceNumber(MAX_POSITIVE));
  TEST_CHECK(!(SequenceNumber(MAX_POSITIVE) < SequenceNumber(POSITIVE_RANGE/2)));
  TEST_CHECK(SequenceNumber(MAX_POSITIVE) < SequenceNumber(POSITIVE_RANGE/2-1));
  TEST_CHECK(!(SequenceNumber(POSITIVE_RANGE/2-1) < SequenceNumber(MAX_POSITIVE)));
  TEST_CHECK(SequenceNumber(MAX_POSITIVE) < SequenceNumber(0));
  TEST_CHECK(!(SequenceNumber(0) < SequenceNumber(MAX_POSITIVE)));

  // testing values and increment operator
  {
    SequenceNumber num(ACE_INT32_MIN);
    TEST_CHECK(num.getValue() == ACE_INT32_MIN);
    TEST_CHECK((++num).getValue() == ACE_INT32_MIN+1);
  }

  {
    SequenceNumber num(-1);
    TEST_CHECK(num.getValue() == -1);
    TEST_CHECK((++num).getValue() == 0);
    TEST_CHECK((++num).getValue() == 1);
  }

  {
    SequenceNumber num(MAX_POSITIVE);
    TEST_CHECK(num.getValue() == MAX_POSITIVE);
    TEST_CHECK((++num).getValue() == 0);
    // test post-incrementer
    TEST_CHECK((num++).getValue() == 0);
  }

  // testing beyond range
  TEST_CHECK(SequenceNumber(ACE_INT64(MAX_POSITIVE)+1).getValue() == 0);
  TEST_CHECK(SequenceNumber(ACE_INT64(MAX_POSITIVE)+ACE_INT32_MAX).getValue() == MAX_POSITIVE);

  return 0;
}
