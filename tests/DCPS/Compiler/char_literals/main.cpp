#include <testTypeSupportImpl.h>

#include <gtest/gtest.h>

TEST(CharLiterals, char_literal_values)
{
  EXPECT_EQ(c_a, 'a');
  EXPECT_EQ(c_newline, '\n');
  EXPECT_EQ(c_tab, '\t');
  EXPECT_EQ(c_vertical_tab, '\v');
  EXPECT_EQ(c_backspace, '\b');
  EXPECT_EQ(c_carriage_return, '\r');
  EXPECT_EQ(c_form_feed, '\f');
  EXPECT_EQ(c_alert, '\a');
  EXPECT_EQ(c_backslash, '\\');
  EXPECT_EQ(c_question, '\?');
  EXPECT_EQ(c_single_quote, '\'');
  EXPECT_EQ(c_double_quote, '\"');
  EXPECT_EQ(c_oct_0, '\0');
  EXPECT_EQ(c_oct_255, '\377');
  EXPECT_EQ(c_hex_1, '\x1');
  EXPECT_EQ(c_hex_254, '\xfe');
}

TEST(CharLiterals, wchar_literal_values)
{
  EXPECT_EQ(c_a, L'a');
  /* TODO(iguessthislldo): See https://github.com/DOCGroup/ACE_TAO/issues/1284
  EXPECT_EQ(wc_newline, L'\n');
  EXPECT_EQ(wc_tab, L'\t');
  EXPECT_EQ(wc_vertical_tab, L'\v');
  EXPECT_EQ(wc_backspace, L'\b');
  EXPECT_EQ(wc_carriage_return, L'\r');
  EXPECT_EQ(wc_form_feed, L'\f');
  EXPECT_EQ(wc_alert, L'\a');
  EXPECT_EQ(wc_backslash, L'\\');
  EXPECT_EQ(wc_question, L'\?');
  EXPECT_EQ(wc_single_quote, L'\'');
  EXPECT_EQ(wc_double_quote, L'\"');
  EXPECT_EQ(wc_oct_0, L'\0');
  EXPECT_EQ(wc_oct_255, L'\377');
  EXPECT_EQ(wc_hex_1, L'\x1');
  EXPECT_EQ(wc_hex_254, L'\xfe');
  */
  // TODO(iguessthislldo): See https://github.com/DOCGroup/ACE_TAO/issues/1284#issuecomment-879455512
#if CPP11_MAPPING
  EXPECT_EQ(wc_u_escape, L'\x203C');
#endif
}

TEST(CharLiterals, str_literal_values)
{
  EXPECT_EQ(str_normal, std::string("a\n \t \v \b \r \f \a \\ \? \' \""));
  EXPECT_EQ(str_values, std::string("\377\x1\xfe\0"));
}

/* TODO(iguessthislldo): See IDL
TEST(CharLiterals, wstr_literal_values)
{
  EXPECT_EQ(wstr_normal, std::wstring(L"a\n \t \v \b \r \f \a \\ \? \' \" \x203c"));
  EXPECT_EQ(wstr_values, std::wstring(L"\377\x1\xfe\0"));
}
*/

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
