#include "tools/rtpsrelay/lib/Name.h"

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

  return status;
}
