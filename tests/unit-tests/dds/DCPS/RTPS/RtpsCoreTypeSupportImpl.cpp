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

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

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

void extract_Parameter_user_data(const std::string& user_data, const Encoding encoding = Encoding(Encoding::KIND_XCDR2))
{
  const CORBA::ULong length = static_cast<CORBA::ULong>(user_data.length());
  ACE_Message_Block mb(length + 128);
  Serializer s(&mb, encoding);
  ASSERT_TRUE(insert_Parameter_user_data(s, user_data));

  Parameter p;
  ASSERT_TRUE(s >> p);

  DDS::UserDataQosPolicy ud_qos = p.user_data();
  EXPECT_EQ(ud_qos.value.length(), length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    EXPECT_EQ(ud_qos.value[i], user_data[i]);
  }
}

TEST(RtpsCoreTypeSupportImpl, InsertParameter)
{
  ACE_Message_Block mb(512);
  const Encoding encoding(Encoding::KIND_XCDR2);
  Serializer s(&mb, encoding);
  EXPECT_TRUE(insert_Parameter_user_data(s, ""));
  EXPECT_TRUE(insert_Parameter_user_data(s, "insert_Parameter_user_data"));
}

TEST(RtpsCoreTypeSupportImpl, ExtractParameter)
{
  extract_Parameter_user_data("");
  extract_Parameter_user_data("extract_Parameter_user_data");
}
