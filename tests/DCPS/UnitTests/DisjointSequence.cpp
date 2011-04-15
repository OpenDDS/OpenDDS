/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/DisjointSequence.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

namespace {
  const SequenceNumber::Value SN_MAX   = SequenceNumber::MAX_VALUE;
  const SequenceNumber::Value SN_MIN   = SequenceNumber::MIN_VALUE;
  const SequenceNumber::Value SN_RANGE = SN_MAX-SN_MIN;
  const SequenceNumber::Value SN_SEAM  = ACE_UINT32_MAX;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Construction (default)
  {
    TEST_CHECK(SequenceNumber(SN_MIN) == SequenceNumber());

    DisjointSequence sequence;

    // ASSERT initial value is a default SequenceNumber:
    SequenceNumber default_value = sequence.low();
    TEST_CHECK(default_value == SequenceNumber());

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Construction (with value)
  {
    DisjointSequence sequence(10);

    // ASSERT initial value is correct:
    SequenceNumber initial_value = sequence.low();
    TEST_CHECK(initial_value == SequenceNumber(10));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

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
    SequenceNumber reset_value = sequence.low();
    TEST_CHECK(reset_value == SequenceNumber(20));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT sequence is not disjointed:
    TEST_CHECK(!sequence.disjoint());
  }

  // Updating values
  {
    DisjointSequence sequence;

    // ASSERT update of low + 1 updates the low water mark:
    sequence = DisjointSequence(1);
    sequence.update(sequence.low() + 1);

    TEST_CHECK(sequence.low() == SequenceNumber(2));
    TEST_CHECK(sequence.low() == sequence.high());
    TEST_CHECK(!sequence.disjoint());

    // ASSERT update of value > low + 1 creates discontiguity:
    sequence = DisjointSequence(1);
    sequence.update(sequence.low() + 5);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    // ASSERT update of low + 1 updates the low water mark
    //        and normalizes new contiguities:
    sequence = DisjointSequence(1);
    sequence.update(3); // discontiguity
    sequence.update(4);
    sequence.update(5);
    sequence.update(6);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    sequence.update(2);

    TEST_CHECK(sequence.low() == SequenceNumber(6));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(!sequence.disjoint());

    // ASSERT update of low + 1 updates the low water mark
    //        and preserves existing discontiguities:
    sequence = DisjointSequence(1);
    sequence.update(3);
    sequence.update(5); // discontiguity
    sequence.update(6);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    sequence.update(2);

    TEST_CHECK(sequence.low() == SequenceNumber(3));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    // ASSERT update of range from low +1 updates the
    //        low water mark and preserves existing
    //        discontiguities:
    sequence = DisjointSequence(1);
    sequence.update(4);
    sequence.update(6); // discontiguity
    sequence.update(7);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(7));
    TEST_CHECK(sequence.disjoint());

    sequence.update(SequenceRange(2, 3));

    TEST_CHECK(sequence.low() == SequenceNumber(4));
    TEST_CHECK(sequence.high() == SequenceNumber(7));
    TEST_CHECK(sequence.disjoint());
  }

  // invalid set of SequenceNumbers
  {
    DisjointSequence sequence(1);
    sequence.update(3);

    try {
      // Should throw because of invalid range
      sequence.update(SequenceRange(50,40));
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    // Carefully pick values such that:
    //   first < second
    //   second < third
    //   third < first
    SequenceNumber first(SN_SEAM+1);
    SequenceNumber second(SN_RANGE/2+1);
    SequenceNumber third(1);
    sequence = DisjointSequence(first);
    sequence.update(second);
    // Validate that update worked
    TEST_CHECK(sequence.low() == first);
    TEST_CHECK(sequence.high() == second);

    try {
      // Should throw because new SN > high and < low
      sequence.update(third);
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(1);
    sequence.update(3);

    try {
      sequence.update(SequenceRange(SN_RANGE/2, SN_MAX));
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(0);

    try {
      sequence.update(SequenceRange(4, 3));
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

    sequence = DisjointSequence(first);
    sequence.update(second);

    try {
      sequence.lowest_valid(third);
      TEST_CHECK(false);
    } catch (const std::exception& ) {
    }

  }

  // Range iterator
  {
    DisjointSequence sequence;
    SequenceRange range;

    // ASSERT a single dicontiguity returns a single range
    //        of values: <low + 1, high - 1>
    sequence = DisjointSequence(1);
    sequence.update(6); // discontiguity

    std::vector<SequenceRange> missingSet = sequence.missing_sequence_ranges();
    std::vector<SequenceRange>::const_iterator it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it == missingSet.end());

    // ASSERT multiple contiguities return multiple ranges
    //        of values:
    sequence = DisjointSequence(1);
    sequence.update(6);   // discontiguity
    sequence.update(7);
    sequence.update(11);  // discontiguity

    missingSet = sequence.missing_sequence_ranges();
    it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(8));
    TEST_CHECK(range.second == SequenceNumber(10));
    TEST_CHECK(++it == missingSet.end());

    // ASSERT multiple contiguities return  multiple ranges
    //        of values with a difference of one:
    sequence = DisjointSequence(SN_MIN);
    sequence.update(3);  // discontiguity
    sequence.update(6);  // discontiguity
    sequence.update(8);  // discontiguity
    sequence.update(10);  // discontiguity

    missingSet = sequence.missing_sequence_ranges();
    it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(SN_MIN + 1));
    TEST_CHECK(range.second == SequenceNumber(2));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(4));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(7));
    TEST_CHECK(range.second == SequenceNumber(7));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(9));
    TEST_CHECK(range.second == SequenceNumber(9));
    TEST_CHECK(++it == missingSet.end());


    // ASSERT rollover ranges return proper missing sequences:
    sequence = DisjointSequence(SN_MAX-2);
    sequence.update(1);   // discontiguity
    sequence.update(6);   // discontiguity
    sequence.update(10);  // discontiguity

    missingSet = sequence.missing_sequence_ranges();
    it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(SN_MAX-1));
    TEST_CHECK(range.second == SequenceNumber(SN_MAX));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it != missingSet.end());

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(7));
    TEST_CHECK(range.second == SequenceNumber(9));
    TEST_CHECK(++it == missingSet.end());

    // ASSERT a range update returns the proper iterators
    sequence = DisjointSequence(1);
    sequence.update(SequenceRange(3, 5)); // discontiguity
    sequence.update(6);

    missingSet = sequence.missing_sequence_ranges();
    it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(2));
    TEST_CHECK(++it == missingSet.end());
  }

  return 0;
}
