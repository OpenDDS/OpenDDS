/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "../common/TestSupport.h"

#include <cstring>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

int ACE_TMAIN(int, ACE_TCHAR*[])
{

  // Test an empty bitmap sequence / zero numbits

  {
    SequenceNumberSet sns;
    sns.numBits = 0;
    sns.bitmap.length(0);
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  // Test a bitmap of one unsigned long

  // Test empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 0;
    sns.bitmap.length(1); // this is not necessarily invalid, could be left over from reducing numBits
    sns.bitmap[0] = 0xFFFFFFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 1;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x7FFFFFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x0000FFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 31;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00000001;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  // Test non-empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 1;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00010000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00010000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00000001;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  // Test a bitmap of more than one unsigned long

  // Test empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(2); // this is not necessarily invalid, could be left over from reducing numBits
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0xFFFFFFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 33;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x7FFFFFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x0000FFFF;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 63;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00000001;
    TEST_CHECK(bitmapNonEmpty(sns) == false);
  }

  // Test non-empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x80000000;
    sns.bitmap[1] = 0x0;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 33;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00010000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00010000;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00000001;
    TEST_CHECK(bitmapNonEmpty(sns) == true);
  }

  return 0;
}
