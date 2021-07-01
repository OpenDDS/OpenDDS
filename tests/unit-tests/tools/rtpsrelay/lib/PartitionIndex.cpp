#ifdef OPENDDS_HAS_CXX11

#include <gtest/gtest.h>
#include "tools/rtpsrelay/lib/PartitionIndex.h"

#include "dds/DCPS/Service_Participant.h"

#include <iostream>

using namespace RtpsRelay;

void test_equal(const char*, const GuidSet& actual, const GuidSet& expected)
{
  EXPECT_TRUE(actual == expected);
}

TEST(tools_rtpsrelay_lib_PartitionIndex, maintest)
{
  // Literal test.
  OpenDDS::DCPS::GUID_t guid1 = make_part_guid(OpenDDS::DCPS::GUID_UNKNOWN);
  guid1.guidPrefix[0] = 1;
  OpenDDS::DCPS::GUID_t guid2 = make_part_guid(OpenDDS::DCPS::GUID_UNKNOWN);
  guid2.guidPrefix[0] = 2;
  OpenDDS::DCPS::GUID_t guid3 = make_part_guid(OpenDDS::DCPS::GUID_UNKNOWN);
  guid3.guidPrefix[0] = 3;
  OpenDDS::DCPS::GUID_t guid4 = make_part_guid(OpenDDS::DCPS::GUID_UNKNOWN);
  guid3.guidPrefix[0] = 4;

  {
    // Test the empty string.
    PartitionIndex pi;
    pi.insert("", guid1);

    GuidSet expected;
    expected.insert(guid1);

    GuidSet actual;

    actual.clear();
    pi.lookup("", actual);
    test_equal("find '' with ''", actual, expected);

    actual.clear();
    pi.lookup("*", actual);
    test_equal("find '' with '*'", actual, expected);

    actual.clear();
    pi.lookup("**", actual);
    test_equal("find '' with '**'", actual, expected);

    actual.clear();
    pi.lookup("***", actual);
    test_equal("find '' with '***'", actual, expected);

    pi.remove("", guid1);

    actual.clear();
    pi.lookup("", actual);
    test_equal("remove ''", actual, GuidSet());
  }

  {
    // Test a literal.
    PartitionIndex pi;
    pi.insert("apple", guid1);

    GuidSet expected;
    expected.insert(guid1);

    GuidSet actual;

    actual.clear();
    pi.lookup("apple", actual);
    test_equal("find 'apple' with 'apple'", actual, expected);

    actual.clear();
    pi.lookup("[ab]pple", actual);
    test_equal("find 'apple' with '[ab]pple'", actual, expected);

    actual.clear();
    pi.lookup("[!b]pple", actual);
    test_equal("find 'apple' with '[!b]pple'", actual, expected);

    actual.clear();
    pi.lookup("?pple", actual);
    test_equal("find 'apple' with '?pple'", actual, expected);

    actual.clear();
    pi.lookup("**a**p**p**l**e**", actual);
    test_equal("find 'apple' with '**a**p**p**l**e**'", actual, expected);

    pi.remove("apple", guid1);

    actual.clear();
    pi.lookup("apple", actual);
    test_equal("remove 'apple'", actual, GuidSet());
  }

  {
    // Test a pattern.
    PartitionIndex pi;
    pi.insert("**a[pq][!rs]?**", guid1);

    GuidSet expected;
    expected.insert(guid1);

    GuidSet actual;

    actual.clear();
    pi.lookup("apple", actual);
    test_equal("find '**a[pq][!rs]?**' with 'apple'", actual, expected);

    actual.clear();
    pi.lookup("aqple", actual);
    test_equal("find '**a[pq][!rs]?**' with 'aqple'", actual, expected);

    actual.clear();
    pi.lookup("arple", actual);
    test_equal("don't find '**a[pq][!rs]?**' with 'arple'", actual, GuidSet());

    actual.clear();
    pi.lookup("aprle", actual);
    test_equal("don't find '**a[pq][!rs]?**' with 'aprle'", actual, GuidSet());

    actual.clear();
    pi.lookup("appl", actual);
    test_equal("find '**a[pq][!rs]?**' with 'apple'", actual, expected);

    pi.remove("**a[pq][!rs]?**", guid1);

    actual.clear();
    pi.lookup("apple", actual);
    test_equal("remove '**a[pq][!rs]?**'", actual, GuidSet());
  }

  {
    // Test multiple literals and patterns.
    PartitionIndex pi;
    pi.insert("apple", guid1);
    pi.insert("orange", guid2);
    pi.insert("*a*e*", guid3);
    pi.insert("?pp[lmnop][!i]", guid4);

    GuidSet expected;
    GuidSet actual;

    expected.clear();
    expected.insert(guid1);
    expected.insert(guid3);
    expected.insert(guid4);
    actual.clear();

    pi.lookup("apple", actual);
    test_equal("general find 'apple'", actual, expected);

    expected.clear();
    expected.insert(guid2);
    expected.insert(guid3);
    actual.clear();

    pi.lookup("orange", actual);
    test_equal("general find 'orange'", actual, expected);

    expected.clear();
    expected.insert(guid1);
    expected.insert(guid2);
    actual.clear();

    pi.lookup("*a*e*", actual);
    test_equal("general find '*a*e*'", actual, expected);

    expected.clear();
    expected.insert(guid1);
    actual.clear();

    pi.lookup("?pp[lmnop][!i]", actual);
    test_equal("general find '?pp[lmnop][!i]'", actual, expected);
  }
}

#endif
