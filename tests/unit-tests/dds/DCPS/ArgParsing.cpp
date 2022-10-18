#include <dds/DCPS/ArgParsing.h>
#include <dds/DCPS/Util.h>

#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <cstring>
#include <sstream>

using namespace OpenDDS::DCPS::ArgParsing;
using OpenDDS::DCPS::String;

namespace {
  struct ArgvMaker {
    int argc;
    char** argv;
    StrVec args;

    ArgvMaker()
      : argc(0)
      , argv(0)
    {
      reset();
    }

    virtual ~ArgvMaker()
    {
      reset();
    }

    ArgvMaker& operator()(const char* argument)
    {
      args.push_back(argument);
      return *this;
    }

    void done()
    {
      clear_argv();
      argv = new char*[args.size() + 1];
      for (StrVecIt it = args.begin(); it != args.end(); it++) {
        argv[argc++] = std::strcpy(new char[it->size() + 1], it->c_str());
      }
      argv[argc] = 0;
    }

    void clear_argv()
    {
      if (argv) {
        for (int i = 0; i < argc; i++) {
          if (argv[i]) {
            delete [] argv[i];
          }
        }
        delete [] argv;
        argv = 0;
      }
      argc = 0;
    }

    void reset()
    {
      clear_argv();
      args.clear();
    }
  };
}

TEST(dds_DCPS_ArgParsing_ArgParser, realistic_example)
{
  ArgvMaker am;
  am("./prog")("foo")("bar")("-x")("--y-opt")("3").done();
  int argc = am.argc;
  char** argv = am.argv;

  // Start of what should be copied to real code
  String a;
  String b = "DEFAULT";
  unsigned c = 2;
  bool x = false;
  int y = -2;

  {
    using namespace OpenDDS::DCPS::ArgParsing;

    ArgParser arg_parser("This is a program that accepts arguments");
    arg_parser.call_exit_ = false; // REMOVE THIS LINE IN REAL CODE

    Positional a_arg(arg_parser, "A", "This is argument A, required", a);
    arg_parser.start_optional_positionals();
    Positional b_arg(arg_parser, "B", "This is argument B, optional", b);
    PositionalAs<IntValue<unsigned> > c_arg(arg_parser, "C",
      "This is argument C, non-negtive integer, optional", c);
    Option x_opt(arg_parser, "x", "This is option x", x);
    OptionAs<IntValue<int> > y_opt(arg_parser,
      "y-opt", "This is option y, integer", y, "INT");
    y_opt.add_alias("y");

    arg_parser.parse(argc, argv);
  }
  // End of what should be copied to real code

  EXPECT_STREQ(a.c_str(), "foo");
  EXPECT_STREQ(b.c_str(), "bar");
  EXPECT_EQ(c, static_cast<unsigned>(2));
  EXPECT_TRUE(x);
  EXPECT_EQ(y, static_cast<int>(3));
}

namespace {
  struct Helper : public ArgvMaker {
    ArgParser arg_parser;

    Helper()
      : arg_parser("TEST DESCRIPTION")
    {
      reset();
    }

    void reset()
    {
      ArgvMaker::reset();
      args.push_back("PROGRAM NAME");
    }

    bool parse(const std::string& exception = "")
    {
      if (!argv) {
        done();
      }
      arg_parser.reset_arg_counts();

      const bool expecting_exception = exception.length();
      bool got_exception = false;
      bool got_expected_result = false;
      try {
        arg_parser.parse_no_exit(argc, argv);
        got_expected_result = true;
      } catch (const ParseError& e) {
        got_exception = true;
        if (!expecting_exception) {
          ADD_FAILURE() << "parse failed: " << e.what();
        } else if (exception != e.what()) {
          ADD_FAILURE() << "parse got expected \"" << exception
            << "\" but got \"" << e.what() << "\"";
        } else {
          got_expected_result = true;
        }
      } catch (const ExitSuccess&) {
        got_exception = true;
        if (exception == "ExitSuccess") {
          got_expected_result = true;
        } else {
          ADD_FAILURE() << "parse got unexpected ExitSuccess";
        }
      }

      if (expecting_exception && !got_exception) {
        ADD_FAILURE() << "parse should have failed: " << exception;
      }

      reset();
      return got_expected_result;
    }
  };

  struct BasicHelper : public Helper {
    String a;
    Positional a_arg;
    String b;
    Positional b_arg;
    unsigned c;
    PositionalAs<IntValue<unsigned> > c_arg;
    bool x;
    Option x_opt;
    int y;
    OptionAs<IntValue<int> > y_opt;
    String z;
    OptionAs<StringValue> z_opt;

    BasicHelper()
      : Helper()
      , a_arg(arg_parser, "A", "This is arg a", a)
      , b("DEFAULT B VALUE")
      , b_arg(arg_parser, "B", "This is arg b", b, true)
      , c(2)
      , c_arg(arg_parser, "C", "This is arg c", c, true)
      , x(false)
      , x_opt(arg_parser, "x", "This is x", x)
      , y(-120)
      , y_opt(arg_parser, "y-opt", "This is y", y, "INT")
      , z("DEFAULT Z VALUE")
      , z_opt(arg_parser, "z", "This is z", z, "STR")
    {
      c_arg.handler.min_value = 2;
      c_arg.handler.max_value = 4;
      y_opt.add_alias("y");
    }

    void expect(const char* ea, const char* eb = "DEFAULT B VALUE", unsigned ec = 2,
      bool ex = false, int ey = -120, const char* ez = "DEFAULT Z VALUE")
    {
      if (!parse()) {
        return;
      }
      EXPECT_STREQ(a.c_str(), ea);
      EXPECT_STREQ(b.c_str(), eb);
      EXPECT_EQ(c, ec);
      EXPECT_EQ(x, ex);
      EXPECT_EQ(y, ey);
      EXPECT_STREQ(z.c_str(), ez);
    }
  };
}

// TODO: Test --help for each one of these?

TEST(dds_DCPS_ArgParsing_ArgParser, empty_parse_no_args)
{
  Helper h;
  h.parse();
}

TEST(dds_DCPS_ArgParsing_ArgParser, empty_parse_with_args)
{
  // There are no arguments setup, so this should fail.
  Helper h;
  h("this is invalid");
  h.parse("expected no positional argument(s), received 1");
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_no_args)
{
  BasicHelper h;
  h.parse("expected between 1 and 3 positional argument(s), received 0");
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_a)
{
  BasicHelper h;
  h("A");
  h.expect("A");
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_ab)
{
  BasicHelper h;
  h("a again")("b too");
  h.expect("a again", "b too");
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_abx)
{
  {
    // -x doesn't accept any values, so this should fail.
    BasicHelper h;
    h("a again")("-x=INVALID_ARG")("b too");
    h.parse("option -x has the value \"INVALID_ARG\" attached, "
      "but isn't supposed to be passed anything");
  }

  {
    BasicHelper h;
    h("-x")("a again")("b too");
    h.expect("a again", "b too", 2, true);
  }

  {
    BasicHelper h;
    h("a again")("-x")("b too");
    h.expect("a again", "b too", 2, true);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_abc)
{
  // C is expecting an integer between 2 and 4, so these should fail.
  {
    BasicHelper h;
    h("a again")("b too")("not int");
    h.parse("positional argument C was passed \"not int\", which is not a valid integer");
  }
  {
    BasicHelper h;
    h("a again")("b too")("-1");
    h.parse("positional argument C was passed \"-1\", which is not a positive integer");
  }
  {
    BasicHelper h;
    h("a again")("b too")("0");
    h.parse("positional argument C was passed \"0\", which which is less than 2");
  }
  {
    BasicHelper h;
    h("a again")("b too")("100");
    h.parse("positional argument C was passed \"100\", which which is greater than 4");
  }

  {
    BasicHelper h;
    h("a again")("b too")("3");
    h.expect("a again", "b too", 3);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_abcx)
{
  {
    BasicHelper h;
    h("a again")("b too")("-x")("3");
    h.expect("a again", "b too", 3, true);
  }

  {
    BasicHelper h;
    h("a again")("b too")("3")("-x");
    h.expect("a again", "b too", 3, true);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_too_many_args)
{
  {
    BasicHelper h;
    h("a again")("b too")("c too")("INVALID");
    h.parse("expected between 1 and 3 positional argument(s), received 4");
  }

  {
    BasicHelper h;
    h("a again")("b too")("-x")("c too")("INVALID");
    h.parse("expected between 1 and 3 positional argument(s), received 4");
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_multics_style)
{
  {
    BasicHelper h;
    h("a")("-y-opt")("12");
    h.parse("option -y-opt is invalid");
  }
  {
    BasicHelper h;
    h.arg_parser.default_option_style(OptionStyleMultics);
    h("a")("-y-opt")("12");
    h.expect("a", "DEFAULT B VALUE", 2, false, 12);
  }
  {
    BasicHelper h;
    h.arg_parser.default_option_style(OptionStyleMultics);
    h("a")("--y-opt")("23");
    h.expect("a", "DEFAULT B VALUE", 2, false, 23);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, allow_multiple)
{
  {
    // -y and --y-opt should conflict here
    BasicHelper h;
    h("a again")("b too")("3")("-x")("-y=4")("--y-opt")("-1");
    h.parse("option --y-opt was passed multiple times");
  }

  {
    // ... but not here
    BasicHelper h;
    h.y_opt.allow_multiple_ = true;
    h("a again")("b too")("3")("-x")("-y=4")("--y-opt")("-1");
    h.expect("a again", "b too", 3, true, -1);
    EXPECT_EQ(h.y_opt.count(), 2ul);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_abcxy)
{
  {
    BasicHelper h;
    h("a again")("b too")("3")("-x")("--y-opt");
    h.parse("option --y-opt requires exactly 1 argument(s) to be passed after it, "
      "but there are only 0");
  }

  {
    BasicHelper h;
    h("--y-opt=-1")("a again")("b too")("3")("-x");
    h.expect("a again", "b too", 3, true, -1);
  }

  {
    BasicHelper h;
    h("a again")("b too")("3")("-x")("-y")("-2");
    h.expect("a again", "b too", 3, true, -2);
  }

  {
    BasicHelper h;
    h("-y=-2")("a again")("b too")("3")("-x");
    h.expect("a again", "b too", 3, true, -2);
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, basic_parse_ayz)
{
  BasicHelper h;
  h("-z=ZZ Top")("-y=-3")("a");
  h.expect("a", "DEFAULT B VALUE", 2, false, -3, "ZZ Top");
}

TEST(dds_DCPS_ArgParsing_ArgParser, unknown_option)
{
  Helper h;
  int i = 1;
  PositionalAs<IntValue<int> > i_arg(h.arg_parser, "INT", "Int", i);

  h("-1x");
  h.parse("option -1x is invalid");

  h("-1");
  h.parse();
  EXPECT_EQ(i, -1);
}

namespace {
  StrVec ordered_values;

  class OrderedValue : public StringValue {
  public:
    OrderedValue(String& dest)
      : StringValue(dest)
    {
    }

    StrVecIt handle(Argument& arg, ArgParseState& state, StrVecIt values)
    {
      const StrVecIt rv = StringValue::handle(arg, state, values);
      ordered_values.push_back(*dest_);
      return rv;
    }
  };
}

TEST(dds_DCPS_ArgParsing_ArgParser, option_order)
{
  Helper h;
  String str;
  OptionAs<OrderedValue> o1(h.arg_parser, "o1", "Ordered option 1", str, "STR");
  OptionAs<OrderedValue> o2(h.arg_parser, "o2", "Ordered option 2", str, "STR");
  OptionAs<OrderedValue> o3(h.arg_parser, "o3", "Ordered option 3", str, "STR");

  h("--o3")("C")("--o1")("A")("--o2")("B");
  h.parse();
  ASSERT_EQ(ordered_values.size(), 3lu);
  EXPECT_STREQ(ordered_values[0].c_str(), "C");
  EXPECT_STREQ(ordered_values[1].c_str(), "A");
  EXPECT_STREQ(ordered_values[2].c_str(), "B");
}

TEST(dds_DCPS_ArgParsing_ArgParser, attached_value_separator)
{
  Helper h;
  h.arg_parser.default_option_style(OptionStyleMultics);

  int x = 0;
  OptionAs<IntValue<int> > x_opt(h.arg_parser, "x", "Default =", x, "INT");
  int y = 0;
  OptionAs<IntValue<int> > y_opt(h.arg_parser, "y", "Try multiple char separator", y, "INT");
  y_opt.attached_value_separator_ = "->";

  String define;
  OptionAs<StringValue> define_opt(h.arg_parser, "d", "Something like -D or -I", define, "NAME=VALUE");
  define_opt.attached_value_separator_ = "";

  String detach;
  OptionAs<StringValue> detach_opt(h.arg_parser, "detach", "Try to throw off -d", detach, "VALUE");
  detach_opt.attached_value_separator_ = "";

  {
    h("-x=3")("-y->4")("-detachvalue")("-dname=value");
    h.parse();
    EXPECT_EQ(x, 3);
    EXPECT_EQ(y, 4);
    EXPECT_STREQ(define.c_str(), "name=value");
    EXPECT_STREQ(detach.c_str(), "value");
  }

  {
    y = 0;
    h("-y=5");
    h.parse("option -y=5 is invalid");
  }
}

TEST(dds_DCPS_ArgParsing_ArgParser, bundling)
{
  Helper h;

  bool x = false;
  Option x_opt(h.arg_parser, "x", "x", x);
  bool y = false;
  Option y_opt(h.arg_parser, "y", "y", y);
  bool z = false;
  Option z_opt(h.arg_parser, "z", "z", z);

  {
    h("-zy");
    h.parse();
    EXPECT_FALSE(x);
    EXPECT_TRUE(y);
    EXPECT_TRUE(y);
  }

  {
    h("-zyx");
    h.parse();
    EXPECT_TRUE(x);
    EXPECT_TRUE(y);
    EXPECT_TRUE(y);
  }

  {
    h("-xyw");
    h.parse("option -xyw is invalid");
  }
}

TEST(dds_DCPS_ArgParsing_ChoicesValue, parse)
{
  Helper h;
  String pos1;
  int pos2 = 0;
  String opt1;
  int opt2 = 0;

  PositionalAs<StringChoiceValue> pos_arg1(h.arg_parser, "POS", "Positional Choice 1", pos1);
  pos_arg1.handler.add_choice("foo", "foo choice");
  pos_arg1.handler.add_choice("bar", "bar choice");
  PositionalAs<ChoiceValue<int> > pos_arg2(h.arg_parser, "", "Positional Choice 2", pos2);
  pos_arg2.handler.add_choice("one", 1, "one choice");
  pos_arg2.handler.add_choice("two", 2, "two choice");
  OptionAs<StringChoiceValue> opt_arg1(h.arg_parser, "a", "Option Choice 1", opt1, "CHOICE");
  opt_arg1.handler.add_choice("foo", "foo choice");
  opt_arg1.handler.add_choice("bar", "bar choice");
  OptionAs<ChoiceValue<int> > opt_arg2(h.arg_parser, "b", "Option Choice 2", opt2, "");
  opt_arg2.handler.add_choice("one", 1, "one choice");
  opt_arg2.handler.add_choice("two", 2, "");

  h("foo")("one");
  h.parse();
  EXPECT_STREQ(pos1.c_str(), "foo");
  EXPECT_EQ(pos2, 1);
  EXPECT_STREQ(opt1.c_str(), "");
  EXPECT_EQ(opt2, 0);

  h("bar")("two")("-a")("foo")("-b=one");
  h.parse();
  EXPECT_STREQ(pos1.c_str(), "bar");
  EXPECT_EQ(pos2, 2);
  EXPECT_STREQ(opt1.c_str(), "foo");
  EXPECT_EQ(opt2, 1);

  h("zar")("one");
  h.parse(
    "positional argument POS was passed invalid value \"zar\".\n"
    "Valid choices are:\n"
    "foo: foo choice\n"
    "bar: bar choice"
  );

  h("foo")("one")("-b")("1");
  h.parse(
    "option -b was passed invalid value \"1\".\n"
    "Valid choices are:\n"
    "one: one choice\n"
    "two"
  );

  std::ostringstream out;
  h.arg_parser.out_stream_ = &out;
  h.arg_parser.call_exit_ = false;

  h("--help");
  h.parse("ExitSuccess");
  ASSERT_STREQ(
    "Usage:\n"
    "  PROGRAM NAME [OPTION...] POS one|two\n"
    "  PROGRAM NAME --help | -h\n"
    "  PROGRAM NAME --version | -v\n"
    "\n"
    "Description:\n"
    "  TEST DESCRIPTION\n"
    "\n"
    "Positional arguments:\n"
    "  POS             Positional Choice 1\n"
    "                  Valid choices are:\n"
    "                  foo: foo choice\n"
    "                  bar: bar choice\n"
    "  one|two         Positional Choice 2\n"
    "                  Valid choices are:\n"
    "                  one: one choice\n"
    "                  two: two choice\n"
    "\n"
    "Options:\n"
    "  All OpenDDS command line options listed in section 7.2 of the OpenDDS\n"
    "  Developer's Guide are also available.\n"
    "  --help | -h     Print this message.\n"
    "  --version | -v  Print the program version.\n"
    "  -a CHOICE       Option Choice 1\n"
    "                  Valid choices are:\n"
    "                  foo: foo choice\n"
    "                  bar: bar choice\n"
    "  -b one|two      Option Choice 2\n"
    "                  Valid choices are:\n"
    "                  one: one choice\n"
    "                  two\n",
    out.str().c_str()
  );
}

namespace {
  const String wrap_text =
    "This is the first line with explicit break\n"
    "Shorter line with explicit break\n"
    "\n"
    "\n"
    "This is another line, it it's long and should be always be broken long before "
      "the explicit break.\n"
    "This line has 39 characters ...........\n"
    "This line has 40 characters ............\n"
    "This line has 41 characters .............\n"
    "This line has 49 characters .....................\n"
    "This line has 50 characters ......................\n"
    "This line has 51 characters .......................";
}

TEST(dds_DCPS_ArgParsing_WordWrapper, wrap)
{
  WordWrapper ww(0, 50);
  const String result = ww.wrap(wrap_text);
  const char* expected =
    "This is the first line with explicit break\n"
    "Shorter line with explicit break\n"
    "\n"
    "\n"
    "This is another line, it it's long and should be\n"
    "always be broken long before the explicit break.\n"
    "This line has 39 characters ...........\n"
    "This line has 40 characters ............\n"
    "This line has 41 characters .............\n"
    "This line has 49 characters .....................\n"
    "This line has 50 characters ......................\n"
    "This line has 51 characters\n"
    ".......................\n";
  ASSERT_STREQ(expected, result.c_str());
}

TEST(dds_DCPS_ArgParsing_WordWrapper, wrap_indent)
{
  WordWrapper ww(10, 50);
  const String result = ww.wrap(wrap_text);
  const char* expected =
    "          This is the first line with explicit\n"
    "          break\n"
    "          Shorter line with explicit break\n"
    "\n"
    "\n"
    "          This is another line, it it's long and\n"
    "          should be always be broken long before\n"
    "          the explicit break.\n"
    "          This line has 39 characters ...........\n"
    "          This line has 40 characters ............\n"
    "          This line has 41 characters\n"
    "          .............\n"
    "          This line has 49 characters\n"
    "          .....................\n"
    "          This line has 50 characters\n"
    "          ......................\n"
    "          This line has 51 characters\n"
    "          .......................\n";
  ASSERT_STREQ(expected, result.c_str());
}

TEST(dds_DCPS_ArgParsing_WordWrapper, wrap_with_left_text)
{
  WordWrapper ww(4, 10, 50);
  const String result = ww.wrap("Left", wrap_text);
  const char* expected =
    "    Left  This is the first line with explicit\n"
    "          break\n"
    "          Shorter line with explicit break\n"
    "\n"
    "\n"
    "          This is another line, it it's long and\n"
    "          should be always be broken long before\n"
    "          the explicit break.\n"
    "          This line has 39 characters ...........\n"
    "          This line has 40 characters ............\n"
    "          This line has 41 characters\n"
    "          .............\n"
    "          This line has 49 characters\n"
    "          .....................\n"
    "          This line has 50 characters\n"
    "          ......................\n"
    "          This line has 51 characters\n"
    "          .......................\n";
  ASSERT_STREQ(expected, result.c_str());
}

TEST(dds_DCPS_ArgParsing_WordWrapper, wrap_with_long_left_text)
{
  WordWrapper ww(10, 50);
  const String result = ww.wrap("Left text that goes on too long", wrap_text);
  const char* expected =
    "Left text that goes on too long\n"
    "          This is the first line with explicit\n"
    "          break\n"
    "          Shorter line with explicit break\n"
    "\n"
    "\n"
    "          This is another line, it it's long and\n"
    "          should be always be broken long before\n"
    "          the explicit break.\n"
    "          This line has 39 characters ...........\n"
    "          This line has 40 characters ............\n"
    "          This line has 41 characters\n"
    "          .............\n"
    "          This line has 49 characters\n"
    "          .....................\n"
    "          This line has 50 characters\n"
    "          ......................\n"
    "          This line has 51 characters\n"
    "          .......................\n";
  ASSERT_STREQ(expected, result.c_str());
}

TEST(dds_DCPS_ArgParsing_ArgParser, print_help)
{
  ArgParser arg_parser(
    "This is testing the generated help message.\n"
    "This is testing the generated help message. This is testing the generated help message.");

  String a;
  Positional a_arg(arg_parser, "A",
    "This is argument A, required.\nThis is some text that I'm adding to make "
    "the string long enough to wrap to the next line...", a);
  arg_parser.start_optional_positionals();
  String b;
  Positional b_arg(arg_parser, "B", "This is argument B, optional", b);
  unsigned c = 0;
  PositionalAs<IntValue<unsigned> > c_arg(arg_parser,
    "THE_SUPER_AWESOME_COOL_ARG_WITH_A_LONG_NAME_YEA",
    "This is argument C, non-negtive integer, optional", c);
  bool x = false;
  Option x_opt(arg_parser, "x", "This is option x", x);
  x_opt.show_in_usage_ = true;
  String y;
  OptionAs<StringValue> y_opt(arg_parser, "y-opt-long-name",
    "This is option y, string\nThis line should be next to -y STRING", y, "STRING");
  y_opt.add_alias("y");
  int z = 0;
  OptionAs<IntValue<int> > z_opt(arg_parser,
    "z-opt-long-name-that-should-be-on-its-own-line", "This is option z, integer", z, "INT");
  z_opt.add_alias("z");
  bool hidden;
  Option hidden_opt(arg_parser, "hidden", "This option is hidden", hidden);
  hidden_opt.show_in_help_ = false;

  std::ostringstream out;
  arg_parser.out_stream_ = &out;
  arg_parser.call_exit_ = false;

  ArgvMaker am;
  am("./prog")("--help").done();
  arg_parser.parse(am.argc, am.argv);

  ASSERT_STREQ(
    "Usage:\n"
    "  ./prog [OPTION...] A [B] [THE_SUPER_AWESOME_COOL_ARG_WITH_A_LONG_NAME_YEA] [-x]\n"
    "  ./prog --help | -h\n"
    "  ./prog --version | -v\n"
    "\n"
    "Description:\n"
    "  This is testing the generated help message.\n"
    "  This is testing the generated help message. This is testing the generated help\n"
    "  message.\n"
    "\n"
    "Positional arguments:\n"
    "  A                         This is argument A, required.\n"
    "                            This is some text that I'm adding to make the string\n"
    "                            long enough to wrap to the next line...\n"
    "  B                         This is argument B, optional\n"
    "  THE_SUPER_AWESOME_COOL_ARG_WITH_A_LONG_NAME_YEA\n"
    "                            This is argument C, non-negtive integer, optional\n"
    "\n"
    "Options:\n"
    "  All OpenDDS command line options listed in section 7.2 of the OpenDDS\n"
    "  Developer's Guide are also available.\n"
    "  --help | -h               Print this message.\n"
    "  --version | -v            Print the program version.\n"
    "  -x                        This is option x\n"
    "  --y-opt-long-name STRING  This is option y, string\n"
    "  -y STRING                 This line should be next to -y STRING\n"
    "  --z-opt-long-name-that-should-be-on-its-own-line INT\n"
    "  -z INT\n"
    "                            This is option z, integer\n",
    out.str().c_str()
  );
}
