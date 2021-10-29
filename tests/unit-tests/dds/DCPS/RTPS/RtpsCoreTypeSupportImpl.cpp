/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <dds/DCPS/Serializer.h>

#include <dds/DCPS/RTPS/RtpsCoreC.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

TEST(RtpsCoreTypeSupportImpl, PropertyQosPolicy)
{
  DDS::PropertySeq pseq(1);
  pseq.length(1);
  pseq[0].name = "test";
  pseq[0].value = "test";
  pseq[0].propagate = true;

  DDS::BinaryPropertySeq bseq(1);
  bseq.length(1);
  bseq[0].name = "test";
  bseq[0].value.length(1);
  bseq[0].value[0] = 99;
  bseq[0].propagate = true;

  const DDS::PropertyQosPolicy pqos = {pseq, bseq};
  Parameter param;
  param.property(pqos);

  static const Encoding enc(Encoding::KIND_XCDR1);
  const size_t size = serialized_size(enc, param) + 3 /*serializer may add padding*/;

  {
    ACE_Message_Block mb_property_qos_security(size);
    Serializer ser_in(&mb_property_qos_security, enc);
    ASSERT_TRUE(ser_in << param);

    Serializer ser_out(&mb_property_qos_security, enc);
    Parameter out_param;
    ASSERT_TRUE(ser_out >> out_param);
    ASSERT_EQ(out_param._d(), PID_PROPERTY_LIST);
    const DDS::PropertyQosPolicy& out_qos = out_param.property();
    ASSERT_EQ(out_qos.value.length(), 1u);
    ASSERT_STREQ(out_qos.value[0].name, "test");
    ASSERT_STREQ(out_qos.value[0].value, "test");
    ASSERT_EQ(out_qos.binary_value.length(), 1u);
    ASSERT_STREQ(out_qos.binary_value[0].name, "test");
    ASSERT_EQ(out_qos.binary_value[0].value.length(), 1u);
    ASSERT_EQ(out_qos.binary_value[0].value[0], 99);
  }

  // In the RTPS spec, PropertyQosPolicy has just the PropertySeq
  // (doesn't have the BinaryPropertySeq), so serializing the PropertySeq
  // is equivalent to that definition of the PropertyQosPolicy
  {
    ACE_Message_Block mb_property_qos_rtps(size);
    Serializer ser(&mb_property_qos_rtps, enc);
    ASSERT_TRUE(ser << param._d());
    const size_t size_in_param = serialized_size(enc, pseq);
    const size_t post_pad = 4 - (size_in_param % 4);
    const size_t total_size = size_in_param + ((post_pad < 4) ? post_pad : 0);
    ASSERT_TRUE(ser << ACE_CDR::UShort(total_size));
    ASSERT_TRUE(ser << pseq);
    if (post_pad < 4) {
      static const ACE_CDR::Octet padding[3] = {0};
      ASSERT_TRUE(ser.write_octet_array(padding, ACE_CDR::ULong(post_pad)));
    }

    Serializer ser_out(&mb_property_qos_rtps, enc);
    Parameter out_param;
    ASSERT_TRUE(ser_out >> out_param);
    ASSERT_EQ(out_param._d(), PID_PROPERTY_LIST);
    const DDS::PropertyQosPolicy& out_qos = out_param.property();
    ASSERT_EQ(out_qos.value.length(), 1u);
    ASSERT_STREQ(out_qos.value[0].name, "test");
    ASSERT_STREQ(out_qos.value[0].value, "test");
    ASSERT_EQ(out_qos.binary_value.length(), 0u);
  }
}


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
  ACE_Message_Block mb(1024);
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
