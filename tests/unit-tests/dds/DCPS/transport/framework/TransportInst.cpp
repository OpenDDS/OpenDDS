#include <dds/DCPS/transport/framework/TransportInst.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

struct MockTransportInst {

  MockTransportInst()
    : my_int_(*this, &MockTransportInst::my_int, &MockTransportInst::my_int)
    , my_int__(0)
    , my_string_(*this, &MockTransportInst::my_string, &MockTransportInst::my_string)
    , my_int2_(*this, &MockTransportInst::my_int2, &MockTransportInst::my_int2)
    , my_int2__(0)
    , my_string2_(*this, &MockTransportInst::my_string2, &MockTransportInst::my_string2)
  {}

  ConfigValue<MockTransportInst, int> my_int_;
  void my_int(int x)
  {
    my_int__ = x;
  }

  int my_int() const
  {
    return my_int__;
  }

  int my_int__;

  ConfigValueRef<MockTransportInst, String> my_string_;
  void my_string(const String& x)
  {
    my_string__ = x;
  }

  String my_string() const
  {
    return my_string__;
  }

  String my_string__;

  ConfigValue<MockTransportInst, int> my_int2_;
  void my_int2(int x)
  {
    my_int2__ = x;
  }

  int my_int2() const
  {
    return my_int2__;
  }

  int my_int2__;

  ConfigValueRef<MockTransportInst, String> my_string2_;
  void my_string2(const String& x)
  {
    my_string2__ = x;
  }

  String my_string2() const
  {
    return my_string2__;
  }

  String my_string2__;
};

TEST(dds_DCPS_transport_framework_ConfigValue, assign_read)
{
  MockTransportInst mtt;

  mtt.my_int_ = 39;
  EXPECT_EQ(mtt.my_int__, 39);
  EXPECT_EQ(mtt.my_int_, 39);
  mtt.my_int2_ = mtt.my_int_;
  EXPECT_EQ(mtt.my_int2_, 39);
}

TEST(dds_DCPS_transport_framework_ConfigValueRef, assign_read)
{
  const String a_string("a string");
  MockTransportInst mtt;

  mtt.my_string_ = a_string;
  EXPECT_EQ(mtt.my_string__, a_string);
  const String b_string = mtt.my_string_;
  EXPECT_EQ(b_string, a_string);
  mtt.my_string2_ = mtt.my_string_;
  const String c_string = mtt.my_string2_;
  EXPECT_EQ(c_string, a_string);
}
