/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Definitions.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

namespace {
  const SequenceNumber::Value SN_MAX   = SequenceNumber::MAX_VALUE;
  const SequenceNumber::Value SN_MIN   = SequenceNumber::MIN_VALUE;
  const SequenceNumber::Value SN_SEAM  = ACE_INT32_MAX;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Construction (default)
  TEST_CHECK(SequenceNumber(SN_MIN) == SequenceNumber());

  TEST_CHECK(SequenceNumber::ZERO().getValue() == 0);
  TEST_CHECK(SequenceNumber::ZERO() < SequenceNumber());
  TEST_CHECK(++SequenceNumber(SequenceNumber::ZERO()) == SequenceNumber());

  // testing numerical sequence
  TEST_CHECK(SequenceNumber(SN_MIN) < SequenceNumber(SN_MIN+1));
  TEST_CHECK(!(SequenceNumber(SN_MIN+1) < SequenceNumber(SN_MIN)));
  TEST_CHECK(SequenceNumber(SN_SEAM) < SequenceNumber(SN_SEAM+1));
  TEST_CHECK(!(SequenceNumber(SN_SEAM+1) < SequenceNumber(SN_SEAM)));
  TEST_CHECK(SequenceNumber(SN_MAX-1) < SequenceNumber(SN_MAX));
  TEST_CHECK(!(SequenceNumber(SN_MAX) < SequenceNumber(SN_MAX-1)));

  // testing values and increment operator
  {
    SequenceNumber num(SN_MIN);
    TEST_CHECK(num.getValue() == SN_MIN);
    TEST_CHECK((++num).getValue() == SN_MIN+1);
  }

  {
    SequenceNumber num(SN_SEAM);
    TEST_CHECK(num.getValue() == SN_SEAM);
    TEST_CHECK((++num).getValue() == SN_SEAM+1);
    TEST_CHECK((++num).getValue() == SN_SEAM+2);
  }

  {
    SequenceNumber num(SN_MAX);
    TEST_CHECK(num.getValue() == SN_MAX);
    TEST_CHECK((++num).getValue() == SN_MIN);
    // test post-incrementer
    TEST_CHECK((num++).getValue() == SN_MIN);
    TEST_CHECK(num.getValue() == SN_MIN+1);
  }

  // Test SEQUENCENUMBER_UNKNOWN
  {
    SequenceNumber num = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
    TEST_CHECK(num.getValue() == ACE_INT64(0xffffffff) << 32);
    SequenceNumber min;
    TEST_CHECK(num != min);
    TEST_CHECK(num == SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  }

  // Test previous() member function
  {
    SequenceNumber num(SN_MIN);
    TEST_CHECK(num.previous() == SN_MAX);
  }

  {
    SequenceNumber num(SN_SEAM+1);
    TEST_CHECK(num.previous() == SN_SEAM);
  }

  {
    SequenceNumber num(99);
    TEST_CHECK(num.previous() == 98);
  }

  {
    SequenceNumber num(SN_MAX);
    TEST_CHECK(num.previous() == SN_MAX-1);
  }


  return 0;
}
