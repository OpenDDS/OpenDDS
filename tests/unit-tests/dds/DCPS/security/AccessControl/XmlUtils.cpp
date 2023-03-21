#ifdef OPENDDS_SECURITY

#include <dds/DCPS/security/AccessControl/XmlUtils.h>
#include <dds/DCPS/debug.h>

#include <gtest/gtest.h>

using namespace OpenDDS::Security::XmlUtils;
using XML::XStr;
using OpenDDS::DCPS::security_debug;
using OpenDDS::Security::DomainIdSet;
using OpenDDS::Security::domain_id_max;

TEST(dds_DCPS_security_AccessControl_XmlUtils, get_parser)
{
  ParserPtr p;
  EXPECT_FALSE(get_parser(p, "empty string", ""));
  EXPECT_FALSE(p);
  EXPECT_FALSE(get_parser(p, "invalid xml", ">What I'm writing isn't valid XML<"));
  EXPECT_FALSE(p);
  EXPECT_TRUE(get_parser(p, "valid xml",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<root>\n"
    "  <content>Hello!</content>"
    "</root>\n"));
  EXPECT_TRUE(p);
}

namespace {
  void parse_bool_checker(ParserPtr& parser, const ACE_TCHAR* tag,
    bool expected_valid, bool expected_value = false)
  {
    const xercesc::DOMNodeList* const nodes =
      parser->getDocument()->getElementsByTagName(XStr(tag));
    const XMLSize_t count = nodes->getLength();
    ASSERT_EQ(count, 3UL) << "node tag name is " << ACE_TEXT_ALWAYS_CHAR(tag);
    for (XMLSize_t i = 0; i < count; ++i) {
      const xercesc::DOMNode* const node = nodes->item(i);
      bool value;
      const bool valid = parse_bool(node, value);
      EXPECT_EQ(valid, expected_valid)
        << "node contains: \"" << to_string(node) << "\"";
      if (expected_valid && valid) {
        EXPECT_EQ(value, expected_value)
          << "node contains: \"" << to_string(node) << "\"";
      }
    }
  }
}

TEST(dds_DCPS_security_AccessControl_XmlUtils, parse_bool)
{
  ParserPtr parser;
  ASSERT_TRUE(get_parser(parser, "parse_bool",
    "<root>\n"
    "  <invalid></invalid>\n"
    "  <invalid>YES</invalid>\n"
    "  <invalid>invalid</invalid>\n"

    "  <true>TRUE</true>\n"
    "  <true>true</true>\n"
    "  <true>1</true>\n"

    "  <false>FALSE</false>\n"
    "  <false>false</false>\n"
    "  <false>0</false>\n"
    "</root>\n"));
  ASSERT_TRUE(parser);

  parse_bool_checker(parser, ACE_TEXT("invalid"), false);
  parse_bool_checker(parser, ACE_TEXT("true"), true, true);
  parse_bool_checker(parser, ACE_TEXT("false"), true, false);
}

namespace {
  void parse_time_checker(ParserPtr& parser, const ACE_TCHAR* tag,
    bool expected_valid, time_t expected_value = 0)
  {
    const xercesc::DOMNodeList* const nodes =
      parser->getDocument()->getElementsByTagName(XStr(tag));
    const XMLSize_t count = nodes->getLength();
    ASSERT_EQ(count, 1UL) << "node tag name is " << ACE_TEXT_ALWAYS_CHAR(tag);
    const xercesc::DOMNode* const node = nodes->item(0);
    time_t value;
    const bool valid = parse_time(node, value);
    EXPECT_EQ(valid, expected_valid)
      << "node contains: \"" << to_string(node) << "\"";
    if (expected_valid && valid) {
      EXPECT_EQ(value, expected_value)
        << "node contains: \"" << to_string(node) << "\"";
    }
  }
}

TEST(dds_DCPS_security_AccessControl_XmlUtils, parse_time)
{
  ParserPtr parser;
  ASSERT_TRUE(get_parser(parser, "parse_time",
    "<root>\n"
    "  <invalid1></invalid1>\n"
    "  <invalid2>2001/9/9</invalid2>\n"
    "  <invalid3>2001-09-09T01:46:40Z+00:00</invalid3>\n"
    "  <leftover>2001-09-09T01:46:40.000+00:00123hello</leftover>\n"

    "  <valid>2001-09-09T01:46:40</valid>\n"
    "  <valid_z>2001-09-09T01:46:40Z</valid_z>\n"
    "  <valid_0>2001-09-09T01:46:40+00:00</valid_0>\n"
    "  <valid_frac_sec>2001-09-09T01:46:40.999</valid_frac_sec>\n"
    "  <valid_frac_sec_z>2001-09-09T01:46:40.999Z</valid_frac_sec_z>\n"
    "  <valid_frac_sec_0>2001-09-09T01:46:40.999+00:00</valid_frac_sec_0>\n"

    "  <tz_plus_one>2001-09-09T02:46:40+01:00</tz_plus_one>\n"
    "  <tz_minus_one>2001-09-09T00:46:40-01:00</tz_minus_one>\n"
    "  <tz_carry>1999-12-31T23:00:00-01:00</tz_carry>\n"
    "  <valid_plus_1min>2001-09-09T01:46:40+00:01</valid_plus_1min>\n"
    "  <valid_minus_1min>2001-09-09T01:46:40-00:01</valid_minus_1min>\n"
    "</root>\n"));
  ASSERT_TRUE(parser);

  parse_time_checker(parser, ACE_TEXT("invalid1"), false);
  parse_time_checker(parser, ACE_TEXT("invalid2"), false);
  parse_time_checker(parser, ACE_TEXT("invalid3"), false);
  parse_time_checker(parser, ACE_TEXT("leftover"), false);

  OpenDDS::DCPS::LogRestore log_restore;
  security_debug.access_warn = true;
  security_debug.access_error = true;

  parse_time_checker(parser, ACE_TEXT("valid"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("valid_z"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("valid_0"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("valid_frac_sec"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("valid_frac_sec_z"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("valid_frac_sec_0"), true, 1000000000);

  parse_time_checker(parser, ACE_TEXT("tz_plus_one"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("tz_minus_one"), true, 1000000000);
  parse_time_checker(parser, ACE_TEXT("tz_carry"), true, 946684800);
  parse_time_checker(parser, ACE_TEXT("valid_plus_1min"), true, 999999940);
  parse_time_checker(parser, ACE_TEXT("valid_minus_1min"), true, 1000000060);
}

namespace {
  void parse_domain_id_set_checker(ParserPtr& parser, const ACE_TCHAR* tag,
    bool expected_valid, const DomainIdSet& expected_value, DomainIdSet& value)
  {
    const xercesc::DOMNodeList* const nodes =
      parser->getDocument()->getElementsByTagName(XStr(tag));
    const XMLSize_t count = nodes->getLength();
    ASSERT_EQ(count, 1UL) << "node tag name is " << ACE_TEXT_ALWAYS_CHAR(tag);
    const xercesc::DOMNode* const node = nodes->item(0);
    const bool valid = parse_domain_id_set(node, value);
    EXPECT_EQ(valid, expected_valid)
      << "node tag name is " << ACE_TEXT_ALWAYS_CHAR(tag);
    if (expected_valid && valid) {
      EXPECT_EQ(value, expected_value)
        << "node tag name is " << ACE_TEXT_ALWAYS_CHAR(tag);
    }
  }

  void parse_domain_id_set_checker(ParserPtr& parser, const ACE_TCHAR* tag)
  {
    const DomainIdSet nilset;
    DomainIdSet value;
    parse_domain_id_set_checker(parser, tag, false, nilset, value);
  }
}

TEST(dds_DCPS_security_AccessControl_XmlUtils, parse_domain_id_set)
{
  ParserPtr parser;
  ASSERT_TRUE(get_parser(parser, "parse_domain_id_set",
    "<root>\n"
    "  <empty></empty>\n"

    "  <negative>\n"
    "    <id>-1</id>\n"
    "  </negative>\n"

    "  <invalid_tag>\n"
    "    <id>1</id>\n"
    "    <invalidtag>1</invalidtag>\n"
    "  </invalid_tag>\n"

    "  <invalid_tag_in_range>\n"
    "    <id>1</id>\n"
    "    <id_range>\n"
    "      <invalidtag>1</invalidtag>\n"
    "    </id_range>\n"
    "  </invalid_tag_in_range>\n"

    "  <invalid_range>\n"
    "    <id>1</id>\n"
    "    <id_range>\n"
    "      <min>6</min>\n"
    "      <max>3</max>\n"
    "    </id_range>\n"
    "  </invalid_range>\n"

    "  <has_1>\n"
    "    <id>1</id>\n"
    "  </has_1>\n"

    "  <has_1_3>\n"
    "    <id>1</id>\n"
    "    <id>3</id>\n"
    "  </has_1_3>\n"

    "  <has_1_3_to_6>\n"
    "    <id>1</id>\n"
    "    <id_range>\n"
    "      <min>3</min>\n"
    "      <max>6</max>\n"
    "    </id_range>\n"
    "  </has_1_3_to_6>\n"

    "  <has_1_to_3_5_to_6>\n"
    "    <id_range>\n"
    "      <min>1</min>\n"
    "      <max>3</max>\n"
    "    </id_range>\n"
    "    <id_range>\n"
    "      <min>5</min>\n"
    "      <max>6</max>\n"
    "    </id_range>\n"
    "  </has_1_to_3_5_to_6>\n"

    "  <has_2_to_4_6_to_all>\n"
    "    <id_range>\n"
    "      <min>2</min>\n"
    "      <max>4</max>\n"
    "    </id_range>\n"
    "    <id_range>\n"
    "      <min>6</min>\n"
    "    </id_range>\n"
    "  </has_2_to_4_6_to_all>\n"

    "  <has_to_7>\n"
    "    <id_range>\n"
    "      <max>7</max>\n"
    "    </id_range>\n"
    "  </has_to_7>\n"

    "  <has_all>\n"
    "    <id_range>\n"
    "      <min>0</min>\n"
    "    </id_range>\n"
    "  </has_all>\n"
    "</root>\n"));
  ASSERT_TRUE(parser);

  parse_domain_id_set_checker(parser, ACE_TEXT("empty"));
  parse_domain_id_set_checker(parser, ACE_TEXT("negative"));
  parse_domain_id_set_checker(parser, ACE_TEXT("invalid_tag"));
  parse_domain_id_set_checker(parser, ACE_TEXT("invalid_tag_in_range"));
  parse_domain_id_set_checker(parser, ACE_TEXT("invalid_range"));

  OpenDDS::DCPS::LogRestore log_restore;
  security_debug.access_warn = true;
  security_debug.access_error = true;

  {
    DomainIdSet expected, value;
    expected.add(1);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_1"), true, expected, value);
    EXPECT_TRUE(value.has(1));
    EXPECT_FALSE(value.has(2));
  }

  {
    DomainIdSet expected, value;
    expected.add(1);
    expected.add(3);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_1_3"), true, expected, value);
  }

  {
    DomainIdSet expected, value;
    expected.add(1);
    expected.add(3, 6);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_1_3_to_6"), true, expected, value);
    EXPECT_FALSE(value.has(2));
  }

  {
    DomainIdSet expected, value;
    expected.add(1, 3);
    expected.add(5, 6);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_1_to_3_5_to_6"), true, expected, value);
  }

  {
    DomainIdSet expected, value;
    expected.add(2, 4);
    expected.add(6, domain_id_max);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_2_to_4_6_to_all"), true, expected, value);
    EXPECT_FALSE(value.has(5));
    EXPECT_TRUE(value.has(12345));
  }

  {
    DomainIdSet expected, value;
    expected.add(0, 7);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_to_7"), true, expected, value);
  }

  {
    DomainIdSet expected, value;
    expected.add(0, domain_id_max);
    parse_domain_id_set_checker(parser, ACE_TEXT("has_all"), true, expected, value);
    EXPECT_TRUE(value.has(1));
    EXPECT_TRUE(value.has(99999));
  }
}

#endif // OPENDDS_SECURITY
