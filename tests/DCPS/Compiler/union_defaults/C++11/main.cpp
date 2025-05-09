#include "union_defaultsTypeSupportImpl.h"

#include <gtest/gtest.h>

#include <dds/DCPS/Definitions.h>

TEST(UnionDefault, no_default)
{
  Unions::Z z;
  OpenDDS::DCPS::set_default(z);
  EXPECT_TRUE(z._d() == 0);
}

TEST(UnionDefault, string)
{
  Unions::Y y;
  OpenDDS::DCPS::set_default(y);
  EXPECT_TRUE(y._d() == 0);
}

TEST(UnionDefault, Z)
{
  Unions::X x;
  OpenDDS::DCPS::set_default(x);
  EXPECT_TRUE(x._d() == 0);
}

TEST(UnionDefault, dummy)
{
  Unions::V v;
  OpenDDS::DCPS::set_default(v);
  EXPECT_TRUE(v._d() == 0);
}

TEST(UnionDefault, dummy_sequence)
{
  Unions::U u;
  OpenDDS::DCPS::set_default(u);
  EXPECT_TRUE(u._d() == 0);
}

TEST(UnionDefault, dummy_array)
{
  Unions::T t;
  OpenDDS::DCPS::set_default(t);
  EXPECT_TRUE(t._d() == 0);
}

#if !OPENDDS_CONFIG_SAFETY_PROFILE
TEST(UnionDefault, wstring)
{
  Unions::S s;
  OpenDDS::DCPS::set_default(s);
  EXPECT_TRUE(s._d() == 0);
}

TEST(UnionDefault, wchar)
{
  Unions::R r;
  OpenDDS::DCPS::set_default(r);
  EXPECT_TRUE(r._d() == 0);
}
#endif

TEST(UnionDefault, long_double)
{
  Unions::Q q;
  OpenDDS::DCPS::set_default(q);
  EXPECT_TRUE(q._d() == 0);
}

TEST(UnionDefault, boolean)
{
  Unions::P p;
  OpenDDS::DCPS::set_default(p);
  EXPECT_TRUE(p._d() == 0);
}

TEST(UnionDefault, enum)
{
  Unions::O o;
  OpenDDS::DCPS::set_default(o);
  EXPECT_TRUE(o._d() == Unions::Dog::Mastiff);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
