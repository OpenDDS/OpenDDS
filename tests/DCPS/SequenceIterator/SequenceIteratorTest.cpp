
#include "gtest/gtest.h"
#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/SequenceIterator.h"
#include <vector>
#include <algorithm>

using namespace OpenDDS;

TEST(SequenceIteratorTest, Iterator_Concept)
{
  DDS::OctetSeq s(1);
  s.length(1);
  s[0] = 1;

  typedef DCPS::SequenceIterator<DDS::OctetSeq> iter_t;
  iter_t i1 = DCPS::sequence_begin(s);
  iter_t i2(i1);
  i1 = i2;
  using std::swap;
  swap(i1, i2);
  const CORBA::Octet o = *i1;
  ACE_UNUSED_ARG(o);
  ++i1;
}

TEST(SequenceIteratorTest, StdCopy_ToVector_Success)
{
  DDS::OctetSeq expected;
  expected.length(4);
  expected[0] = 1;
  expected[1] = 2;
  expected[2] = 3;
  expected[3] = 4;

  std::vector<CORBA::Octet> result;

  std::copy(DCPS::sequence_begin(expected),
            DCPS::sequence_end(expected),
            std::back_inserter(result));

  ASSERT_EQ(expected.length(), result.size());

  for (CORBA::ULong i = 0; i < expected.length(); ++i) {
    ASSERT_EQ(expected[i], result[i]);
  }
}

TEST(SequenceIteratorTest, StdCopy_FromVector_Success)
{
  std::vector<CORBA::Octet> expected;
  for (CORBA::Octet i = 0; i < 5; ++i) expected.push_back(i);

  DDS::OctetSeq result;

  std::copy(expected.begin(),
            expected.end(),
            DCPS::back_inserter(result));

  ASSERT_EQ(result.length(), expected.size());

  for (CORBA::ULong i = 0; i < expected.size(); ++i) {
    ASSERT_EQ(expected[i], result[i]);
  }
}
