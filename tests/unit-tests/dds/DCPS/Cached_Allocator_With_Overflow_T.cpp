#include <dds/DCPS/Cached_Allocator_With_Overflow_T.h>

#include <gtest/gtest.h>

#include <ace/Thread_Mutex.h>

namespace {

struct TestChunk {
  char data[32];
};

} // namespace

TEST(dds_DCPS_Cached_Allocator_With_Overflow_T, malloc_rejects_oversized_request)
{
  OpenDDS::DCPS::Cached_Allocator_With_Overflow<TestChunk, ACE_Thread_Mutex> allocator(2);

  EXPECT_EQ(static_cast<void*>(0), allocator.malloc(sizeof(TestChunk) + 1));
  EXPECT_EQ(2u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
}

TEST(dds_DCPS_Cached_Allocator_With_Overflow_T, malloc_uses_pool_when_available)
{
  OpenDDS::DCPS::Cached_Allocator_With_Overflow<TestChunk, ACE_Thread_Mutex> allocator(2);

  void* p1 = allocator.malloc();
  void* p2 = allocator.malloc();

  ASSERT_NE(static_cast<void*>(0), p1);
  ASSERT_NE(static_cast<void*>(0), p2);
  EXPECT_NE(p1, p2);
  EXPECT_EQ(0u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());

  allocator.free(p1);
  allocator.free(p2);
  EXPECT_EQ(2u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
}

TEST(dds_DCPS_Cached_Allocator_With_Overflow_T, malloc_overflows_to_heap_when_pool_exhausted)
{
  OpenDDS::DCPS::Cached_Allocator_With_Overflow<TestChunk, ACE_Thread_Mutex> allocator(2);

  void* p1 = allocator.malloc();
  void* p2 = allocator.malloc();
  void* p3 = allocator.malloc();
  void* p4 = allocator.malloc();

  ASSERT_NE(static_cast<void*>(0), p1);
  ASSERT_NE(static_cast<void*>(0), p2);
  ASSERT_NE(static_cast<void*>(0), p3);
  ASSERT_NE(static_cast<void*>(0), p4);

  EXPECT_EQ(0u, allocator.available());
  EXPECT_EQ(2u * sizeof(TestChunk), allocator.bytes_heap_allocated());

  allocator.free(p3);
  allocator.free(p4);
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
  EXPECT_EQ(0u, allocator.available());

  allocator.free(p1);
  allocator.free(p2);
  EXPECT_EQ(2u, allocator.available());
}
