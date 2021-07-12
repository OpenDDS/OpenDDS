#include <dds/DCPS/AddressCache.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS::DCPS;

struct TestKey {
  TestKey(const RepoId& from, const RepoId& to) : from_(from), to_(to) {}
  bool operator<(const TestKey& rhs) const {
    return std::memcmp(this, &rhs, sizeof (TestKey)) < 0;
  }
  bool contains(const RepoId& id) const {
    return from_ == id || to_ == id;
  }
  RepoId from_;
  RepoId to_;
};

TEST(address_cache_test, load_fail)
{
  AddressCache<TestKey> test_cache_;
  AddrSet addrs;
  ASSERT_FALSE(test_cache_.load(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs));
}

TEST(address_cache_test, store_load_success)
{
  AddressCache<TestKey> test_cache_;
  AddrSet addrs;
  addrs.insert(ACE_INET_Addr("127.0.0.1:1234"));

  test_cache_.store(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs);
  addrs.clear();

  ASSERT_TRUE(test_cache_.load(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs));
  ASSERT_EQ(addrs.size(), 1u);
  ASSERT_EQ(*(addrs.begin()),ACE_INET_Addr("127.0.0.1:1234"));
}

TEST(address_cache_test, store_remove_load_fail)
{
  AddressCache<TestKey> test_cache_;
  AddrSet addrs;
  addrs.insert(ACE_INET_Addr("127.0.0.1:1234"));

  test_cache_.store(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs);
  addrs.clear();

  test_cache_.remove(TestKey(GUID_UNKNOWN, GUID_UNKNOWN));

  ASSERT_FALSE(test_cache_.load(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs));
}

TEST(address_cache_test, store_remove_contains_load_fail)
{
  AddressCache<TestKey> test_cache_;
  AddrSet addrs;
  addrs.insert(ACE_INET_Addr("127.0.0.1:1234"));

  test_cache_.store(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs);
  addrs.clear();

  test_cache_.remove_contains(GUID_UNKNOWN);

  ASSERT_FALSE(test_cache_.load(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs));
}

TEST(address_cache_test, scoped_access_load_success)
{
  AddressCache<TestKey> test_cache_;
  {
    AddressCache<TestKey>::ScopedAccess entry(test_cache_, TestKey(GUID_UNKNOWN, GUID_UNKNOWN));
    AddrSet& addrs = entry.value().addrs_;

    addrs.insert(ACE_INET_Addr("127.0.0.1:1234"));
    addrs.insert(ACE_INET_Addr("127.0.1.1:4321"));
  }

  AddrSet addrs;
  ASSERT_TRUE(test_cache_.load(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs));
  ASSERT_EQ(addrs.size(), 2u);
  ASSERT_EQ(*(addrs.begin()),ACE_INET_Addr("127.0.0.1:1234"));
  ASSERT_EQ(*(++(addrs.begin())),ACE_INET_Addr("127.0.1.1:4321"));
}
