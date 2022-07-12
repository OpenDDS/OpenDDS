#include <dds/DCPS/RcHandle_T.h>

#include <gtest/gtest.h>

namespace {
  struct Count {
    mutable int c_;
    Count() : c_(1) {}
    virtual ~Count() {} // vtbl needed for test of dynamic_rchandle_cast
    void _add_ref() const { ++c_; }
    void _remove_ref() const { if (--c_ == 0) delete this; }
    int ref_count() const { return c_; } // only used in rchandle_from()
  };

  struct Derived : Count {};

  typedef OpenDDS::DCPS::RcHandle<Count> Handle;
}

TEST(dds_DCPS_RcHandle_T, ctors)
{
  Handle h1;
  EXPECT_TRUE(h1.is_nil());

  Handle h2(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h2.is_nil());

  OpenDDS::DCPS::unique_ptr<Count> u(new Count);
  EXPECT_TRUE(u);
  Handle h3(OpenDDS::DCPS::move(u));
  EXPECT_FALSE(h3.is_nil());

  Handle h4(h2.get(), OpenDDS::DCPS::inc_count());
  EXPECT_EQ(h4.get(), h2.get());

  OpenDDS::DCPS::RcHandle<Derived> h5(new Derived, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h5.is_nil());
  Handle h6(h5);
  EXPECT_EQ(h6.get(), h5.get());

  Handle h7(h6);
  EXPECT_EQ(h7.get(), h5.get());
}

TEST(dds_DCPS_RcHandle_T, dtor)
{
  Count c;
  {
    Handle h(&c, OpenDDS::DCPS::inc_count());
    EXPECT_EQ(c.c_, 2);
  }
  EXPECT_EQ(c.c_, 1);
}

TEST(dds_DCPS_RcHandle_T, reset)
{
  Handle h(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h.is_nil());
  h.reset();
  EXPECT_TRUE(h.is_nil());

  h.reset(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h.is_nil());
  h.reset();
  EXPECT_TRUE(h.is_nil());

  Count c;
  h.reset(&c, OpenDDS::DCPS::inc_count());
  EXPECT_FALSE(h.is_nil());
}

TEST(dds_DCPS_RcHandle_T, assign)
{
  Handle h(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h.is_nil());
  h = Handle(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h.is_nil());

  OpenDDS::DCPS::unique_ptr<Count> u(new Count);
  EXPECT_TRUE(u);
  h = OpenDDS::DCPS::move(u);
  EXPECT_FALSE(h.is_nil());

  OpenDDS::DCPS::RcHandle<Derived> h5(new Derived, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h5.is_nil());
  h = h5;
  EXPECT_EQ(h.get(), h5.get());
}

TEST(dds_DCPS_RcHandle_T, swaps)
{
  Handle h1(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h1.is_nil());
  Handle h2;
  EXPECT_TRUE(h2.is_nil());
  h1.swap(h2);
  EXPECT_TRUE(h1.is_nil());
  EXPECT_FALSE(h2.is_nil());

  swap(h2, h1);
  EXPECT_FALSE(h1.is_nil());
  EXPECT_TRUE(h2.is_nil());
}

TEST(dds_DCPS_RcHandle_T, access)
{
  Handle h1(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_EQ(h1->c_, 1);
  EXPECT_EQ((*h1).c_, 1);
  EXPECT_EQ(h1.get()->c_, 1);
}

namespace {
  Count* params(Count* /*in*/, Count*& inout, Count*& out)
  {
    Handle h(inout, OpenDDS::DCPS::keep_count());
    inout = new Count;
    out = new Count;
    return h._retn();
  }
}

TEST(dds_DCPS_RcHandle_T, param_passing)
{
  Handle h1(new Count, OpenDDS::DCPS::keep_count());
  Handle h2(h1);
  Handle h3;
  Handle h4(params(h1.in(), h2.inout(), h3.out()), OpenDDS::DCPS::keep_count());
  EXPECT_FALSE(h2.is_nil());
  EXPECT_FALSE(h3.is_nil());
  EXPECT_FALSE(h4.is_nil());
}

TEST(dds_DCPS_RcHandle_T, bool_conv)
{
  Handle h1(new Count, OpenDDS::DCPS::keep_count());
  Handle h2;
  EXPECT_TRUE(h1);
  EXPECT_FALSE(h2);
}

TEST(dds_DCPS_RcHandle_T, relational)
{
  Handle h1(new Count, OpenDDS::DCPS::keep_count());
  Handle h2(h1);
  EXPECT_TRUE(h1 == h2);
  h2.reset();
  EXPECT_TRUE(h1 != h2);
  h2.reset(new Count, OpenDDS::DCPS::keep_count());
  EXPECT_TRUE(h1.get() < h2.get() ? h1 < h2 : h2 < h1);
}

TEST(dds_DCPS_RcHandle_T, casts)
{
  Handle hc(new Derived, OpenDDS::DCPS::keep_count());
  OpenDDS::DCPS::RcHandle<Derived> hd = OpenDDS::DCPS::static_rchandle_cast<Derived>(hc);
  EXPECT_TRUE(hd);
  hd = OpenDDS::DCPS::dynamic_rchandle_cast<Derived>(hc);
  EXPECT_TRUE(hd);
  OpenDDS::DCPS::RcHandle<const Count> const_handle(new Count, OpenDDS::DCPS::keep_count());
  hc = OpenDDS::DCPS::const_rchandle_cast<Count>(const_handle);
  EXPECT_TRUE(hc);
}

TEST(dds_DCPS_RcHandle_T, make_rch)
{
  Handle h = OpenDDS::DCPS::make_rch<Count>();
  EXPECT_TRUE(h);
}

TEST(dds_DCPS_RcHandle_T, rchandle_from)
{
  Handle h = OpenDDS::DCPS::make_rch<Count>();
  Handle h2 = OpenDDS::DCPS::rchandle_from(h.get());
  EXPECT_TRUE(h2 && h == h2);
}
