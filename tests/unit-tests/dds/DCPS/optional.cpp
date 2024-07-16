#include <dds/DCPS/optional.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

template <typename T>
struct StaticCounter {
  static int count;
  T value;

  StaticCounter(T v)
    : value(v)
  {
    ++count;
  }

  StaticCounter(const StaticCounter& other)
    : value(other.value)
  {
    ++count;
  }

  template <typename U>
  StaticCounter(StaticCounter<U> u)
    : value(u.value)
  {
    ++count;
  }

  ~StaticCounter()
  {
    --count;
  }

  bool operator==(const StaticCounter& other) const
  {
    return value == other.value;
  }

  bool operator<(const StaticCounter& other) const
  {
    return value < other.value;
  }
};

template <>
int StaticCounter<int>::count = 0;
template <>
int StaticCounter<char>::count = 0;

typedef StaticCounter<int> IntCounter;
typedef StaticCounter<char> CharCounter;

TEST(dds_DCPS_optional, default_ctor)
{
  optional<IntCounter> x;
  EXPECT_FALSE(x.has_value());
  EXPECT_FALSE(x);
  EXPECT_EQ(IntCounter::count, 0);
  EXPECT_THROW(x.value(), std::runtime_error);
}

TEST(dds_DCPS_optional, value_ctor)
{
  {
    optional<IntCounter> x(IntCounter(5));
    EXPECT_TRUE(x.has_value());
    EXPECT_TRUE(x);
    EXPECT_EQ(x.value().value, 5);
    EXPECT_EQ(x->value, 5);
    x->value = 6;
    EXPECT_EQ(x->value, 6);
    EXPECT_EQ((*x).value, 6);
    (*x).value = 7;
    EXPECT_EQ((*x).value, 7);
    EXPECT_EQ(IntCounter::count, 1);
  }
  EXPECT_EQ(IntCounter::count, 0);
}

TEST(dds_DCPS_optional, copy_ctor)
{
  optional<IntCounter> x;
  optional<IntCounter> y(x);
  EXPECT_FALSE(y.has_value());
  EXPECT_FALSE(y);
  EXPECT_EQ(IntCounter::count, 0);
  {
    optional<IntCounter> x(IntCounter(5));
    optional<IntCounter> y(x);
    EXPECT_TRUE(y.has_value());
    EXPECT_TRUE(y);
    EXPECT_EQ(y.value().value, 5);
    EXPECT_EQ(IntCounter::count, 2);
  }
  EXPECT_EQ(IntCounter::count, 0);
}

TEST(dds_DCPS_optional, copy_convert_ctor)
{
  optional<CharCounter> x;
  optional<IntCounter> y(x);
  EXPECT_FALSE(y.has_value());
  EXPECT_FALSE(y);
  EXPECT_EQ(IntCounter::count, 0);
  EXPECT_EQ(CharCounter::count, 0);
  {
    optional<CharCounter> x(CharCounter(5));
    optional<IntCounter> y(x);
    EXPECT_TRUE(y.has_value());
    EXPECT_TRUE(y);
    EXPECT_EQ(y.value().value, 5);
    EXPECT_EQ(IntCounter::count, 1);
    EXPECT_EQ(CharCounter::count, 1);
  }
  EXPECT_EQ(IntCounter::count, 0);
  EXPECT_EQ(CharCounter::count, 0);
}

TEST(dds_DCPS_optional, value_assign)
{
  optional<IntCounter> x;
  x = 5;
  EXPECT_TRUE(x.has_value());
  EXPECT_TRUE(x);
  EXPECT_EQ(x.value().value, 5);
  EXPECT_EQ(IntCounter::count, 1);
}

TEST(dds_DCPS_optional, copy_assign)
{
  optional<IntCounter> x;
  optional<IntCounter> y(IntCounter(5));
  x = y;
  EXPECT_TRUE(x.has_value());
  EXPECT_TRUE(x);
  EXPECT_EQ(x.value().value, 5);
  EXPECT_EQ(IntCounter::count, 2);
}

TEST(dds_DCPS_optional, copy_convert_assign)
{
  optional<CharCounter> x;
  optional<IntCounter> y;
  y = x;
  EXPECT_FALSE(y.has_value());
  EXPECT_FALSE(y);
  EXPECT_EQ(IntCounter::count, 0);
  EXPECT_EQ(CharCounter::count, 0);
  {
    optional<CharCounter> x(CharCounter(5));
    optional<IntCounter> y;
    y = x;
    EXPECT_TRUE(y.has_value());
    EXPECT_TRUE(y);
    EXPECT_EQ(y.value().value, 5);
    EXPECT_EQ(IntCounter::count, 1);
    EXPECT_EQ(CharCounter::count, 1);
  }
  EXPECT_EQ(IntCounter::count, 0);
  EXPECT_EQ(CharCounter::count, 0);
}

TEST(dds_DCPS_optional, swap)
{
  optional<IntCounter> x(5);
  optional<IntCounter> y(6);
  x.swap(y);
  EXPECT_TRUE(x);
  EXPECT_TRUE(y);
  EXPECT_EQ(x->value, 6);
  EXPECT_EQ(y->value, 5);
  swap(x, y);
  EXPECT_TRUE(x);
  EXPECT_TRUE(y);
  EXPECT_EQ(x->value, 5);
  EXPECT_EQ(y->value, 6);
}

TEST(dds_DCPS_optional, reset)
{
  optional<IntCounter> x(5);
  EXPECT_EQ(IntCounter::count, 1);
  x.reset();
  EXPECT_EQ(IntCounter::count, 0);
}

TEST(dds_DCPS_optional, equal)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_TRUE(a == b);
  EXPECT_TRUE(x == y);
  EXPECT_FALSE(a == x);
  EXPECT_FALSE(x == z);
}

TEST(dds_DCPS_optional, not_equal)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(x != y);
  EXPECT_TRUE(a != x);
  EXPECT_TRUE(x != z);
}

TEST(dds_DCPS_optional, less_than)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(x < y);
  EXPECT_TRUE(a < x);
  EXPECT_TRUE(x < z);
  EXPECT_FALSE(b < a);
  EXPECT_FALSE(y < x);
  EXPECT_FALSE(x < a);
  EXPECT_FALSE(z < x);
}

TEST(dds_DCPS_optional, less_than_equal)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(x <= y);
  EXPECT_TRUE(a <= x);
  EXPECT_TRUE(x <= z);
  EXPECT_TRUE(b <= a);
  EXPECT_TRUE(y <= x);
  EXPECT_FALSE(x <= a);
  EXPECT_FALSE(z <= x);
}

TEST(dds_DCPS_optional, greater_than)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_FALSE(a > b);
  EXPECT_FALSE(x > y);
  EXPECT_FALSE(a > x);
  EXPECT_FALSE(x > z);
  EXPECT_FALSE(b > a);
  EXPECT_FALSE(y > x);
  EXPECT_TRUE(x > a);
  EXPECT_TRUE(z > x);
}

TEST(dds_DCPS_optional, greater_than_equal)
{
  optional<IntCounter> a;
  optional<IntCounter> b;
  optional<IntCounter> x(5);
  optional<IntCounter> y(5);
  optional<IntCounter> z(6);
  EXPECT_TRUE(a >= b);
  EXPECT_TRUE(x >= y);
  EXPECT_FALSE(a >= x);
  EXPECT_FALSE(x >= z);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(y >= x);
  EXPECT_TRUE(x >= a);
  EXPECT_TRUE(z >= x);
}

TEST(dds_DCPS_optional, equal_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_FALSE(a == y);
  EXPECT_FALSE(a == z);
  EXPECT_TRUE(x == y);
  EXPECT_FALSE(x == z);
  EXPECT_FALSE(y == a);
  EXPECT_FALSE(z == a);
  EXPECT_TRUE(y == x);
  EXPECT_FALSE(z == x);
}

TEST(dds_DCPS_optional, not_equal_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_TRUE(a != y);
  EXPECT_TRUE(a != z);
  EXPECT_FALSE(x != y);
  EXPECT_TRUE(x != z);
  EXPECT_TRUE(y != a);
  EXPECT_TRUE(z != a);
  EXPECT_FALSE(y != x);
  EXPECT_TRUE(z != x);
}

TEST(dds_DCPS_optional, less_than_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_TRUE(a < y);
  EXPECT_TRUE(a < z);
  EXPECT_FALSE(x < y);
  EXPECT_TRUE(x < z);
  EXPECT_FALSE(y < a);
  EXPECT_FALSE(z < a);
  EXPECT_FALSE(y < x);
  EXPECT_FALSE(z < x);
}

TEST(dds_DCPS_optional, less_than_equal_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_TRUE(a <= y);
  EXPECT_TRUE(a <= z);
  EXPECT_TRUE(x <= y);
  EXPECT_TRUE(x <= z);
  EXPECT_FALSE(y <= a);
  EXPECT_FALSE(z <= a);
  EXPECT_TRUE(y <= x);
  EXPECT_FALSE(z <= x);
}

TEST(dds_DCPS_optional, greater_than_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_FALSE(a > y);
  EXPECT_FALSE(a > z);
  EXPECT_FALSE(x > y);
  EXPECT_FALSE(x > z);
  EXPECT_FALSE(y > a);
  EXPECT_FALSE(z > a);
  EXPECT_TRUE(y > x);
  EXPECT_FALSE(z > x);
}

TEST(dds_DCPS_optional, greater_than_equal_value)
{
  optional<IntCounter> a;
  optional<IntCounter> x(5);
  IntCounter y(5);
  IntCounter z(6);
  EXPECT_FALSE(a >= y);
  EXPECT_FALSE(a >= z);
  EXPECT_TRUE(x >= y);
  EXPECT_FALSE(x >= z);
  EXPECT_FALSE(y >= a);
  EXPECT_FALSE(z >= a);
  EXPECT_TRUE(y >= x);
  EXPECT_FALSE(z >= x);
}
