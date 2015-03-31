#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/Log_Msg.h"
#include "test_check.h"

#include <string.h>
#include <iostream>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
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

  // Test allocating sizes which are not multiples of 8
  void test_pool_alloc_odd_size() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(120);
    validate_pool(pool, 120);
    char* ptr1 = pool.pool_alloc(121);
    validate_pool(pool, 120+128);
    char* ptr2 = pool.pool_alloc(12);
    validate_pool(pool, 120+128+16);
    char* ptr3 = pool.pool_alloc(7);
    validate_pool(pool, 120+128+16+8);
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr0 > ptr1);
    TEST_CHECK(ptr1 > ptr2);
    TEST_CHECK(ptr2 > ptr3);
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

  // Allocate a block and free it, case I
  void test_pool_alloc_free() {
    Pool pool(1024, 64);
    char* ptr = pool.pool_alloc(128);
    validate_pool(pool, 128);
    pool.pool_free(ptr);

    validate_pool(pool, 0);
  }

  // Allocate a block and free it, becoming largest
  void test_pool_alloc_free_largest() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(512);
    validate_pool(pool, 1024);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    pool.pool_free(ptr0);
    pool.pool_free(ptr3);

    validate_pool(pool, 1024 - 512 - 128);
  }

  // Allocate a block and free it, becoming smallest
  void test_pool_alloc_free_smallest() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(512);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    validate_pool(pool, 1024);
    pool.pool_free(ptr3);
    pool.pool_free(ptr0);

    validate_pool(pool, 1024 - 512 - 128);
  }

  // Allocate a block, free it, reallocate
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

  // Free a block and the one to its right, case II
  void test_pool_free_join_right() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*7);
    pool.pool_free(ptr2);
    validate_pool(pool, 128*6);
    // now free ptr3, join right ptr2
    pool.pool_free(ptr3);
    validate_pool(pool, 128*5);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*7);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  // Free a block and the block to it's left, case III
  void test_pool_free_join_left() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr2 = pool.pool_alloc(128); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128);
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr4);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*7);
    pool.pool_free(ptr2);
    validate_pool(pool, 128*6);
    // now free ptr1, join left ptr2
    pool.pool_free(ptr1);
    validate_pool(pool, 128*5);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*7);
    TEST_CHECK(ptr7 < ptr0);
    TEST_CHECK(ptr7 > ptr3);
  }

  // Free a block and the ones to its left and right, case IV
  void test_pool_free_join_both() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2 and ptr4
    char* ptr4 = pool.pool_alloc(128); // freed ahead of time
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*7);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    validate_pool(pool, 128*5);
    // now free ptr3, join ptr2 and ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*4);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*6);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr5);
  }

  // Free block and join with the one to its right (largest), case II
  void test_pool_free_join_largest_right() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(256); // freed ahead of time (largest)
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*8);
    pool.pool_free(ptr2);
    validate_pool(pool, 128*6);
    // now free ptr3, join right ptr2
    pool.pool_free(ptr3);
    validate_pool(pool, 128*5);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*7);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  // Free block join with and the block to it's left (largest), case III
  void test_pool_free_join_largest_left() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr2 = pool.pool_alloc(256); // freed ahead of time (largest)
    char* ptr3 = pool.pool_alloc(128);
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr4);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*8);
    pool.pool_free(ptr2);
    validate_pool(pool, 128*6);
    // now free ptr1, join left ptr2
    pool.pool_free(ptr1);
    validate_pool(pool, 128*5);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*7);
    TEST_CHECK(ptr7 < ptr0);
    TEST_CHECK(ptr7 > ptr3);
  }

  // Free block and join with the ones to its left and largest right, case IV
  void test_pool_free_join_largest_both_right() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(256); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2 and ptr4
    char* ptr4 = pool.pool_alloc(128); // freed ahead of time
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*8);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    validate_pool(pool, 128*5);
    // now free ptr3, join ptr2 and ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*4);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*6);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr5);
  }

  // Free block and join with the ones to its largest left and right, case IV
  void test_pool_free_join_largest_both_left() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2 and ptr4
    char* ptr4 = pool.pool_alloc(256); // freed ahead of time
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(128); // leftmost
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr6);
    validate_pool(pool, 128*8);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    validate_pool(pool, 128*5);
    // now free ptr3, join ptr2 and ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*4);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(256);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*6);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr5);
  }

  // Free block and join with the one to its right (smallest), case II
  void test_pool_free_join_smallest_right() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(64);  // freed ahead of time (smallest)
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(512); // leftmost, freed largest
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    pool.pool_free(ptr2);
    pool.pool_free(ptr6);
    validate_pool(pool, 128*5);
    // now free ptr3, join right ptr2
    pool.pool_free(ptr3);
    validate_pool(pool, 128*4);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(160);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*4 + 160);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  // Free block and join with the one to its left (smallest), case III
  void test_pool_free_join_smallest_left() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(64);  // freed ahead of time (smallest)
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(512); // leftmost, freed largest
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr5);
    pool.pool_free(ptr4);
    pool.pool_free(ptr6);
    validate_pool(pool, 128*5);
    // now free ptr3, join left ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*4);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(160);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*4 + 160);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  void test_pool_free_join_smallest_both_right() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(64);  // freed ahead of time (smallest)
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(128); // freed ahead of time
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(512); // leftmost, freed largest
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr5);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    pool.pool_free(ptr6);
    validate_pool(pool, 128*4);
    // now free ptr3, join left ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*3);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(160);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*3 + 160);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  void test_pool_free_join_smallest_both_left() {
    Pool pool(2048, 64);
    char* ptr0 = pool.pool_alloc(128); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc(128); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); // free and join with ptr2
    char* ptr4 = pool.pool_alloc(64);  // freed ahead of time (smallest)
    char* ptr5 = pool.pool_alloc(128);
    char* ptr6 = pool.pool_alloc(512); // leftmost, freed largest
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr5);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    pool.pool_free(ptr6);
    validate_pool(pool, 128*4);
    // now free ptr3, join left ptr4
    pool.pool_free(ptr3);
    validate_pool(pool, 128*3);
    // Alloate from free
    char* ptr7 = pool.pool_alloc(160);
    TEST_CHECK(ptr7);
    validate_pool(pool, 128*3 + 160);
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
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

  // Allocates after running out of memory should return null
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
    // Out of memory
    char* ptr8 = pool.pool_alloc(128);
    TEST_CHECK(!ptr8);
    validate_pool(pool, 1024);
  }

  // Allocates larger than remaining memory should return null
  void test_alloc_too_large_returns_null() {
    Pool pool(1024, 64);
    char* ptr0 = pool.pool_alloc(256);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(256);
    char* ptr3 = pool.pool_alloc(128);
    char* ptr4 = pool.pool_alloc(640);
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(!ptr4);
    validate_pool(pool, 1024 - 128);
    pool.pool_free(ptr2);
    validate_pool(pool, 1024 - 128 - 256);
  }

  void test_alloc_null_once_out_of_allocs() {
    Pool pool(1024, 5); // 5 gives us max 11 allocs
    char* ptr0 = pool.pool_alloc(64);  // 2 allocs
    char* ptr1 = pool.pool_alloc(64);  // 3 allocs
    char* ptr2 = pool.pool_alloc(64);  // 4 allocs
    char* ptr3 = pool.pool_alloc(64);  // 5 allocs
    char* ptr4 = pool.pool_alloc(64);  // 6 allocs
    char* ptr5 = pool.pool_alloc(64);  // 7 allocs
    char* ptr6 = pool.pool_alloc(64);  // 8 allocs
    char* ptr7 = pool.pool_alloc(64);  // 9 allocs
    char* ptr8 = pool.pool_alloc(64);  // 10 allocs
    char* ptr9 = pool.pool_alloc(64);  // 11 allocs
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr4);
    TEST_CHECK(ptr5);
    TEST_CHECK(ptr6);
    TEST_CHECK(ptr7);
    TEST_CHECK(ptr8);
    TEST_CHECK(ptr9);
    validate_pool(pool, 64*10);
    // Out of allocs
    char* ptr10 = pool.pool_alloc(64);  // 11 allocs
    TEST_CHECK(!ptr10);
    validate_pool(pool, 64*10);
    pool.pool_free(ptr2);
    pool.pool_free(ptr4);
    validate_pool(pool, 64*8);
  }

  void test_free_null_should_ignore() {
    Pool pool(1024, 5); // 5 gives us max 11 allocs
    char* ptr0 = pool.pool_alloc(64);  // 2 allocs
    char* ptr1 = pool.pool_alloc(64);  // 3 allocs
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    validate_pool(pool, 64*2);
    pool.pool_free(ptr1);
    pool.pool_free(NULL);
    validate_pool(pool, 64);
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
  test.test_pool_alloc_odd_size();
  test.test_pool_alloc_last_avail();

  test.test_pool_alloc_free();
  test.test_pool_alloc_free_largest();
  test.test_pool_alloc_free_smallest();

  test.test_pool_alloc_free_repeated();
  test.test_pool_alloc_non_first_free_block();
  test.test_pool_realloc();

  test.test_pool_free_join_right();
  test.test_pool_free_join_left();
  test.test_pool_free_join_both();

  test.test_pool_free_join_largest_right();
  test.test_pool_free_join_largest_left();
  test.test_pool_free_join_largest_both_right();
  test.test_pool_free_join_largest_both_left();

  test.test_pool_free_join_smallest_right();
  test.test_pool_free_join_smallest_left();
  test.test_pool_free_join_smallest_both_right();
  test.test_pool_free_join_smallest_both_left();

  test.test_pool_free_in_order();
  test.test_pool_free_in_reverse_order();
  test.test_pool_free_out_of_order();

  test.test_alloc_null_once_out_of_memory();
  test.test_alloc_too_large_returns_null();
  test.test_alloc_null_once_out_of_allocs();
  test.test_free_null_should_ignore();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);

  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

