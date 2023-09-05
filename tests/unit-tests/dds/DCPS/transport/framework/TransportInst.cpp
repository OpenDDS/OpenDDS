#include <dds/DCPS/transport/framework/TransportInst.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

struct MockTransportInst {

  MockTransportInst()
    : my_int_(*this, &MockTransportInst::my_int, &MockTransportInst::my_int)
    , my_int__(0)
    , my_string_(*this, &MockTransportInst::my_string, &MockTransportInst::my_string)
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
};

TEST(dds_DCPS_transport_framework_ConfigValue, assign_read)
{
  MockTransportInst mtt;

  mtt.my_int_ = 39;
  EXPECT_EQ(mtt.my_int__, 39);
  EXPECT_EQ(mtt.my_int_, 39);
}

TEST(dds_DCPS_transport_framework_ConfigValueRef, assign_read)
{
  const String a_string("a string");
  MockTransportInst mtt;

  mtt.my_string_ = a_string;
  EXPECT_EQ(mtt.my_string__, a_string);
  const String b_string = mtt.my_string_;
  EXPECT_EQ(b_string, a_string);
}
