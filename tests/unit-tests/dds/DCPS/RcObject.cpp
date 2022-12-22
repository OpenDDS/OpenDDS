#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/RcObject.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

namespace {
  struct Counted : RcObject {
    Counted() {}
  };
}

TEST(dds_DCPS_RcObject, ctors_weak)
{
  RcHandle<Counted> h1 = make_rch<Counted>();
  WeakRcHandle<Counted> w1(h1);
  EXPECT_EQ(h1, w1);

  WeakRcHandle<Counted> w2;
  EXPECT_FALSE(w2);

  WeakRcHandle<Counted> w3(*h1);
  EXPECT_TRUE(w3);
}

TEST(dds_DCPS_RcObject, assign_weak)
{
  RcHandle<Counted> h1 = make_rch<Counted>();
  WeakRcHandle<Counted> w1, w2, w3;

  w1 = h1;
  EXPECT_EQ(h1, w1);

  w2 = w1;
  EXPECT_TRUE(w2);

  w3 = *h1;
  EXPECT_TRUE(w3);
}

TEST(dds_DCPS_RcObject, add_remove_ref_count)
{
  Counted c;
  EXPECT_EQ(c.ref_count(), 1);
  c._add_ref();
  EXPECT_EQ(c.ref_count(), 2);
  c._remove_ref();
  EXPECT_EQ(c.ref_count(), 1);
}

TEST(dds_DCPS_RcObject, lock_reset_weak)
{
  RcHandle<Counted> h1 = make_rch<Counted>();
  WeakRcHandle<Counted> w1(h1);
  RcHandle<Counted> h2 = w1.lock();
  EXPECT_EQ(h1, h2);
  w1.reset();
  EXPECT_FALSE(w1);
}

TEST(dds_DCPS_RcObject, lock_failed)
{
  WeakRcHandle<Counted> w1;
  {
    RcHandle<Counted> h1 = make_rch<Counted>();
    w1 = h1;
  }
  RcHandle<Counted> locked = w1.lock();
  EXPECT_TRUE(locked.is_nil());
}

TEST(dds_DCPS_RcObject, compare_weak)
{
  RcHandle<Counted> h1 = make_rch<Counted>();
  WeakRcHandle<Counted> w1(h1);
  WeakRcHandle<Counted> w2(w1);
  EXPECT_EQ(w1, w2);
  EXPECT_FALSE(w1 != w2);
  EXPECT_FALSE(w1 < w2);
  EXPECT_FALSE(w2 < w1);
}
