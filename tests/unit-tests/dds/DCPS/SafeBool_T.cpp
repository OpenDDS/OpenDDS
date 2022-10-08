#include <dds/DCPS/SafeBool_T.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {
  class VirtualMethod : public SafeBool_T<> {
  public:
    virtual bool boolean_test() const
    {
      return false;
    }
  };

  class DerivedVirtualMethod : public VirtualMethod {
  public:
    virtual bool boolean_test() const
    {
      return true;
    }
  };

  class NonVirtualMethod : public SafeBool_T<NonVirtualMethod> {
  public:
    bool value_;

    NonVirtualMethod(bool value)
    : value_(value)
    {
    }

    bool boolean_test() const
    {
      return value_;
    }
  };
}

TEST(dds_DCPS_SafeBool_T, virtual_method)
{
  EXPECT_FALSE(VirtualMethod());
  DerivedVirtualMethod dvm;
  EXPECT_TRUE(dvm);
  EXPECT_TRUE(dynamic_cast<VirtualMethod&>(dvm));
}

TEST(dds_DCPS_SafeBool_T, non_virtual_method)
{
  EXPECT_TRUE(NonVirtualMethod(true));
  EXPECT_FALSE(NonVirtualMethod(false));
}
