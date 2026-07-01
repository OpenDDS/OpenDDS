#include <dds/DCPS/Dynamic_Cached_Allocator_With_Overflow_T.h>

#include <gtest/gtest.h>

#include <ace/Atomic_Op.h>
#include <ace/Thread_Mutex.h>

namespace {

const size_t CHUNK_SIZE = 32;

} // namespace

TEST(dds_DCPS_Dynamic_Cached_Allocator_With_Overflow_T, malloc_rejects_oversized_request)
{
  OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> allocator(2, CHUNK_SIZE);

  EXPECT_EQ(static_cast<void*>(0), allocator.malloc(CHUNK_SIZE + 1));
  EXPECT_EQ(2u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
  EXPECT_EQ(0u, allocator.allocs_from_heap_.load());
  EXPECT_EQ(0u, allocator.allocs_from_pool_.load());
}

TEST(dds_DCPS_Dynamic_Cached_Allocator_With_Overflow_T, malloc_uses_pool_when_available)
{
  OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> allocator(2, CHUNK_SIZE);

  void* p1 = allocator.malloc(CHUNK_SIZE);
  void* p2 = allocator.malloc(CHUNK_SIZE);

  ASSERT_NE(static_cast<void*>(0), p1);
  ASSERT_NE(static_cast<void*>(0), p2);
  EXPECT_NE(p1, p2);
  EXPECT_EQ(0u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
  EXPECT_EQ(0u, allocator.allocs_from_heap_.load());
  EXPECT_EQ(2u, allocator.allocs_from_pool_.load());

  allocator.free(p1);
  allocator.free(p2);
  EXPECT_EQ(2u, allocator.available());
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
}

TEST(dds_DCPS_Dynamic_Cached_Allocator_With_Overflow_T, malloc_overflows_to_heap_when_pool_exhausted)
{
  OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> allocator(2, CHUNK_SIZE);

  void* p1 = allocator.malloc(CHUNK_SIZE);
  void* p2 = allocator.malloc(CHUNK_SIZE);
  void* p3 = allocator.malloc(CHUNK_SIZE);
  void* p4 = allocator.malloc(CHUNK_SIZE);

  ASSERT_NE(static_cast<void*>(0), p1);
  ASSERT_NE(static_cast<void*>(0), p2);
  ASSERT_NE(static_cast<void*>(0), p3);
  ASSERT_NE(static_cast<void*>(0), p4);

  EXPECT_EQ(0u, allocator.available());
  EXPECT_EQ(2u * CHUNK_SIZE, allocator.bytes_heap_allocated());
  EXPECT_EQ(2u, allocator.allocs_from_heap_.load());
  EXPECT_EQ(2u, allocator.allocs_from_pool_.load());

  allocator.free(p3);
  allocator.free(p4);
  EXPECT_EQ(0u, allocator.bytes_heap_allocated());
  EXPECT_EQ(0u, allocator.available());
  EXPECT_EQ(2u, allocator.frees_to_heap_.load());

  allocator.free(p1);
  allocator.free(p2);
  EXPECT_EQ(2u, allocator.available());
  EXPECT_EQ(2u, allocator.frees_to_pool_.load());
}
