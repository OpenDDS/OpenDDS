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
    ACE_ERROR((LM_ERROR,"(%P|%t) TEST_CHECK(%C) FAILED at %N:%l %a\n",\
        #COND , -1)); \
  }

using namespace OpenDDS::DCPS;

void test_pool_alloc_null() {
  PoolAllocation alloc;
  TEST_CHECK(alloc.ptr() == 0);
  TEST_CHECK(alloc.size() == 0);
}

// Malloc should return pointer
void test_malloc() {
  SafetyProfilePool pool;
  TEST_CHECK(pool.malloc(24));
  //TEST_CHECK(pool.pools_used() == 1);
  //TEST_CHECK(pool.pool(0)->chunk_size() == 32);
}

// Malloc should return pointer
void test_mallocs() {
  SafetyProfilePool pool;
  void* p1;
  void* p2;
  void* p3;
  void* p4;
  void* p5;
  TEST_CHECK(p1 = pool.malloc(24));
  TEST_CHECK(p2 = pool.malloc(44));
  TEST_CHECK(p3 = pool.malloc(24));
  TEST_CHECK(p4 = pool.malloc(64));
  TEST_CHECK(p5 = pool.malloc(32));
  TEST_CHECK(p1 < p3); // 32 byte pool
  TEST_CHECK(p3 < p5);
  //TEST_CHECK(p5 < p2); // 64 byte pool
  TEST_CHECK(p2 < p4);
  //TEST_CHECK(pool.pools_used() == 2);
  //TEST_CHECK(pool.pool(0)->chunk_size() == 32);
  //TEST_CHECK(pool.pool(1)->chunk_size() == 64);
}

/*
// Allocate enough to fill the pool, and a new one should be allocated
void test_mallocs_overflow() {
  SafetyProfilePool pool;
  TEST_CHECK(pool.malloc(24)); // 1
  TEST_CHECK(pool.malloc(44)); //    1
  TEST_CHECK(pool.malloc(24)); // 2
  TEST_CHECK(pool.malloc(64)); //    2
  TEST_CHECK(pool.malloc(32)); // 3
  TEST_CHECK(pool.malloc(21)); // 4
  TEST_CHECK(pool.malloc(84)); //         1
  TEST_CHECK(pool.malloc(64)); //     3
  TEST_CHECK(pool.malloc(22)); // 5
  TEST_CHECK(pool.malloc(18)); // 6
  TEST_CHECK(pool.malloc(10)); // 7
  TEST_CHECK(pool.malloc(84)); //         2
  TEST_CHECK(pool.malloc(28)); // 8
  TEST_CHECK(pool.malloc(22)); // 9
  TEST_CHECK(pool.malloc(64)); //     4
  TEST_CHECK(pool.malloc(22)); // 10
  TEST_CHECK(pool.malloc(18)); // 11
  TEST_CHECK(pool.malloc(10)); // 12
  TEST_CHECK(pool.pools_used() == 3);
  TEST_CHECK(pool.malloc(28)); // 13
  TEST_CHECK(pool.pools_used() == 4);
  TEST_CHECK(pool.malloc(22)); // 14

  TEST_CHECK(pool.pool(0)->chunk_size() == 32);
  TEST_CHECK(pool.pool(1)->chunk_size() == 32);
  TEST_CHECK(pool.pool(2)->chunk_size() == 64);
}

// Make sure memory is reclaimed
void test_malloc_free_loop() {
  SafetyProfilePool pool;
  void* block;
  void* prev = 0;
  for (int i = 0; i < 100; ++i) {
    block = pool.malloc(24);
    if (prev) {
      TEST_CHECK(prev == block);
    } else {
      prev = block;
    }
    pool.free(block);
  }

  TEST_CHECK(pool.pools_used() == 1);
  TEST_CHECK(pool.pool(0)->chunk_size() == 32);
}
*/
int main(int, const char** )
{
  test_malloc();
  test_mallocs();
  //test_mallocs_overflow();
  //test_malloc_free_loop();
  printf("%d assertions failed, %d passed\n", failed, assertions - failed);
  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

