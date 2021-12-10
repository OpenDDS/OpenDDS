#include <dds/DCPS/AddressCache.h>

#include <gtest/gtest.h>

#include <cstring>

#ifdef ACE_HAS_CPP11
#include <dds/DCPS/Hash.h>
#include <functional>
#endif

using namespace OpenDDS::DCPS;

struct TestKey : public RcObject {
  TestKey(const RepoId& from, const RepoId& to) : from_(from), to_(to) {}
  TestKey(const TestKey& val) : RcObject(), from_(val.from_), to_(val.to_) {}
  bool operator<(const TestKey& rhs) const {
    return std::memcmp(static_cast<const void*>(&from_), static_cast<const void*>(&rhs.from_), 2 * sizeof (RepoId)) < 0;
  }
  bool operator==(const TestKey& rhs) const {
    return std::memcmp(static_cast<const void*>(&from_), static_cast<const void*>(&rhs.from_), 2 * sizeof (RepoId)) == 0;
  }
  void get_contained_guids(RepoIdSet& set) const {
    set.clear();
    set.insert(from_);
    set.insert(to_);
  }
  RepoId from_;
  RepoId to_;
};

#define NOOP

#ifdef ACE_HAS_CPP11
namespace std
{

template<> struct hash<TestKey>
{
  std::size_t operator()(const TestKey& val) const noexcept
  {
    uint32_t hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&val.from_), 2 * sizeof (OpenDDS::DCPS::GUID_t));
    return static_cast<size_t>(hash);
  }
};

}
#endif

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

TEST(address_cache_test, store_remove_id_load_fail)
{
  AddressCache<TestKey> test_cache_;
  AddrSet addrs;
  addrs.insert(ACE_INET_Addr("127.0.0.1:1234"));

  test_cache_.store(TestKey(GUID_UNKNOWN, GUID_UNKNOWN), addrs);
  addrs.clear();

  test_cache_.remove_id(GUID_UNKNOWN);

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
