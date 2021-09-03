/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/RTPS/RtpsCoreC.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

// ---------- ---------- ---------- ---------- ----------
void set_Parameter_user_data(Parameter& p, const std::string& user_data)
{
  DDS::UserDataQosPolicy ud_qos;
  ud_qos.value.length(static_cast<CORBA::ULong>(user_data.length()));
  for (CORBA::ULong i = 0; i < ud_qos.value.length(); ++i) {
    ud_qos.value[i] = user_data[i];
  }
  p.user_data(ud_qos);
}

bool insert_Parameter_user_data(Serializer& s, const std::string& user_data)
{
  Parameter p;
  set_Parameter_user_data(p, user_data);
  return s << p;
}

void extract_Parameter_user_data(const std::string& user_data)
{
  const CORBA::ULong length = static_cast<CORBA::ULong>(user_data.length());
  ACE_Message_Block mb(length + 128);
  Serializer s(&mb, Encoding(Encoding::KIND_XCDR1));
  ASSERT_TRUE(insert_Parameter_user_data(s, user_data));

  Parameter p;
  ASSERT_TRUE(s >> p);

  ASSERT_EQ(p._d(), PID_USER_DATA);
  DDS::UserDataQosPolicy ud_qos = p.user_data();
  ASSERT_EQ(ud_qos.value.length(), length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    EXPECT_EQ(ud_qos.value[i], user_data[i]);
  }
}

// ---------- ---------- ---------- ---------- ----------
template <typename Seq, typename Element>
bool insert_sequence(Serializer& s, const std::vector<Element>& v)
{
  const CORBA::ULong length = static_cast<CORBA::ULong>(v.size());
  Seq seq;
  seq.length(length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    seq[i] = v[i];
  }
  return s << seq;
}

template <typename Seq, typename Element>
void extract_sequence(Serializer& s, const std::vector<Element>& v)
{
  const CORBA::ULong length = static_cast<CORBA::ULong>(v.size());
  Seq seq;
  ASSERT_TRUE(s >> seq);
  ASSERT_EQ(seq.length(), length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    EXPECT_EQ(seq[i], v[i]);
  }
}

template <typename Seq>
bool extract_sequence_with_unverified_length(const CORBA::ULong length = 0)
{
  const CORBA::ULong data[] = {length, 1000};
  ACE_Message_Block mb(512);
  mb.copy((const char*)data, sizeof(data));
  Serializer s(&mb, Encoding(Encoding::KIND_XCDR1));

  Seq seq;
  return s >> seq;
}

// ---------- ---------- ---------- ---------- ----------
void extract_FilterResult_t(const std::vector<CORBA::Long>& v)
{
  const CORBA::ULong length = static_cast<CORBA::ULong>(v.size());
  ACE_Message_Block mb(length * 32 + 128);
  Serializer s(&mb, Encoding(Encoding::KIND_XCDR1));
  ASSERT_TRUE((insert_sequence<FilterResult_t, CORBA::Long>(s, v)));

  extract_sequence<FilterResult_t, CORBA::Long>(s, v);
}

// ---------- ---------- ---------- ---------- ----------
TEST(RtpsCoreTypeSupportImpl, InsertParameter)
{
  ACE_Message_Block mb(512);
  Serializer s(&mb, Encoding(Encoding::KIND_XCDR1));
  EXPECT_TRUE(insert_Parameter_user_data(s, ""));
  EXPECT_TRUE(insert_Parameter_user_data(s, "insert_Parameter_user_data"));
}

TEST(RtpsCoreTypeSupportImpl, ExtractParameter)
{
  extract_Parameter_user_data("");
  extract_Parameter_user_data("extract_Parameter_user_data");
}

// ---------- ---------- ---------- ---------- ----------
TEST(RtpsCoreTypeSupportImpl, InsertFilterResult_t)
{
  std::vector<CORBA::Long> v;
  ACE_Message_Block mb(512);
  Serializer s(&mb, Encoding(Encoding::KIND_XCDR1));
  EXPECT_TRUE((insert_sequence<FilterResult_t, CORBA::Long>(s, v)));
  v.push_back(3);
  v.push_back(5);
  EXPECT_TRUE((insert_sequence<FilterResult_t, CORBA::Long>(s, v)));
}

TEST(RtpsCoreTypeSupportImpl, ExtractFilterResult_t)
{
  std::vector<CORBA::Long> v;
  extract_FilterResult_t(v);
  v.push_back(3);
  v.push_back(5);
  v.push_back(7);
  extract_FilterResult_t(v);
}

TEST(RtpsCoreTypeSupportImpl, ExtractSequenceWithUnverifiedLength)
{
  EXPECT_TRUE(extract_sequence_with_unverified_length<FilterResult_t>());
  EXPECT_FALSE(extract_sequence_with_unverified_length<FilterResult_t>(10));
}
