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

    // ASSERT initial value is empty:
    TEST_CHECK(sequence.empty());

    // ASSERT sequence is not disjoint:
    TEST_CHECK(!sequence.disjoint());
  }

  // Insert single value
  {
    DisjointSequence sequence;
    sequence.insert(10);

    // ASSERT initial value is correct:
    SequenceNumber initial_value = sequence.low();
    TEST_CHECK(initial_value == SequenceNumber(10));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT sequence is not disjoint:
    TEST_CHECK(!sequence.disjoint());

    // ASSERT sequence is not empty:
    TEST_CHECK(!sequence.empty());
  }

  // Skipping values
  {
    DisjointSequence sequence;
    sequence.insert(0);
    sequence.insert(5);   // discontiguous
    sequence.insert(10);  // discontiguous

    sequence.reset();
    sequence.insert(20);

    // ASSERT reset value is correct:
    SequenceNumber reset_value = sequence.low();
    TEST_CHECK(reset_value == SequenceNumber(20));

    // ASSERT low and high water marks are the same:
    TEST_CHECK(sequence.low() == sequence.high());

    // ASSERT sequence is not disjoint:
    TEST_CHECK(!sequence.disjoint());
  }

  // Updating values
  {
    DisjointSequence sequence;

    // ASSERT insert of low + 1 updates the cumulative_ack:
    sequence.insert(1);
    sequence.insert(sequence.low() + 1);

    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(2));
    TEST_CHECK(sequence.cumulative_ack() == sequence.high());
    TEST_CHECK(!sequence.disjoint());

    // ASSERT insert of value > low + 1 creates discontiguity:
    sequence.reset();
    sequence.insert(1);
    sequence.insert(sequence.low() + 5);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    // ASSERT insert of low + 1 updates the cumulative_ack
    //        and normalizes new contiguities:
    sequence.reset();
    sequence.insert(1);
    sequence.insert(3); // discontiguity
    sequence.insert(4);
    sequence.insert(5);
    sequence.insert(6);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    sequence.insert(2);

    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(6));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(!sequence.disjoint());

    // ASSERT insert of low + 1 updates the cumulative_ack
    //        and preserves existing discontiguities:
    sequence.reset();
    sequence.insert(1);
    sequence.insert(3);
    sequence.insert(5); // discontiguity
    sequence.insert(6);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    sequence.insert(2);

    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(3));
    TEST_CHECK(sequence.high() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    // ASSERT insert of range from low + 1 updates the
    //        cumulative_ack and preserves existing
    //        discontiguities:
    sequence.reset();
    sequence.insert(1);
    sequence.insert(4);
    sequence.insert(6); // discontiguity
    sequence.insert(7);

    TEST_CHECK(sequence.low() == SequenceNumber(1));
    TEST_CHECK(sequence.high() == SequenceNumber(7));
    TEST_CHECK(sequence.disjoint());

    sequence.insert(SequenceRange(2, 3));

    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(4));
    TEST_CHECK(sequence.high() == SequenceNumber(7));
    TEST_CHECK(sequence.disjoint());
  }

  // invalid set of SequenceNumbers
  {
    DisjointSequence sequence;
    sequence.insert(1);
    sequence.insert(3);

    try {
      // Should throw because of invalid range
      sequence.insert(SequenceRange(50, 40));
      TEST_CHECK(false);
    } catch (const std::exception&) {
    }

    // Carefully pick values such that:
    //   first < second
    //   second < third
    //   third < first
    SequenceNumber first(SN_SEAM+1);
    SequenceNumber second(SN_RANGE/2+1);
    SequenceNumber third(1);

    sequence.reset();
    sequence.insert(1);
    sequence.insert(3);

    try {
      sequence.insert(SequenceRange(SN_RANGE/2, SN_MAX));
      TEST_CHECK(false);
    } catch (const std::exception&) {
    }

    sequence.reset();
    sequence.insert(0);

    try {
      sequence.insert(SequenceRange(4, 3));
      TEST_CHECK(false);
    } catch (const std::exception&) {
    }

    sequence.reset();
    sequence.insert(first);
    sequence.insert(second);
  }

  // Range iterator
  {
    DisjointSequence sequence;
    SequenceRange range;

    // ASSERT a single discontiguity returns a single range
    //        of values: <low + 1, high - 1>
    sequence.reset();
    sequence.insert(1);
    sequence.insert(6); // discontiguity

    std::vector<SequenceRange> missingSet = sequence.missing_sequence_ranges();
    std::vector<SequenceRange>::const_iterator it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(5));
    TEST_CHECK(++it == missingSet.end());

    // ASSERT multiple contiguities return multiple ranges
    //        of values:
    sequence.reset();
    sequence.insert(1);
    sequence.insert(6);   // discontiguity
    sequence.insert(7);
    sequence.insert(11);  // discontiguity

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
    sequence.reset();
    sequence.insert(SN_MIN);
    sequence.insert(3);  // discontiguity
    sequence.insert(6);  // discontiguity
    sequence.insert(8);  // discontiguity
    sequence.insert(10);  // discontiguity

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
    sequence.reset();
    sequence.insert(SN_MAX-2);
    sequence.insert(1);   // discontiguity
    sequence.insert(6);   // discontiguity
    sequence.insert(10);  // discontiguity

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

    // ASSERT a range insert returns the proper iterators
    sequence.reset();
    sequence.insert(1);
    sequence.insert(SequenceRange(3, 5)); // discontiguity
    sequence.insert(6);

    missingSet = sequence.missing_sequence_ranges();
    it = missingSet.begin();

    range = *it;
    TEST_CHECK(range.first == SequenceNumber(2));
    TEST_CHECK(range.second == SequenceNumber(2));
    TEST_CHECK(++it == missingSet.end());
  }
  {
    DisjointSequence sequence;
    sequence.insert(SequenceRange(3, 5));
    sequence.insert(SequenceRange(2, 4));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.low() == 2);
    TEST_CHECK(sequence.high() == 5);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(6);
    sequence.insert(5);
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.low() == 3);
    TEST_CHECK(sequence.high() == 6);
    std::vector<SequenceRange> ranges = sequence.present_sequence_ranges();
    TEST_CHECK(ranges.size() == 2);
    TEST_CHECK(ranges[0] == SequenceRange(3, 3));
    TEST_CHECK(ranges[1] == SequenceRange(5, 6));
  }
  {
    DisjointSequence sequence;
    sequence.insert(2);
    sequence.insert(5);
    sequence.insert(6);
    sequence.insert(9);
    sequence.insert(10);
    sequence.insert(11);

    std::vector<SequenceRange> dropped;
    TEST_CHECK(sequence.insert(SequenceRange(sequence.low(), 12), dropped));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(dropped.size() == 3);
    TEST_CHECK(dropped[0] == SequenceRange(3, 4));
    TEST_CHECK(dropped[1] == SequenceRange(7, 8));
    TEST_CHECK(dropped[2] == SequenceRange(12, 12));
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(5);
    sequence.insert(6);
    sequence.insert(7);

    std::vector<SequenceRange> dropped;
    TEST_CHECK(sequence.insert(SequenceRange(sequence.low(), 7), dropped));
    TEST_CHECK(dropped.size() == 1);
    TEST_CHECK(dropped[0] == SequenceRange(4, 4));
  }
  {
    DisjointSequence sequence;
    sequence.insert(4);
    sequence.insert(6);
    sequence.insert(10);

    std::vector<SequenceRange> dropped;
    TEST_CHECK(sequence.insert(SequenceRange(sequence.low(), 8), dropped));
    TEST_CHECK(dropped.size() == 2);
    TEST_CHECK(dropped[0] == SequenceRange(5, 5));
    TEST_CHECK(dropped[1] == SequenceRange(7, 8));
  }
  {
    DisjointSequence sequence;
    sequence.insert(5);
    sequence.insert(7);
    sequence.insert(10);

    std::vector<SequenceRange> dropped;
    TEST_CHECK(sequence.insert(SequenceRange(sequence.low(), 9), dropped));
    TEST_CHECK(dropped.size() == 2);
    TEST_CHECK(dropped[0] == SequenceRange(6, 6));
    TEST_CHECK(dropped[1] == SequenceRange(8, 9));
  }
  {
    DisjointSequence sequence;
    sequence.insert(SequenceRange(1, 2));
    sequence.insert(5);
    sequence.insert(9);

    std::vector<SequenceRange> dropped;
    TEST_CHECK(sequence.insert(SequenceRange(4, 12), dropped));
    TEST_CHECK(dropped.size() == 3);
    TEST_CHECK(dropped[0] == SequenceRange(4, 4));
    TEST_CHECK(dropped[1] == SequenceRange(6, 8));
    TEST_CHECK(dropped[2] == SequenceRange(10, 12));
  }
  return 0;
}
