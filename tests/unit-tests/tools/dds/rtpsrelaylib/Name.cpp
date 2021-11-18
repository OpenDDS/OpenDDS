#ifdef OPENDDS_HAS_CXX11

#include <dds/rtpsrelaylib/Name.h>

#include <gtest/gtest.h>

using namespace RtpsRelay;

TEST(tools_dds_rtpsrelaylib_Name, Atom_ctor_character)
{
  Atom atom('z');
  EXPECT_EQ(atom.kind(), Atom::CHARACTER);
  EXPECT_EQ(atom.character(), 'z');
  EXPECT_FALSE(atom.is_pattern());
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_ctor_character_class)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  Atom atom(false, abc);
  EXPECT_EQ(atom.kind(), Atom::CHARACTER_CLASS);
  EXPECT_EQ(atom.characters(), abc);
  EXPECT_TRUE(atom.is_pattern());
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_ctor_negated_character_class)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  Atom atom(true, abc);
  EXPECT_EQ(atom.kind(), Atom::NEGATED_CHARACTER_CLASS);
  EXPECT_EQ(atom.characters(), abc);
  EXPECT_TRUE(atom.is_pattern());
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_ctor_wildcard)
{
  Atom atom(Atom::WILDCARD);
  EXPECT_EQ(atom.kind(), Atom::WILDCARD);
  EXPECT_TRUE(atom.is_pattern());
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_ctor_glob)
{
  Atom atom(Atom::GLOB);
  EXPECT_EQ(atom.kind(), Atom::GLOB);
  EXPECT_TRUE(atom.is_pattern());
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_equal)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  Atom character('z');
  Atom character_class(false, abc);
  Atom negated_character_class(true, abc);
  Atom wildcard(Atom::WILDCARD);
  Atom glob(Atom::GLOB);

  EXPECT_EQ(character, character);
  EXPECT_EQ(character_class, character_class);
  EXPECT_EQ(negated_character_class, negated_character_class);
  EXPECT_EQ(wildcard, wildcard);
  EXPECT_EQ(glob, glob);
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_not_equal)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  Atom character('z');
  Atom character_class(false, abc);
  Atom negated_character_class(true, abc);
  Atom wildcard(Atom::WILDCARD);
  Atom glob(Atom::GLOB);

  EXPECT_NE(character, character_class);
  EXPECT_NE(character_class, negated_character_class);
  EXPECT_NE(negated_character_class, wildcard);
  EXPECT_NE(wildcard, glob);
  EXPECT_NE(glob, character);
}

std::string stringify(const Atom& atom)
{
  std::stringstream ss;
  ss << atom;
  return ss.str();
}

TEST(tools_dds_rtpsrelaylib_Name, Atom_stringify)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  EXPECT_EQ(stringify(Atom('a')), "a");
  EXPECT_EQ(stringify(Atom(false, abc)), "[abc]");
  EXPECT_EQ(stringify(Atom(true, abc)), "[!abc]");
  EXPECT_EQ(stringify(Atom(Atom::WILDCARD)), "?");
  EXPECT_EQ(stringify(Atom(Atom::GLOB)), "*");
  EXPECT_EQ(stringify(Atom('[')), "\\[");
  EXPECT_EQ(stringify(Atom(']')), "\\]");
  EXPECT_EQ(stringify(Atom('!')), "\\!");
  EXPECT_EQ(stringify(Atom('\\')), "\\\\");
  EXPECT_EQ(stringify(Atom('?')), "\\?");
  EXPECT_EQ(stringify(Atom('*')), "\\*");
}

TEST(tools_dds_rtpsrelaylib_Name, AtomHash)
{
  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  Atom character_class(false, abc);
  size_t h = AtomHash()(character_class);
  EXPECT_EQ(h, 6291457u);
}

void test_valid(const std::string& s, const Name& expected, bool pattern)
{
  Name actual(s);
  EXPECT_TRUE(actual.is_valid());
  EXPECT_EQ(actual, expected);
  if (pattern) {
    EXPECT_FALSE(actual.is_literal());
    EXPECT_TRUE(actual.is_pattern());
  } else {
    EXPECT_TRUE(actual.is_literal());
    EXPECT_FALSE(actual.is_pattern());
  }
}

TEST(tools_dds_rtpsrelaylib_Name, valid)
{
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
    test_valid("", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    test_valid("a", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid("?", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom('?'));
    test_valid("\\?", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid("*", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom('*'));
    test_valid("\\*", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom('\\'));
    test_valid("\\\\", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom(']'));
    test_valid("]", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    expected.push_back(Atom('b'));
    expected.push_back(Atom('c'));
    test_valid("abc", expected, false);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    expected.push_back(Atom('.'));
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid("*.?", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom(false, abc));
    test_valid("[abc]", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid("[0-9]", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid("[091-8]", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom(true, digits));
    test_valid("[!0-9]", expected, true);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid("**", expected, true);
  }
}

void test_invalid(const std::string& s)
{
  Name actual(s);
  EXPECT_FALSE(actual.is_valid());
}

TEST(tools_dds_rtpsrelaylib_Name, invalid)
{
  test_invalid("\\");
  test_invalid("[");
  test_invalid("[!");
  test_invalid("[]");
  test_invalid("[a-");
  test_invalid("[a-]");
  test_invalid("[b-a]");
}

TEST(tools_dds_rtpsrelaylib_Name, begin_end)
{
  {
    Name name;
    EXPECT_EQ(name.begin(), name.end());
  }

  {
    Name name("a");
    EXPECT_NE(name.begin(), name.end());
    EXPECT_EQ(*name.begin(), Atom('a'));
  }
}

TEST(tools_dds_rtpsrelaylib_Name, equal_not_equal)
{
  Name a("a");
  Name glob("*");
  Name invalid("[]");

  EXPECT_EQ(a, a);
  EXPECT_EQ(glob, glob);
  EXPECT_EQ(invalid, invalid);

  EXPECT_NE(a, glob);
  EXPECT_NE(glob, invalid);
  EXPECT_NE(invalid, a);
}

std::string stringify(const Name& name)
{
  std::stringstream ss;
  ss << name;
  return ss.str();
}

TEST(tools_dds_rtpsrelaylib_Name, Name_stringify)
{
  EXPECT_EQ(stringify(Name("a[bc][d-e][!fg][!h-i]*?")), "a[bc][de][!fg][!hi]*?");
}

#endif
