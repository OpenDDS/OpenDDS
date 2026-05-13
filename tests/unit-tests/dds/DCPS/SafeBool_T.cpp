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

// Implicit test case: these comparisons should not compile since
// SafeBool_T explicitly prevents them.
//
// TEST(dds_DCPS_SafeBool_T, comparisons_not_supported)
// {
//   VirtualMethod vm1, vm2;
//   if (vm1 == vm2) {}
//   if (vm1 != vm2) {}

//   DerivedVirtualMethod dvm1, dvm2;
//   if (dvm1 == dvm2) {}
//   if (dvm1 != dvm2) {}

//   NonVirtualMethod nvm1(true), nvm2(true);
//   if (nvm1 == nvm2) {}
//   if (nvm1 != nvm2) {}

//   if (vm1 == dvm1) {}
//   if (dvm1 == nvm1) {}
//   if (nvm1 == vm1) {}
// }
