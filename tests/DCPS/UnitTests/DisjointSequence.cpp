/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include "dds/DCPS/DisjointSequence.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Construction (default)
  {
    DisjointSequence sequence;

    // ASSERT initial value is a default SequenceNumber:
    SequenceNumber default_value = sequence;
    TEST_CHECK(default_value == SequenceNumber());

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT depth is 1:
    TEST_CHECK(sequence.depth() == 1);

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Construction (with value)
  {
    DisjointSequence sequence(10);

    // ASSERT initial value is correct:
    SequenceNumber initial_value = sequence;
    TEST_CHECK(initial_value == SequenceNumber(10));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT depth is 1:
    TEST_CHECK(sequence.depth() == 1);

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Skipping values
  {
    DisjointSequence sequence(0);
    sequence.update(5);   // discontiguous
    sequence.update(10);  // discontiguous

    sequence.skip(20);

    // ASSERT skip value is correct:
    SequenceNumber skip_value = sequence;
    TEST_CHECK(skip_value == SequenceNumber(20));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT depth is 1:
    TEST_CHECK(sequence.depth() == 1);

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Updating values
  {
    DisjointSequence sequence;
    SequenceNumber low;

    // ASSERT update of low + 1 updates the low water mark:
    sequence = DisjointSequence(0);
    sequence.update(sequence.low() + 1);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.depth() == 1);

    // ASSERT update of value > low + 1 creates discontiguity:
    sequence = DisjointSequence(0);
    sequence.update(sequence.low() + 5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 6);

    // ASSERT update of low + 1 updates the low water mark and
    //        normalizes any new contiguities:
    sequence = DisjointSequence(0);
    sequence.update(2);
    sequence.update(3);
    sequence.update(4);
    sequence.update(5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 6);

    sequence.update(1);

    TEST_CHECK(sequence.low() == SequenceNumber(5));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.depth() == 1);

    // ASSERT update of low + 1 updates the low water mark
    //        and preserves existing discontiguities:
    sequence = DisjointSequence(0);
    sequence.update(2);
    sequence.update(4); // discontiguity
    sequence.update(5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 6);

    sequence.update(1);

    TEST_CHECK(sequence.low() == SequenceNumber(2));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 4);
  }

  return 0;
}
