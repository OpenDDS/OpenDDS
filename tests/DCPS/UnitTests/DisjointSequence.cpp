/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/DisjointSequence.h"

#include "../common/TestSupport.h"

#include <stdexcept>

using namespace OpenDDS::DCPS;

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    // Construction (default)
    {
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
      TEST_CHECK(sequence.contains(1));
      TEST_CHECK(!sequence.contains(2));
      TEST_CHECK(sequence.contains(3));
      TEST_CHECK(sequence.contains(4));
      TEST_CHECK(sequence.contains(5));
      TEST_CHECK(sequence.contains(6));
      TEST_CHECK(!sequence.contains(7));

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
      }
      catch (const std::exception&) {
      }
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

      OPENDDS_VECTOR(SequenceRange) missingSet = sequence.missing_sequence_ranges();
      OPENDDS_VECTOR(SequenceRange)::const_iterator it = missingSet.begin();

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
      sequence.insert(SequenceNumber::MIN_VALUE);
      sequence.insert(3);  // discontiguity
      sequence.insert(6);  // discontiguity
      sequence.insert(8);  // discontiguity
      sequence.insert(10);  // discontiguity

      missingSet = sequence.missing_sequence_ranges();
      it = missingSet.begin();

      range = *it;
      TEST_CHECK(range.first == SequenceNumber(SequenceNumber::MIN_VALUE + 1));
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
      SequenceNumber zero = SequenceNumber::ZERO();
      sequence.insert(SequenceRange(zero, zero));
      TEST_CHECK(!sequence.empty() && !sequence.disjoint());
      TEST_CHECK(zero == sequence.low() && zero == sequence.high());
      sequence.insert(2);
      OPENDDS_VECTOR(SequenceRange) ranges = sequence.missing_sequence_ranges();
      TEST_CHECK(ranges.size() == 1);
      TEST_CHECK(ranges[0] == SequenceRange(1, 1));
      sequence.reset();
      sequence.insert(SequenceRange(zero, zero));
      sequence.insert(SequenceRange(1, 2));
      TEST_CHECK(!sequence.disjoint());
      sequence.reset();
      sequence.insert(SequenceRange(1, 2));
      sequence.insert(SequenceRange(zero, zero));
      TEST_CHECK(!sequence.disjoint());
    }
    {
      DisjointSequence sequence;
      sequence.insert(3);
      sequence.insert(6);
      sequence.insert(5);
      TEST_CHECK(sequence.disjoint());
      TEST_CHECK(sequence.low() == 3);
      TEST_CHECK(sequence.high() == 6);
      OPENDDS_VECTOR(SequenceRange) ranges = sequence.present_sequence_ranges();
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

      OPENDDS_VECTOR(SequenceRange) dropped;
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

      OPENDDS_VECTOR(SequenceRange) dropped;
      TEST_CHECK(sequence.insert(SequenceRange(sequence.low(), 7), dropped));
      TEST_CHECK(dropped.size() == 1);
      TEST_CHECK(dropped[0] == SequenceRange(4, 4));
    }
    {
      DisjointSequence sequence;
      sequence.insert(4);
      sequence.insert(6);
      sequence.insert(10);

      OPENDDS_VECTOR(SequenceRange) dropped;
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

      OPENDDS_VECTOR(SequenceRange) dropped;
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

      OPENDDS_VECTOR(SequenceRange) dropped;
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
      CORBA::Long bits[] = { 3 << 30 }; // high bit is logical index '0'
      TEST_CHECK(sequence.insert(0, 2 /*num_bits*/, bits));
      TEST_CHECK(!sequence.empty() && !sequence.disjoint());
      TEST_CHECK(sequence.low() == 0 && sequence.high() == 1);
    }
    {
      DisjointSequence sequence;
      CORBA::Long bits[] = { 3 << 29 }; // high bit is logical index '0'
      TEST_CHECK(sequence.insert(0, 3 /*num_bits*/, bits));
      TEST_CHECK(!sequence.empty() && !sequence.disjoint());
      TEST_CHECK(sequence.low() == 1 && sequence.high() == 2);
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
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap) == 0xFFFFFFFF);
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
      CORBA::Long bitmap[] = { CORBA::Long(0xAAAAAAAA), CORBA::Long(0xAAAAAAAA) };
      // 2:62/010101...
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
      TEST_CHECK(num_bits == 62);
      TEST_CHECK(bitmap[0] == 0x55555555);
      TEST_CHECK((bitmap[1] & 0xFFFFFFFC) == 0x55555554);

      CORBA::Long anti_bitmap[] = { 0x55555555, 0x55555555 }; // 2:61/101010...
      CORBA::ULong anti_num_bits;
      // doesn't fit in 1 Long, verify that it returns false
      TEST_CHECK(!sequence.to_bitmap(anti_bitmap, 1, anti_num_bits, true));
      TEST_CHECK(sequence.to_bitmap(anti_bitmap, 2, anti_num_bits, true));
      TEST_CHECK(anti_num_bits == 61);
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[0]) == 0xAAAAAAAA);
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
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[0]) == 0xFFFFBFFF);
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
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[0]) == 0xFFC00FFF);
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[1]) == 0xFFFFFFFF);
      TEST_CHECK((anti_bitmap[2] & 0xF0000000) == 0xF0000000);
    }
    {
      DisjointSequence sequence;
      sequence.insert(0);
      sequence.insert(SequenceRange(3, 3));
      CORBA::Long bitmap[1];
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 1, num_bits));
      TEST_CHECK(num_bits == 3);
      TEST_CHECK((bitmap[0] & 0xE0000000) == 0x20000000);

      CORBA::Long anti_bitmap[1];
      CORBA::ULong anti_num_bits;
      TEST_CHECK(sequence.to_bitmap(anti_bitmap, 1, anti_num_bits, true));
      TEST_CHECK(anti_num_bits == 2);
      TEST_CHECK((anti_bitmap[0] & 0xE0000000) == 0xC0000000);
    }
    {
      DisjointSequence sequence;
      sequence.insert(0);
      sequence.insert(SequenceRange(16, 16));
      sequence.insert(SequenceRange(32, 38));
      CORBA::Long bitmap[2];
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
      TEST_CHECK(num_bits == 38);
      TEST_CHECK(bitmap[0] == 0x00010001);
      TEST_CHECK((bitmap[1] & 0xFC000000) == 0xFC000000);

      CORBA::Long anti_bitmap[1];
      CORBA::ULong anti_num_bits;
      TEST_CHECK(sequence.to_bitmap(anti_bitmap, 1, anti_num_bits, true));
      TEST_CHECK(anti_num_bits == 31);
      TEST_CHECK((anti_bitmap[0] & 0xFFFFFFFE) == 0xFFFEFFFE);
    }
    {
      DisjointSequence sequence;
      sequence.insert(0);
      sequence.insert(SequenceRange(55, 84));
      sequence.insert(SequenceRange(96, 100));
      CORBA::Long bitmap[4];
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 4, num_bits));
      TEST_CHECK(num_bits == 100);
      TEST_CHECK(bitmap[0] == 0x00000000);
      TEST_CHECK(bitmap[1] == 0x000003FF);
      TEST_CHECK(static_cast<unsigned int>(bitmap[2]) == 0xFFFFF001);
      TEST_CHECK((bitmap[3] & 0xF0000000) == 0xF0000000);

      CORBA::Long anti_bitmap[3];
      CORBA::ULong anti_num_bits;
      TEST_CHECK(sequence.to_bitmap(anti_bitmap, 3, anti_num_bits, true));
      TEST_CHECK(anti_num_bits == 95);
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[0]) == 0xFFFFFFFF);
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[1]) == 0xFFFFFC00);
      TEST_CHECK((anti_bitmap[2] & 0x00000FFE) == 0x00000FFE);
    }
    {
      DisjointSequence sequence;
      sequence.insert(SequenceRange( 0, 22));
      sequence.insert(SequenceRange(27, 32));
      sequence.insert(SequenceRange(35, 38));
      sequence.insert(SequenceRange(42, 53));
      sequence.insert(SequenceRange(55, 76));
      sequence.insert(SequenceRange(81, 87));
      sequence.insert(SequenceRange(95, 96));
      CORBA::Long bitmap[3];
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 3, num_bits));
      TEST_CHECK(num_bits == 74);
      TEST_CHECK(bitmap[0] == 0x0FCF1FFE);
      TEST_CHECK(static_cast<unsigned int>(bitmap[1]) == 0xFFFFFC3F);
      TEST_CHECK((bitmap[2] & 0xFFC00000) == 0x80C00000);

      CORBA::Long anti_bitmap[3];
      CORBA::ULong anti_num_bits;
      TEST_CHECK(sequence.to_bitmap(anti_bitmap, 3, anti_num_bits, true));
      TEST_CHECK(anti_num_bits == 72);
      TEST_CHECK(static_cast<unsigned int>(anti_bitmap[0]) == 0xF030E001);
      TEST_CHECK(anti_bitmap[1] == 0x000003C0);
      TEST_CHECK((anti_bitmap[2] & 0xFF000000) == 0x7F000000);
    }
    {
      DisjointSequence sequence;
      sequence.insert(31); // set base to 32
      CORBA::Long bitmap[2] = { 0, 0 };
      CORBA::ULong num_bits;
      TEST_CHECK(sequence.to_bitmap(bitmap, 2, num_bits));
      TEST_CHECK(num_bits == 0);
      TEST_CHECK(bitmap[0] == 0);
      TEST_CHECK(bitmap[1] == 0);
    }
    {
      DisjointSequence sequence;
      sequence.insert(31); // set base to 32
      sequence.insert(SequenceRange(32 + 10, 32 + 19));
      sequence.insert(SequenceRange(32 + 63, 32 + 66));
      CORBA::Long bitmap[2];
      CORBA::ULong num_bits;
      TEST_CHECK(!sequence.to_bitmap(bitmap, 2, num_bits));
      TEST_CHECK(num_bits == 64);
      TEST_CHECK(bitmap[0] == 0x003FF000);
      TEST_CHECK(bitmap[1] == 0x00000001);
    }
    {
      DisjointSequence sequence;
      sequence.insert(SequenceRange(1, 3));
      sequence.insert(5);
      sequence.insert(7);
      sequence.insert(9);
      sequence.insert(11);
      sequence.insert(13);
      sequence.insert(15);
      sequence.insert(17);
      sequence.insert(19);
      sequence.insert(21);
      sequence.insert(23);
      sequence.insert(25);
      sequence.insert(27);
      sequence.insert(29);
      sequence.insert(31);
      sequence.insert(33);
      sequence.insert(35);
      sequence.insert(37);
      sequence.insert(39);
      sequence.insert(41);
      sequence.insert(43);
      sequence.insert(45);
      sequence.insert(47);
      sequence.insert(49);
      sequence.insert(51);
      sequence.insert(53);
      sequence.insert(55);
      sequence.insert(57);
      sequence.insert(59);
      sequence.insert(61);
      sequence.insert(63);
      sequence.insert(65);
      sequence.insert(67);
      sequence.insert(69);
      sequence.insert(71);
      sequence.insert(73);
      sequence.insert(75);
      sequence.insert(77);
      sequence.insert(79);
      sequence.insert(81);
      sequence.insert(83);
      sequence.insert(85);
      sequence.insert(87);
      sequence.insert(89);
      sequence.insert(91);
      sequence.insert(93);
      sequence.insert(95);
      sequence.insert(97);
      sequence.insert(99);
      sequence.insert(SequenceRange(101, 103));
      sequence.insert(105);
      sequence.insert(107);
      sequence.insert(109);
      sequence.insert(111);
      sequence.insert(113);
      sequence.insert(115);
      sequence.insert(117);
      sequence.insert(119);
      sequence.insert(121);
      sequence.insert(123);
      sequence.insert(125);
      sequence.insert(127);
      sequence.insert(129);
      sequence.insert(131);
      sequence.insert(133);
      sequence.insert(135);
      sequence.insert(137);
      sequence.insert(139);
      sequence.insert(141);
      sequence.insert(143);
      sequence.insert(145);
      sequence.insert(147);
      sequence.insert(149);
      sequence.insert(151);
      sequence.insert(153);
      sequence.insert(155);
      sequence.insert(157);
      sequence.insert(159);
      sequence.insert(161);
      sequence.insert(163);
      sequence.insert(165);
      sequence.insert(167);
      sequence.insert(169);
      sequence.insert(171);
      sequence.insert(173);
      sequence.insert(175);
      sequence.insert(177);
      sequence.insert(179);
      sequence.insert(181);
      sequence.insert(183);
      sequence.insert(185);
      sequence.insert(187);
      sequence.insert(189);
      sequence.insert(191);
      sequence.insert(193);
      sequence.insert(195);
      sequence.insert(197);
      sequence.insert(199);
      sequence.insert(SequenceRange(201, 203));
      sequence.insert(205);
      sequence.insert(207);
      sequence.insert(209);
      sequence.insert(211);
      sequence.insert(213);
      sequence.insert(215);
      sequence.insert(217);
      sequence.insert(219);
      sequence.insert(221);
      sequence.insert(223);
      sequence.insert(225);
      sequence.insert(227);
      sequence.insert(229);
      sequence.insert(231);
      sequence.insert(233);
      sequence.insert(235);
      sequence.insert(237);
      sequence.insert(239);
      sequence.insert(241);
      sequence.insert(243);
      sequence.insert(245);
      sequence.insert(247);
      sequence.insert(249);
      sequence.insert(251);
      sequence.insert(253);
      sequence.insert(255);
      CORBA::Long bitmap[8];
      CORBA::ULong num_bits = 0;
      TEST_CHECK(sequence.to_bitmap(bitmap, 8, num_bits));
      TEST_CHECK(num_bits == 252);
      TEST_CHECK(bitmap[0] == 0x55555555);
      TEST_CHECK(bitmap[1] == 0x55555555);
      TEST_CHECK(bitmap[2] == 0x55555555);
      TEST_CHECK(bitmap[3] == 0x75555555);
      TEST_CHECK(bitmap[4] == 0x55555555);
      TEST_CHECK(bitmap[5] == 0x55555555);
      TEST_CHECK(bitmap[6] == 0x57555555);
      TEST_CHECK((bitmap[7] & 0xFFFFFFF0) == 0x55555550);
    }
    {
      DisjointSequence sequence;
      sequence.insert(1979);
      sequence.insert(1981);
      sequence.insert(1983);
      sequence.insert(1985);
      sequence.insert(1987);
      sequence.insert(1989);
      sequence.insert(1991);
      sequence.insert(1993);
      sequence.insert(1995);
      sequence.insert(1997);
      sequence.insert(1999);
      sequence.insert(SequenceRange(2001, 2580));
      CORBA::Long bitmap[8];
      CORBA::ULong num_bits = 0;
      TEST_CHECK(!sequence.to_bitmap(bitmap, 8, num_bits));
      TEST_CHECK(num_bits == 256);
      TEST_CHECK(bitmap[0] == 0x555557FF);
      TEST_CHECK(bitmap[1] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[2] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[3] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[4] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[5] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[6] == (int) 0xFFFFFFFF);
      TEST_CHECK(bitmap[7] == (int) 0xFFFFFFFF);
    }
    {
      DisjointSequence sequence;
      sequence.insert(1);
      sequence.insert(7);
      sequence.insert(16);
      sequence.insert(SequenceRange(27,65));
      CORBA::Long bitmap[8];
      CORBA::ULong num_bits = 0;
      TEST_CHECK(sequence.to_bitmap(bitmap, 3, num_bits));
      TEST_CHECK(num_bits == 64);
      TEST_CHECK(bitmap[0] == 0x0402007F);
      TEST_CHECK(bitmap[1] == (int) 0xFFFFFFFF);
    }
  }
  catch (std::runtime_error& err)
  {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
      err.what()), -1);
  }
  return 0;
}
