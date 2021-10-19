#ifdef OPENDDS_HAS_CXX11

#include <dds/rtpsrelaylib/Name.h>

#include <dds/DCPS/Service_Participant.h>

#include <gtest/gtest.h>

#include <iostream>

using namespace RtpsRelay;

void test_valid(const std::string& s, const Name& expected)
{
  Name actual(s);
  EXPECT_TRUE(actual.is_valid());
  EXPECT_TRUE(actual == expected);
}

void test_invalid(const std::string& s)
{
  Name actual(s);
  EXPECT_TRUE(!actual.is_valid());
}

TEST(tools_rtpsrelay_lib_Name, maintest)
{
  std::unordered_set<char> digits;
  for (char c = '0'; c <= '9'; ++c) {
    digits.insert(c);
  }

  std::unordered_set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  {
    Name expected;
    test_valid("", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    test_valid("a", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid("?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('?'));
    test_valid("\\?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid("*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('*'));
    test_valid("\\*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('\\'));
    test_valid("\\\\", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(']'));
    test_valid("]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    expected.push_back(Atom('b'));
    expected.push_back(Atom('c'));
    test_valid("abc", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    expected.push_back(Atom('.'));
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid("*.?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, abc));
    test_valid("[abc]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid("[0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid("[091-8]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(true, digits));
    test_valid("[!0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid("**", expected);
  }

  test_invalid("\\");
  test_invalid("[");
  test_invalid("[]");
  test_invalid("[a-]");
  test_invalid("[b-a]");
}

#endif
