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
    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(1));
    TEST_CHECK(sequence.last_ack() == SequenceNumber(6));
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
    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(1));
    TEST_CHECK(sequence.last_ack() == SequenceNumber(3));
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
    TEST_CHECK(sequence.last_ack() == SequenceNumber(5));
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
    TEST_CHECK(sequence.last_ack() == SequenceNumber(6));
    TEST_CHECK(sequence.disjoint());

    sequence.insert(SequenceRange(2, 3));

    TEST_CHECK(sequence.cumulative_ack() == SequenceNumber(4));
    TEST_CHECK(sequence.high() == SequenceNumber(7));
    TEST_CHECK(sequence.last_ack() == SequenceNumber(6));
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

    // See RTPS v2.1 section 9.4.2.6 for the definition the X:Y/ZZZZ notation
    CORBA::Long bitmap; // 4:3/011 = (5,6)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 3);
    TEST_CHECK((bitmap & 0xE0000000) == 0x60000000);

    CORBA::Long anti_bitmap; // 4:1/1 = (4,4)
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(&anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 1);
    TEST_CHECK((anti_bitmap & 0x80000000) == 0x80000000);
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

    CORBA::Long bitmap; // 9:2/01 = (10,10)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 2);
    TEST_CHECK((bitmap & 0xC0000000) == 0x40000000);
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

    CORBA::Long bitmap; // 3:10/0111111111 = (4,12)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 10);
    TEST_CHECK((bitmap & 0xFFC00000) == 0x7FC00000);
  }
  {
    DisjointSequence sequence;
    CORBA::Long bits[] = { 1 << 31 }; // high bit is logical index '0'
    TEST_CHECK(sequence.insert(5, 1 /*num_bits*/, bits));
    TEST_CHECK(!sequence.empty() && !sequence.disjoint());
    TEST_CHECK(sequence.low() == 5 && sequence.high() == 5);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    CORBA::Long bits[] = { 1 << 31 }; // high bit is logical index '0'
    TEST_CHECK(sequence.insert(5, 1 /*num_bits*/, bits));
    TEST_CHECK(!sequence.empty() && sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 5);

    CORBA::Long bitmap; // 4:2/01 = (5,5)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 2);
    TEST_CHECK((bitmap & 0xC0000000) == 0x40000000);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(7);
    CORBA::Long bits[] = { 1 << 31 }; // high bit is logical index '0'
    TEST_CHECK(sequence.insert(5, 1 /*num_bits*/, bits));
    TEST_CHECK(!sequence.empty() && sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 7);
    TEST_CHECK(sequence.present_sequence_ranges()[1] == SequenceRange(5, 5));

    CORBA::Long bitmap; // 4:4/0101 = (5,5),(7,7)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 4);
    TEST_CHECK((bitmap & 0xF0000000) == 0x50000000);

    CORBA::Long anti_bitmap; // 4:3/101 = (4,4),(6,6)
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(&anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 3);
    TEST_CHECK((anti_bitmap & 0xE0000000) == 0xA0000000);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    CORBA::Long bits[] = { (1 << 31) | (1 << 29) };
    TEST_CHECK(sequence.insert(5, 12 /*num_bits*/, bits));
    TEST_CHECK(!sequence.empty() && sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 7);
    TEST_CHECK(sequence.present_sequence_ranges()[1] == SequenceRange(5, 5));
  }
  {
    DisjointSequence sequence;
    sequence.insert(4);
    CORBA::Long bits[] = { (1 << 31) | (1 << 30) };
    TEST_CHECK(sequence.insert(5, 12 /*num_bits*/, bits));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.low() == 4 && sequence.high() == 6);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(5);
    CORBA::Long bits[] = { (1 << 30) | (1 << 29) | (1 << 28) |
                           (1 << 27) | (1 << 26) }; // 1/6:011111 = (2,6)
    TEST_CHECK(sequence.insert(1, 6 /*num_bits*/, bits));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.low() == 2 && sequence.high() == 6);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(10);
    CORBA::Long bits[] = { (1 << 30) | (1 << 29) |
                           (1 << 27) | (1 << 26) }; // 1/6:011011 = (2,3),(5,6)
    TEST_CHECK(sequence.insert(1, 6 /*num_bits*/, bits));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.low() == 2 && sequence.high() == 10);
    TEST_CHECK(sequence.present_sequence_ranges()[0] == SequenceRange(2, 3));
    TEST_CHECK(sequence.present_sequence_ranges()[1] == SequenceRange(5, 6));

    CORBA::Long bitmap; // 4:7/0110001 = (5,6),(10,10)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 7);
    TEST_CHECK((bitmap & 0xFE000000) == 0x62000000);

    CORBA::Long anti_bitmap; // 4:6/100111 = (4,4),(7,9)
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(&anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 6);
    TEST_CHECK((anti_bitmap & 0xFC000000) == 0x9C000000);
  }
  {
    DisjointSequence sequence;
    sequence.insert(SequenceRange(3, 6));
    sequence.insert(SequenceRange(8, 10));
    CORBA::Long bits[] = { (1 << 30) | (1 << 29) | (1 << 28) |
                           (1 << 27) | (1 << 26) }; // 4/6:011111 = (5,9)
    TEST_CHECK(sequence.insert(4, 6 /*num_bits*/, bits));
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 10);
  }
  {
    DisjointSequence sequence;
    sequence.insert(SequenceRange(3, 6));
    sequence.insert(SequenceRange(8, 10));
    CORBA::Long bits[] = { (1 << 30) | (1 << 29) | (1 << 28) };
    TEST_CHECK(sequence.insert(4, 4 /*num_bits*/, bits)); // 4/4:0111 = (5,7)
    TEST_CHECK(!sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 10);
  }
  {
    DisjointSequence sequence;
    sequence.insert(3);
    sequence.insert(SequenceRange(37, 38)); // skip ahead after inserting 36
    CORBA::Long bits[] = { 0, (1 << 31)     // 36
                            | (1 << 28) | (1 << 27) };  // 39-40
    TEST_CHECK(sequence.insert(4, 37 /*num_bits*/, bits));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.low() == 3 && sequence.high() == 40);
    TEST_CHECK(sequence.present_sequence_ranges()[1] == SequenceRange(36, 40));

    CORBA::Long bitmap[2]; // 4:37/{32x0}11111 = (36,40)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
    TEST_CHECK(num_bits == 37);
    TEST_CHECK(!bitmap[0] && ((bitmap[1] & 0xF8000000) == 0xF8000000));

    CORBA::Long anti_bitmap; // 4:32/{32x1} = (4,35)
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(&anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 32);
    TEST_CHECK(anti_bitmap == 0xFFFFFFFF);
  }
  {
    DisjointSequence sequence;
    sequence.insert(SequenceRange(32, 37)); // skip ahead across a Long boundary
    CORBA::Long bits[] = { (1 << 1),    // 31 (0th long has #s 1-32)
                           (1 << 24) }; // 40 (1st long has #s 33-65)
    TEST_CHECK(sequence.insert(1, 64 /*num_bits*/, bits));
    TEST_CHECK(sequence.disjoint());
    TEST_CHECK(sequence.low() == 31 && sequence.high() == 40);
    TEST_CHECK(sequence.present_sequence_ranges()[0] == SequenceRange(31, 37));
    TEST_CHECK(sequence.present_sequence_ranges()[1] == SequenceRange(40, 40));

    CORBA::Long bitmap; // 38:3/001 = (40,40)
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(&bitmap, 1, num_bits));
    TEST_CHECK(num_bits == 3);
    TEST_CHECK((bitmap & 0xE0000000) == 0x20000000);
  }
  {
    DisjointSequence sequence;
    for (int i = 0; i < 32; ++i) sequence.insert(1 + 2 * i);
    // sequence contains all odd numbers in [1, 63]
    CORBA::Long bitmap[] = {0xAAAAAAAA, 0xAAAAAAAA}; // 2:62/010101...
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
    TEST_CHECK(num_bits == 62);
    TEST_CHECK(bitmap[0] == 0x55555555);
    TEST_CHECK((bitmap[1] & 0xFFFFFFFC) == 0x55555554);

    CORBA::Long anti_bitmap[] = {0x55555555, 0x55555555}; // 2:61/101010...
    CORBA::ULong anti_num_bits;
    // doesn't fit in 1 Long, verify that it returns false
    TEST_CHECK(!sequence.to_bitmap(anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(sequence.to_bitmap(anti_bitmap, 2, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 61);
    TEST_CHECK(anti_bitmap[0] == 0xAAAAAAAA);
    TEST_CHECK((anti_bitmap[1] & 0xFFFFFFF8) == 0xAAAAAAA8);
  }
  {
    DisjointSequence sequence;
    sequence.insert(31); // set base to 32
    sequence.insert(32 + 17); // 17th bit from msb of bitmap[0]
    sequence.insert(64 + 3);  //  3rd bit from msb of bitmap[1]
    CORBA::Long bitmap[2];
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
    TEST_CHECK(num_bits == 36);
    TEST_CHECK(bitmap[0] == 0x00004000);
    TEST_CHECK((bitmap[1] & 0xF0000000) == 0x10000000);

    CORBA::Long anti_bitmap[2];// 32:35/11111111 11111111 10111111 11111111 111
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(anti_bitmap, 2, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 35);
    TEST_CHECK(anti_bitmap[0] == 0xFFFFBFFF);
    TEST_CHECK((anti_bitmap[1] & 0xE0000000) == 0xE0000000);
  }
  {
    DisjointSequence sequence;
    sequence.insert(31); // set base to 32
    sequence.insert(SequenceRange(32 + 10, 64 + 23));
    CORBA::Long bitmap[2];
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
    TEST_CHECK(num_bits == 56);
    TEST_CHECK(bitmap[0] == 0x003FFFFF);
    TEST_CHECK((bitmap[1] & 0xFFFFFF00) == 0xFFFFFF00);

    CORBA::Long anti_bitmap; // 32:10/1111111111
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(&anti_bitmap, 1, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 10);
    TEST_CHECK((anti_bitmap & 0xFFC00000) == 0xFFC00000);
  }
  {
    DisjointSequence sequence;
    sequence.insert(31); // set base to 32
    sequence.insert(SequenceRange(32 + 10, 32 + 19));
    sequence.insert(100);
    CORBA::Long bitmap[3];
    CORBA::ULong num_bits;
    TEST_CHECK(sequence.to_bitmap(bitmap, 3, num_bits));
    TEST_CHECK(num_bits == 69);
    TEST_CHECK(bitmap[0] == 0x003FF000);
    TEST_CHECK(bitmap[1] == 0);
    TEST_CHECK((bitmap[2] & 0xF8000000) == 0x08000000);

    CORBA::Long anti_bitmap[3]; // 32:68/11111111 11000000 00001...
    CORBA::ULong anti_num_bits;
    TEST_CHECK(sequence.to_bitmap(anti_bitmap, 3, anti_num_bits, true));
    TEST_CHECK(anti_num_bits == 68);
    TEST_CHECK(anti_bitmap[0] == 0xFFC00FFF);
    TEST_CHECK(anti_bitmap[1] == 0xFFFFFFFF);
    TEST_CHECK((anti_bitmap[2] & 0xF0000000) == 0xF0000000);
  }
  return 0;
}
