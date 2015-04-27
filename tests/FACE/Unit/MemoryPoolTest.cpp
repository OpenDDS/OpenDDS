#include "dds/DCPS/MemoryPool.h"
#include "ace/Log_Msg.h"
#include "test_check.h"

#include <string.h>
#include <iostream>
#include <map>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

using namespace OpenDDS::DCPS;

class MemoryPoolTest {
public:
  // Allocate a block
  void test_pool_alloc() {
    MemoryPool pool(1024, 8);
    char* ptr = pool.pool_alloc(128);

    TEST_CHECK(ptr);
    validate_pool(pool, 128);
  }

  // Allocate a several blocks
  void test_pool_allocs() {
    MemoryPool pool(1024, 8);
    validate_pool(pool, 0);
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
    MemoryPool pool(1024, 8);
    char* ptr0 = pool.pool_alloc(120);
    validate_pool(pool, 120);
    char* ptr1 = pool.pool_alloc(121);
    validate_pool(pool, 120+128);
    char* ptr2 = pool.pool_alloc(12);
    validate_pool(pool, 120+128+16);
    char* ptr3 = pool.pool_alloc(7);
    validate_pool(pool, 120+128+16+16);
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
    MemoryPool pool(1024, 8);
    char* ptr0 = pool.pool_alloc(128);
    validate_pool(pool, 128);
    char* ptr1 = pool.pool_alloc(256);
    validate_pool(pool, 384);
    char* ptr2 = pool.pool_alloc(128);
    validate_pool(pool, 512);
    char* ptr3 = pool.pool_alloc(448);
    validate_pool(pool, 1024 - 64);

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
    MemoryPool pool(1024, 8);
    validate_pool(pool, 0);
    char* ptr = pool.pool_alloc(128);
    validate_pool(pool, 128);
    pool.pool_free(ptr);
    validate_pool(pool, 0);
  }

  // Allocate a block and free it, becoming largest
  void test_pool_alloc_free_join_largest() {
    MemoryPool pool(2048, 8);
    char* ptr0 = pool.pool_alloc(128);
    char* ptr1 = pool.pool_alloc(256);
    char* ptr2 = pool.pool_alloc(128);
    char* ptr3 = pool.pool_alloc(512);
    validate_pool(pool, 1024);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr2);
    pool.pool_free(ptr0);
    validate_pool(pool, 1024 - 128);
    pool.pool_free(ptr3);
    validate_pool(pool, 1024 - 512 - 128);
  }

  // Allocate a block and free it, becoming smallest
  void test_pool_alloc_free_smallest() {
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(1024, 8);
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
    MemoryPool pool(2048, 8);
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
    validate_pool(pool, 128*7+sizeof(AllocHeader));
    TEST_CHECK(ptr7 < ptr1);
    TEST_CHECK(ptr7 > ptr4);
  }

  // Free a block and the block to it's left, case III
  void test_pool_free_join_left() {
    MemoryPool pool(2048, 8);
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
    validate_pool(pool, 128*7+sizeof(AllocHeader));
    TEST_CHECK(ptr7 < ptr0);
    TEST_CHECK(ptr7 > ptr3);
  }

  // Free a block and the ones to its left and right, case IV
  void test_pool_free_join_both() {
    MemoryPool pool(2048, 8);
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

  void test_pool_free_join_first_both() {
    MemoryPool pool(2048, 8);
    char* ptr0 = pool.pool_alloc( 64); // rightmost
    char* ptr1 = pool.pool_alloc(128);
    char* ptr2 = pool.pool_alloc( 64); // freed ahead of time
    char* ptr3 = pool.pool_alloc(128); 
    char* ptr4 = pool.pool_alloc(128);
    char* ptr5 = pool.pool_alloc(256); // freed ahead of time, second largest
    char* ptr6 = pool.pool_alloc(128); // leftmost, join with head and ptr5
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr1);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr4);
    TEST_CHECK(ptr6);
    pool.pool_free(ptr5);
    pool.pool_free(ptr2);
    validate_pool(pool, 128*4 + 64);
    // now free ptr6, join both largest and ptr5
    pool.pool_free(ptr6);
    validate_pool(pool, 128*3 + 64);
  }

  // Free block and join with the one to its right (largest), case II
  void test_pool_free_join_largest_right() {
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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
    MemoryPool pool(2048, 8);
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

  // Allocate and move ahead in free list
  void test_pool_alloc_move_ahead() {
    MemoryPool pool(1024, 8);
    pool.pool_alloc(16);
    char* ptr0 = pool.pool_alloc(128);
    pool.pool_alloc(16);
    char* ptr1 = pool.pool_alloc(256);
    pool.pool_alloc(16);
    char* ptr2 = pool.pool_alloc(64);
    pool.pool_alloc(16);
    char* ptr3 = pool.pool_alloc(32);
    pool.pool_alloc(16);
    char* ptr4 = pool.pool_alloc(16);
    pool.pool_alloc(16);
    validate_pool(pool, 128+256+64+32+16+96);

    pool.pool_free(ptr0);
    validate_pool(pool, 256+64+32+16+96);

    pool.pool_free(ptr1);
    validate_pool(pool, 64+32+16+96);

    pool.pool_free(ptr2);
    validate_pool(pool, 32+16+96);

    pool.pool_free(ptr3);
    validate_pool(pool, 16+96);

    pool.pool_free(ptr4);
    validate_pool(pool, 96);

    // Allocate from ptr0
    pool.pool_alloc(80);
    validate_pool(pool, 96+80);
  }

  // Alloc and free same block repeatedly
  void test_pool_alloc_free_repeated() {
    MemoryPool pool(1024, 8);
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
    MemoryPool pool(1800, 8);
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
    TEST_CHECK(ptr0);
    TEST_CHECK(ptr3);
    TEST_CHECK(ptr4);
  }

  // Allocate blocks and free in alloc order
  void test_pool_free_in_order() {
    MemoryPool pool(1024, 8);
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
    MemoryPool pool(1024, 8);
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
    MemoryPool pool(1024, 8);
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

  // Join two adjacent blocks, without moving in free list
  void test_pool_free_join_without_moving() {
    MemoryPool pool(1024*20, 8);
    char* ptr0 = pool.pool_alloc(1024);
    pool.pool_alloc(16); // add alloc in between
    char* ptr1 = pool.pool_alloc(512);
    pool.pool_alloc(16); // add alloc in between
    char* ptr2 = pool.pool_alloc(256);
    pool.pool_alloc(16); // add alloc in between
    char* ptr3 = pool.pool_alloc(128);
    pool.pool_alloc(16); // add alloc in between
    char* ptr4 = pool.pool_alloc(64);
                       // no space here, 4+5 adjacent
    char* ptr5 = pool.pool_alloc(32);
    pool.pool_alloc(16); // add alloc in between
    char* ptr6 = pool.pool_alloc(16);
    validate_pool(pool, 1024+512+256+128+64+32+16 + 16*5);
    // Now create free list
    pool.pool_free(ptr0);
    TEST_CHECK(ptr1);
    pool.pool_free(ptr2);
    TEST_CHECK(ptr3);
    pool.pool_free(ptr4);
    TEST_CHECK(ptr5);
    pool.pool_free(ptr6);
    // Free list sizes are 1024, 256, 64
    validate_pool(pool, 0+512+0+128+0+32+0 + 16*5);
    // free 5, join with adjacent 4, making size (64+32)
    pool.pool_free(ptr5);
    validate_pool(pool, 0+512+0+128+0+0+0 + 16*5);
  }

  // Split free block, without moving in free list
  void test_pool_alloc_split_without_moving() {
    MemoryPool pool(1024*20, 8);
    char* ptr0 = pool.pool_alloc(1024);
    pool.pool_alloc(16); // add alloc in between
    char* ptr1 = pool.pool_alloc(512);
    pool.pool_alloc(16); // add alloc in between
    char* ptr2 = pool.pool_alloc(256);
    pool.pool_alloc(16); // add alloc in between
    char* ptr3 = pool.pool_alloc(128);
    pool.pool_alloc(16); // add alloc in between
    char* ptr4 = pool.pool_alloc(64);
    pool.pool_alloc(16); // add alloc in between
    char* ptr5 = pool.pool_alloc(32);
    pool.pool_alloc(16); // add alloc in between
    char* ptr6 = pool.pool_alloc(16);
    validate_pool(pool, 1024+512+256+128+64+32+16 + 16*6);
    pool.pool_free(ptr0);
    TEST_CHECK(ptr1);
    pool.pool_free(ptr2);
    TEST_CHECK(ptr3);
    pool.pool_free(ptr4);
    TEST_CHECK(ptr5);
    pool.pool_free(ptr6);
    // Free list sizes are 1024, 256, 64, 16
    validate_pool(pool, 0+512+0+128+0+32+0 + 16*6);
    // allocate from free size 256
    pool.pool_alloc(80);
    validate_pool(pool, 0+512+80+128+0+32+0 + 16*6);
  }

  // Test shifting by allocating from earlier in the free list
  // Caught by DCPS Reliability test
  void test_pool_alloc_shift_free() {
    MemoryPool pool(1024, 8);
    pool.pool_alloc(48);
    pool.pool_alloc(40);
    pool.pool_alloc(48);
    char* p1 = pool.pool_alloc(16);
    pool.pool_alloc(16);
    char* p2 = pool.pool_alloc(24);
    pool.pool_alloc(48);
    pool.pool_alloc(40);
    pool.pool_alloc(48);
    pool.pool_alloc(40);
    validate_pool(pool, 48+40+48+16+16+24+48+40+48+40);
    pool.pool_free(p1);
    pool.pool_free(p2);
    validate_pool(pool, 48+40+48+16+48+40+48+40);
    // Allocating 40 (from head) corrupts list
    pool.pool_alloc(40);
    validate_pool(pool, 40+48+40+48+16+48+40+48+40);
  }

  void test_pool_alloc_shift_first_free() {
    MemoryPool pool(1024, 64);
    pool.pool_alloc(256);
    pool.pool_alloc(256);
    char* p1 = pool.pool_alloc(256);
    pool.pool_alloc(32);
    pool.pool_alloc(32);
    pool.pool_alloc(32);
    validate_pool(pool, 256*3 + 32*3);
    pool.pool_free(p1); // Largest free buffer, first now 160
    validate_pool(pool, 256*2 + 32*3);
    pool.pool_alloc(96);
    validate_pool(pool, 256*2 + 96 + 32*3);
  }

  // Test moving ahead in free list where the block previous to the insert is 
  // shifted
  void test_pool_move_forward_prev_shifted() {
    MemoryPool pool(2024, 8);
    pool.pool_alloc(8);
    char* p1 = pool.pool_alloc(64);
    pool.pool_alloc(8);
    char* p2 = pool.pool_alloc(232);
    pool.pool_alloc(32);
    char* p3 = pool.pool_alloc(24);
    pool.pool_alloc(32);
    char* p4 = pool.pool_alloc(24);
    pool.pool_alloc(32);
    pool.pool_free(p1); // Free buffer to allocate from
    pool.pool_free(p2); // Largest free buffer, first now 160
    pool.pool_free(p3); // 
    pool.pool_free(p4); // 
    validate_pool(pool, 32*3 + 16);
    // Now, allocate from p1, moving it to end
    char* p5 = pool.pool_alloc(56);
    validate_pool(pool, 32*3 + 16 + 56);
    // Undo and restore
    pool.pool_free(p5);
    validate_pool(pool, 32*3 + 16);
  }

  // Make sure when joining free list does not get double-
  void test_pool_move_forward_prev_shifted2() {
    MemoryPool pool(2024, 8);
    char* p10 = (pool.pool_alloc(24));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p9 = (pool.pool_alloc(24));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p8 = (pool.pool_alloc(24));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p7 = (pool.pool_alloc(32));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p6 = (pool.pool_alloc(24));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p5 = (pool.pool_alloc(24));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p4 = (pool.pool_alloc(32));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p3 = (pool.pool_alloc(88));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p2 = (pool.pool_alloc(64));
    pool.pool_alloc(8);
    pool.pool_alloc(8);
    char* p1 = pool.pool_alloc(56);  // Free last
    char* p0 = pool.pool_alloc(8);
    pool.pool_alloc(8);

    pool.pool_free(p0);
    pool.pool_free(p2);
    pool.pool_free(p3);
    pool.pool_free(p4);
    pool.pool_free(p5);
    pool.pool_free(p6);
    pool.pool_free(p7);
    pool.pool_free(p8);
    pool.pool_free(p9);
    pool.pool_free(p10);
    validate_pool(pool, 56 + 8*19);
    // Now free from end to middle
    pool.pool_free(p1);
    validate_pool(pool, 8*19);
  }

  // Allocates after running out of memory should return null
  void test_alloc_null_once_out_of_memory() {
    MemoryPool pool(1024, 8);
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
    MemoryPool pool(1024, 8);
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

  void test_free_null_should_ignore() {
    MemoryPool pool(1024, 8);
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
  void validate_index(FreeIndex& index, char* pool_base, bool log = false)
  {
    if (log) {
      FreeIndexNode* node = index.nodes_;
      while (node < index.nodes_ + index.size_) {
        printf("  IND[%4d] -> ", node->size());
        if (node->ptr()) {
          printf(" %4d\n", node->ptr()->size());
        } else {
          printf(" NULL\n");
        }
        ++node;
      };
    }

    for (size_t size = 8; size <= 4096; size *= 2) {
      // Find size or larger
      FreeHeader* header = index.find(size, pool_base);
      if (header) {
        TEST_CHECK(header->size() >= size);
        FreeHeader* smaller = header->smaller_free(pool_base);
        if (smaller) {
          TEST_CHECK(smaller->size() <= header->size());
        }
      }
    }
  }

  void validate_pool(MemoryPool& pool, size_t expected_allocated_bytes,
                     bool log = false) {
    AllocHeader* prev = 0;
    size_t allocated_bytes = 0;
    size_t free_bytes = 0;
    size_t oh_bytes = 0;
    size_t free_count = 0;
    char* pool_end = pool.pool_ptr_ + pool.pool_size_;
    bool prev_was_free;
    size_t index = 0;

    typedef std::map<FreeHeader*, int> FreeMap;
    FreeMap free_map;
    // Gather all free indices
    AllocHeader* alloc = reinterpret_cast<AllocHeader*>(pool.pool_ptr_);
    while (pool.includes(alloc)) {
      FreeHeader* free_header = alloc->is_free() ? 
            reinterpret_cast<FreeHeader*>(alloc) : NULL;
      if (free_header) {
        free_map[free_header] = index;
      }
      alloc = alloc->next_adjacent();
      ++index;
    }

    index = 0;
    if (log) {
      printf("Pool ptr %zx end %zx\n", (unsigned long)pool.pool_ptr_,
             (unsigned long)pool_end);
     }

    // Check all allocs in positional order and not overlapping
    alloc = reinterpret_cast<AllocHeader*>(pool.pool_ptr_);
    while (pool.includes(alloc)) {
      if (log) {
        
        int smlr_index = -1;
        int lrgr_index = -1;
        char lrgr_buff[32];
        char smlr_buff[32];

        FreeHeader* free_header = alloc->is_free() ? 
              reinterpret_cast<FreeHeader*>(alloc) : NULL;
        if (free_header) {
          FreeMap::const_iterator found;
          found = free_map.find(free_header->smaller_free(pool.pool_ptr_));
          if (found != free_map.end()) {
            smlr_index = found->second;
            sprintf(smlr_buff, "[%2d]", smlr_index);
          }
          found = free_map.find(free_header->larger_free(pool.pool_ptr_));
          if (found != free_map.end()) {
            lrgr_index = found->second;
            sprintf(lrgr_buff, "[%2d]", lrgr_index);
          }
        }
        printf(
          "Alloc[%zu] %s at %zx ptr %zx lg %s sm %s size %d psize %d\n",
          index++,
          alloc->is_free() ? 
          (alloc == pool.largest_free_ ? "FREE!" : "free ")  : "     ",
          (unsigned long)alloc,
          (unsigned long)alloc->ptr(),
          lrgr_index >= 0 ? lrgr_buff : "[  ]",
          smlr_index >= 0 ? smlr_buff : "[  ]",
          alloc->size(),
          alloc->prev_size()
          );
      }

      TEST_CHECK(alloc->size());
      if (prev) {
        TEST_CHECK(prev->next_adjacent() == alloc);
        TEST_CHECK(alloc->prev_adjacent() == prev);
        // Validate  these are not consecutive free blocks
        TEST_CHECK(!(prev_was_free && alloc->is_free()));
      }

      if (!alloc->is_free()) {
        allocated_bytes += alloc->size();
        prev_was_free = false;
      } else {
        free_bytes += alloc->size();
        prev_was_free = true;
      }
      oh_bytes += pool.align(sizeof(AllocHeader));
      prev = alloc;
      alloc = alloc->next_adjacent();
    }
    TEST_CHECK((char*)alloc == pool_end);

    TEST_CHECK(allocated_bytes == expected_allocated_bytes);
    TEST_CHECK(allocated_bytes + free_bytes + oh_bytes == pool.pool_size_);

    validate_index(pool.free_index_, pool.pool_ptr_, log);

    size_t prev_size = 0;
    size_t free_bytes_in_list = 0;
    FreeHeader* free_alloc = NULL;
    FreeHeader* prev_free = NULL;

    // Check all free blocks in size order
    for (free_alloc = pool.largest_free_;
         free_alloc;
         free_alloc = free_alloc->smaller_free(pool.pool_ptr_)) {
      // Should be marked free
      TEST_CHECK(free_alloc->is_free());
      // Check for infinite loop
      TEST_CHECK(++free_count < 10000);

      // Sum bytes found
      free_bytes_in_list += free_alloc->size();

      // If not the first alloc
      if (free_alloc != pool.largest_free_) {
        TEST_CHECK(free_alloc->size() <= prev_size);
        TEST_CHECK(free_alloc->size() > 0);
      }
      prev_size = free_alloc->size();
      prev_free = free_alloc;
    }

    TEST_CHECK(free_bytes == free_bytes_in_list);

    // Try again from smallest to largest
    if (prev_free) {
      free_bytes_in_list = 0;

      for (free_alloc = prev_free;
           free_alloc;
           free_alloc = free_alloc->larger_free(pool.pool_ptr_)) {
        // Should be marked free
        TEST_CHECK(free_alloc->is_free());

        // Sum bytes found
        free_bytes_in_list += free_alloc->size();

        // If not the first alloc
        if (free_alloc != prev_free) {
          TEST_CHECK(free_alloc->size() >= prev_size);
          TEST_CHECK(free_alloc->size() > 0);
        }
        prev_size = free_alloc->size();
      }
      TEST_CHECK(free_bytes == free_bytes_in_list);
    }
  }
};

int main(int, const char** )
{
  MemoryPoolTest test;

  test.test_pool_alloc();
  test.test_pool_allocs();
  test.test_pool_alloc_odd_size();
/*
  test.test_pool_align_other_size();
*/
  test.test_pool_alloc_last_avail();
  test.test_pool_alloc_free();
  //test.test_pool_alloc_free_largest();
  test.test_pool_alloc_free_join_largest();
  test.test_pool_alloc_free_smallest();

  test.test_pool_alloc_move_ahead();
  test.test_pool_alloc_free_repeated();
  test.test_pool_alloc_non_first_free_block();
  test.test_pool_realloc();

  test.test_pool_free_join_right();
  test.test_pool_free_join_left();
  test.test_pool_free_join_both();

  test.test_pool_free_join_first_both();
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
  test.test_pool_free_join_without_moving();
  test.test_pool_alloc_split_without_moving();
  test.test_pool_alloc_shift_free();
/*
  test.test_pool_alloc_shift_first_free();
  test.test_pool_move_forward_prev_shifted();
  test.test_pool_move_forward_prev_shifted2();

  test.test_alloc_null_once_out_of_memory();
  test.test_alloc_too_large_returns_null();
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

