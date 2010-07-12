/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/DisjointSequence.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

namespace {
  const ACE_INT16 POSITIVE_RANGE = ACE_INT16_MAX;
  const ACE_INT16 MAX_POSITIVE = ACE_INT16_MAX-1;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Construction (default)
  {
    TEST_CHECK(SequenceNumber(ACE_INT16_MIN) == SequenceNumber());

    DisjointSequence sequence;

    // ASSERT initial value is a default SequenceNumber:
    SequenceNumber default_value = sequence;
    TEST_CHECK(default_value == SequenceNumber());

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT depth is 0:
    TEST_CHECK(sequence.depth() == 0);

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

    // ASSERT depth is 0:
    TEST_CHECK(sequence.depth() == 0);

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Skipping values
  {
    DisjointSequence sequence(0);
    sequence.update(5);   // discontiguous
    sequence.update(10);  // discontiguous

    sequence.reset(20);

    // ASSERT reset value is correct:
    SequenceNumber reset_value = sequence;
    TEST_CHECK(reset_value == SequenceNumber(20));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT depth is 0:
    TEST_CHECK(sequence.depth() == 0);

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
    TEST_CHECK(sequence.depth() == 0);

    // ASSERT update of value > low + 1 creates discontiguity:
    sequence = DisjointSequence(0);
    sequence.update(sequence.low() + 5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 5);

    // ASSERT update of low + 1 updates the low water mark
    //        and normalizes new contiguities:
    sequence = DisjointSequence(0);
    sequence.update(2); // discontiguity
    sequence.update(3);
    sequence.update(4);
    sequence.update(5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 5);

    sequence.update(1);

    TEST_CHECK(sequence.low() == SequenceNumber(5));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.depth() == 0);

    // ASSERT update of low + 1 updates the low water mark
    //        and preserves existing discontiguities:
    sequence = DisjointSequence(0);
    sequence.update(2);
    sequence.update(4); // discontiguity
    sequence.update(5);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 5);
  
    sequence.update(1);

    TEST_CHECK(sequence.low() == SequenceNumber(2));
    TEST_CHECK(sequence.high() == SequenceNumber(5));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 3);

    // ASSERT update of range from low +1 updates the 
    //        low water mark and preserves existing 
    //        discontiguities:
    sequence = DisjointSequence(0);
    sequence.update(3);
    sequence.update(5); // discontiguity
    sequence.update(6);

    TEST_CHECK(sequence.low() == SequenceNumber(0));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 6);
  
    sequence.update(SequenceRange(1, 2));

    TEST_CHECK(sequence.low() == SequenceNumber(3));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.depth() == 3);
  }

  // invalid set of SequenceNumbers
  {
    DisjointSequence sequence(0);
    sequence.update(2);

    try {
      sequence.update(POSITIVE_RANGE/2+1);
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(1);
    sequence.update(POSITIVE_RANGE/2+1);

    try {
      sequence.update(0);
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(0);
    sequence.update(2);

    try {
      sequence.update(SequenceRange(POSITIVE_RANGE/2,MAX_POSITIVE));
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(0);

    try {
      sequence.update(SequenceRange(4, 3));
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(1);
    sequence.update(POSITIVE_RANGE/2+1);

    try {
      sequence.shift(0);
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

  }

  // Range iterator
  {
    DisjointSequence sequence;
    DisjointSequence::range_iterator it;
    DisjointSequence::range_iterator end;
    SequenceRange range;

    // ASSERT a single dicontiguity returns a single range
    //        of values: <low + 1, high - 1>
    sequence = DisjointSequence(0);
    sequence.update(5); // discontiguity

    it = sequence.range_begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(1));
    TEST_CHECK(range.second == SequenceNumber(4));
    TEST_CHECK(++it == sequence.range_end());

    // ASSERT multiple contiguities return multiple ranges
    //        of values:
    sequence = DisjointSequence(0);
    sequence.update(5);   // discontiguity
    sequence.update(6);
    sequence.update(10);  // discontiguity

    it = sequence.range_begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(1));
    TEST_CHECK(range.second == SequenceNumber(4));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(7));
    TEST_CHECK(range.second == SequenceNumber(9));
    TEST_CHECK(++it == sequence.range_end());

    // ASSERT multiple contiguities return  multiple ranges
    //        of values with a difference of one:
    sequence = DisjointSequence(ACE_INT16_MIN);
    sequence.update(0);  // discontiguity
    sequence.update(5);  // discontiguity
    sequence.update(7);  // discontiguity
    sequence.update(9);  // discontiguity

    it = sequence.range_begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(ACE_INT16_MIN + 1));
    TEST_CHECK(range.second == SequenceNumber(-1));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(1));
    TEST_CHECK(range.second == SequenceNumber(4));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(6));
    TEST_CHECK(range.second == SequenceNumber(6));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(8));
    TEST_CHECK(range.second == SequenceNumber(8));
    TEST_CHECK(++it == sequence.range_end());


    // ASSERT rollover ranges return proper missing sequences:
    sequence = DisjointSequence(MAX_POSITIVE-2);
    sequence.update(0);   // discontiguity
    sequence.update(6);   // discontiguity
    sequence.update(10);  // discontiguity

    it = sequence.range_begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(MAX_POSITIVE-1));
    TEST_CHECK(range.second == SequenceNumber(MAX_POSITIVE));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(1));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it != sequence.range_end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(7));
    TEST_CHECK(range.second == SequenceNumber(9));
    TEST_CHECK(++it == sequence.range_end());

    // ASSERT a range update returns the proper iterators
    sequence = DisjointSequence(0);
    sequence.update(SequenceRange(3, 5)); // discontiguity
    sequence.update(6);

    it = sequence.range_begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(1));
    TEST_CHECK(range.second == SequenceNumber(2));
    TEST_CHECK(++it == sequence.range_end());
  }

  return 0;
}
