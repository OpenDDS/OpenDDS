
#include "gtest/gtest.h"
#include "dds/DCPS/iterator_adaptor.h"
#include "dds/DdsDcpsCoreC.h"
#include <vector>
#include <algorithm>

using namespace OpenDDS;

TEST(IteratorAdaptorTest, StdCopy_ToVector_Success)
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

  for (size_t i = 0; i < expected.length(); ++i) {
      ASSERT_EQ(expected[i], result[i]);
  }
}

TEST(IteratorAdaptorTest, StdCopy_FromVector_Success)
{
  std::vector<CORBA::Octet> expected;
  for (size_t i = 0; i < 5; ++i) expected.push_back(i);

  DDS::OctetSeq result;

  std::copy(expected.begin(),
            expected.end(),
            DCPS::back_inserter(result));

  ASSERT_EQ(result.length(), expected.size());

  for (size_t i = 0; i < expected.size(); ++i) {
      ASSERT_EQ(expected[i], result[i]);
  }
}


