#include "tools/rtpsrelay/lib/Name.h"
#include "tools/rtpsrelay/lib/PartitionIndex.h"

#include "dds/DCPS/Service_Participant.h"

#include <iostream>

using namespace RtpsRelay;

void test_valid(int& status, const std::string& s, const Name& expected)
{
  Name actual(s);
  if (!actual.is_valid()) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ":  expected name to be valid but it wasn't" << std::endl;
    status = EXIT_FAILURE;
    return;
  }

  if (actual != expected) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ':'
              << " expected=" << expected << " pattern=" << std::boolalpha << expected.is_pattern() << " valid=" << std::boolalpha << expected.is_valid()
              << " actual=" << actual << " pattern=" << std::boolalpha <<  actual.is_pattern() << " valid=" << std::boolalpha << actual.is_valid() << std::endl;
    status = EXIT_FAILURE;
    return;
  }
}

void test_invalid(int& status, const std::string& s)
{
  Name actual(s);
  if (actual.is_valid()) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ":  expected name to be invalid but it wasn't" << std::endl;
    status = EXIT_FAILURE;
    return;
  }
}

void test_equal(int& status, const std::string& s, const GuidSet& actual, const GuidSet& expected)
{
  if (actual != expected) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ":  expected guid sets not equal" << std::endl;
    status = EXIT_FAILURE;
    return;
  }

}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  int status = EXIT_SUCCESS;

  std::set<char> digits;
  for (char c = '0'; c <= '9'; ++c) {
    digits.insert(c);
  }

  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  {
    Name expected;
    test_valid(status, "", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    test_valid(status, "a", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid(status, "?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('?'));
    test_valid(status, "\\?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid(status, "*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('*'));
    test_valid(status, "\\*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('\\'));
    test_valid(status, "\\\\", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(']'));
    test_valid(status, "]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    expected.push_back(Atom('b'));
    expected.push_back(Atom('c'));
    test_valid(status, "abc", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    expected.push_back(Atom('.'));
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid(status, "*.?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, abc));
    test_valid(status, "[abc]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid(status, "[0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid(status, "[091-8]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(true, digits));
    test_valid(status, "[!0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid(status, "**", expected);
  }

  test_invalid(status, "\\");
  test_invalid(status, "[");
  test_invalid(status, "[]");
  test_invalid(status, "[a-]");
  test_invalid(status, "[b-a]");

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
    test_equal(status, "find '' with ''", actual, expected);

    actual.clear();
    pi.lookup("*", actual);
    test_equal(status, "find '' with '*'", actual, expected);

    actual.clear();
    pi.lookup("**", actual);
    test_equal(status, "find '' with '**'", actual, expected);

    actual.clear();
    pi.lookup("***", actual);
    test_equal(status, "find '' with '***'", actual, expected);

    pi.remove("", guid1);

    actual.clear();
    pi.lookup("", actual);
    test_equal(status, "remove ''", actual, GuidSet());
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
    test_equal(status, "find 'apple' with 'apple'", actual, expected);

    actual.clear();
    pi.lookup("[ab]pple", actual);
    test_equal(status, "find 'apple' with '[ab]pple'", actual, expected);

    actual.clear();
    pi.lookup("[!b]pple", actual);
    test_equal(status, "find 'apple' with '[!b]pple'", actual, expected);

    actual.clear();
    pi.lookup("?pple", actual);
    test_equal(status, "find 'apple' with '?pple'", actual, expected);

    actual.clear();
    pi.lookup("**a**p**p**l**e**", actual);
    test_equal(status, "find 'apple' with '**a**p**p**l**e**'", actual, expected);

    pi.remove("apple", guid1);

    actual.clear();
    pi.lookup("apple", actual);
    test_equal(status, "remove 'apple'", actual, GuidSet());
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
    test_equal(status, "find '**a[pq][!rs]?**' with 'apple'", actual, expected);

    actual.clear();
    pi.lookup("aqple", actual);
    test_equal(status, "find '**a[pq][!rs]?**' with 'aqple'", actual, expected);

    actual.clear();
    pi.lookup("arple", actual);
    test_equal(status, "don't find '**a[pq][!rs]?**' with 'arple'", actual, GuidSet());

    actual.clear();
    pi.lookup("aprle", actual);
    test_equal(status, "don't find '**a[pq][!rs]?**' with 'aprle'", actual, GuidSet());

    actual.clear();
    pi.lookup("appl", actual);
    test_equal(status, "find '**a[pq][!rs]?**' with 'apple'", actual, expected);

    pi.remove("**a[pq][!rs]?**", guid1);

    actual.clear();
    pi.lookup("apple", actual);
    test_equal(status, "remove '**a[pq][!rs]?**'", actual, GuidSet());
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
    test_equal(status, "general find 'apple'", actual, expected);

    expected.clear();
    expected.insert(guid2);
    expected.insert(guid3);
    actual.clear();

    pi.lookup("orange", actual);
    test_equal(status, "general find 'orange'", actual, expected);

    expected.clear();
    expected.insert(guid1);
    expected.insert(guid2);
    actual.clear();

    pi.lookup("*a*e*", actual);
    test_equal(status, "general find '*a*e*'", actual, expected);

    expected.clear();
    expected.insert(guid1);
    actual.clear();

    pi.lookup("?pp[lmnop][!i]", actual);
    test_equal(status, "general find '?pp[lmnop][!i]'", actual, expected);
  }

  return status;
}
