#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/Log_Msg.h"

#include <string.h>
#include <iostream>

unsigned int assertions = 0;
unsigned int failed = 0;

#define TEST_CHECK(COND) \
  ++assertions; \
  if (!( COND )) {\
    ++failed; \
    ACE_DEBUG((LM_ERROR,"TEST_CHECK(%C) FAILED at %N:%l\n",\
        #COND )); \
    return; \
  }

using namespace OpenDDS::DCPS;

class PoolTest {
public:
  // Allocate a block
  void test_pool_alloc() {
    Pool pool(1024, 64);
    char* ptr = pool.pool_alloc(128);

    TEST_CHECK(ptr);
    validate_pool(pool, 128);
  }

  // Allocate a several blocks
  void test_pool_allocs() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    char* ptr1 = pool.pool_alloc(256);
    validate_pool(pool, 384);
    char* ptr2 = pool.pool_alloc(128);
    validate_pool(pool, 512);

    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr0 > ptr1);
    TEST_CHECK(ptr1 > ptr2);
  }

  // Allocate a several blocks until all gone
  void test_pool_alloc_last_avail() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    char* ptr1 = pool.pool_alloc(256);
    validate_pool(pool, 384);
    char* ptr2 = pool.pool_alloc(128);
    validate_pool(pool, 512);
    char* ptr3 = pool.pool_alloc(512);
    validate_pool(pool, 1024);

    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr0 > ptr1);
    TEST_CHECK(ptr1 > ptr2);
    TEST_CHECK(ptr2 > ptr3);
  }

  // Allocate a block and free it
  void test_pool_alloc_free() {
    Pool pool(1024, 64);
    char* ptr = pool.pool_alloc(128);
    validate_pool(pool, 128);
    pool.pool_free(ptr);

    validate_pool(pool, 0);
  }

  // Allocate a block and free it
  void test_pool_realloc() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    pool.pool_free(ptr0);
    char* ptr1 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    TEST_CHECK(ptr0 == ptr1);

    pool.pool_free(ptr1);
    validate_pool(pool, 0);
  }

  // Alloc and free same block repeatedly
  void test_pool_alloc_free_repeated() {
    Pool pool(1024, 64);
    for (int i = 0; i < 256; ++i) {
      char* ptr0 = pool.pool_alloc(128);
      validate_pool(pool, 128);
      TEST_CHECK(ptr0);
      char* ptr1 = pool.pool_alloc(64);
      validate_pool(pool, 128 + 64);
      TEST_CHECK(ptr1);
      pool.pool_free(ptr0);
      validate_pool(pool, 64);
      pool.pool_free(ptr1);
      validate_pool(pool, 0);
    }
  }

  void test_pool_alloc_non_first_free_block() {
    Pool pool(800, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(128);
    validate_pool(pool, 512 + 128);
    pool.pool_free(ptr1);
    validate_pool(pool, 256 + 128);
    pool.pool_free(ptr2);
    validate_pool(pool, 256);
    // Index 2 is only spot large enough for this alloc
    //      ptr3      ptr0
    // F160 A128 F384 A128
    char* ptr4 = pool.pool_alloc(320);
    validate_pool(pool, 256 + 320);
    TEST_CHECK(ptr4);
    TEST_CHECK(ptr0 > ptr4);
    TEST_CHECK(ptr4 > ptr3);
  }

  // Allocate blocks and free in alloc order
  void test_pool_free_in_order() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(128);
    validate_pool(pool, 512);
    pool.pool_free(ptr0);
    validate_pool(pool, 384);
    pool.pool_free(ptr1);
    validate_pool(pool, 256);
    pool.pool_free(ptr2);
    validate_pool(pool, 128);
    pool.pool_free(ptr3);
    validate_pool(pool, 0);
  }

  // Allocate blocks and free in reverse order
  void test_pool_free_in_reverse_order() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(128);
    validate_pool(pool, 512);
    pool.pool_free(ptr3);
    validate_pool(pool, 384);
    pool.pool_free(ptr2);
    validate_pool(pool, 256);
    pool.pool_free(ptr1);
    validate_pool(pool, 128);
    pool.pool_free(ptr0);
    validate_pool(pool, 0);
  }

  // Allocate in one order, free in other order
  void test_pool_free_out_of_order() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(128);
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    validate_pool(pool, 768);
    pool.pool_free(ptr3);
    validate_pool(pool, 640);
    pool.pool_free(ptr0);
    validate_pool(pool, 512);
    pool.pool_free(ptr4);
    validate_pool(pool, 384);
    pool.pool_free(ptr1);
    validate_pool(pool, 256);
    pool.pool_free(ptr5);
    validate_pool(pool, 128);
    pool.pool_free(ptr2);
    validate_pool(pool, 0);
  }

  void test_alloc_null_once_out_of_memory() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(256);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(256);
    char* ptr3 = pool.pool_alloc(128);
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr4);
    TEST_CHECK(!ptr5);
    validate_pool(pool, 1024);
    pool.pool_free(ptr2);
    validate_pool(pool, 1024 - 256);
    char* ptr6 = pool.pool_alloc(128);
    TEST_CHECK(ptr6);
    validate_pool(pool, 1024 - 128);
    char* ptr7 = pool.pool_alloc(128);
    TEST_CHECK(ptr7);
    validate_pool(pool, 1024);
    char* ptr8 = pool.pool_alloc(128);
    TEST_CHECK(!ptr8);
    validate_pool(pool, 1024);
  }

  void test_alloc_null_once_out_of_allocs() {
  }

  void test_free_null_should_ignore() {
  }


private:
  void validate_pool(Pool& pool, size_t expected_allocated_bytes,
                     bool log = false) {
    unsigned int free_nodes = 0;
    char* prev = 0;
    char* prev_end = 0;
    size_t allocated_bytes = 0;
    size_t free_bytes = 0;
    bool prev_free;
    // Check all allocs in positional order and not overlapping
    for (unsigned int i = 0; i < pool.allocs_in_use_; ++i) {
      PoolAllocation* alloc = pool.allocs_ + i;
      if (log) {
        printf("Index %d: alloc %zx, ptr is %zx, size %zu %s %s nextind %d\n", 
                i, (unsigned long)alloc, (unsigned long)(void*)alloc->ptr(),
                alloc->size(), alloc->free_ ? "free " : "alloc", 
                alloc == pool.first_free_ ? "FIRST" : "     ",
                alloc->next_free_ ? int(alloc->next_free_ - pool.allocs_) : -1);
      }
      TEST_CHECK(prev < alloc->ptr());
      if (prev_end) {
        TEST_CHECK(prev_end == alloc->ptr());
        if (alloc->size()) {
          // Validate  these are not consecutive free blocks
          TEST_CHECK(!(prev_free && alloc->free_));
        }
      }
      prev_end = alloc->ptr() + alloc->size();
      prev_free = alloc->free_;

      if (!alloc->free_) {
        allocated_bytes += alloc->size();
      } else {
        free_bytes += alloc->size();
      }
    }

    TEST_CHECK(allocated_bytes == expected_allocated_bytes);
    TEST_CHECK(allocated_bytes + free_bytes == pool.pool_size_);

    size_t prev_size = 0;
    size_t free_bytes_in_list = 0;

    // Check all free blocks in size order
    for (PoolAllocation* free_alloc = pool.first_free_;
         free_alloc;
         free_alloc = free_alloc->next_free_) {
      // Should be marked free
      TEST_CHECK(free_alloc->free_);

      TEST_CHECK(++free_nodes <= pool.allocs_in_use_);
      // Sum bytes found
      free_bytes_in_list += free_alloc->size();

      // If not the first alloc
      if (free_alloc != pool.first_free_) {
        TEST_CHECK(free_alloc->size() <= prev_size);
        TEST_CHECK(free_alloc->size() > 0);
      }
      prev_size = free_alloc->size();
    }

    TEST_CHECK(free_bytes == free_bytes_in_list);
  }
};

int main(int, const char** )
{
  PoolTest test;

  test.test_pool_alloc();
  test.test_pool_allocs();
  test.test_pool_alloc_last_avail();
  test.test_pool_alloc_free();
  test.test_pool_alloc_free_repeated();
  test.test_pool_alloc_non_first_free_block();
  test.test_pool_realloc();
  test.test_pool_free_in_order();
  test.test_pool_free_in_reverse_order();
  test.test_pool_free_out_of_order();
/*
  test.test_alloc_null_once_out_of_memory();
  test.test_alloc_null_once_out_of_allocs();
  test.test_free_null_should_ignore();
*/

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);

  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

