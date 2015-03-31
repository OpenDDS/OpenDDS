#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/Log_Msg.h"

#include <string.h>
#include <iostream>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

#define TEST_CHECK(COND) \
  ++assertions; \
  if (!( COND )) {\
    ++failed; \
    ACE_DEBUG((LM_ERROR,"TEST_CHECK(%C) FAILED at %N:%l\n",\
        #COND )); \
    return; \
  }

using namespace OpenDDS::DCPS;

class PoolAllocationTest {
public:
  void test_pool_alloc_null() {
    PoolAllocation alloc;
    TEST_CHECK(alloc.ptr() == 0);
    TEST_CHECK(alloc.size() == 0);
  }

  void test_set() {
    char buffer[1024];
    PoolAllocation alloc;
    alloc.set(buffer, 1024);
    TEST_CHECK(alloc.ptr() == buffer);
    TEST_CHECK(alloc.size() == 1024);
  }

  void test_allocate_partial() {
    char buffer[1024];
    PoolAllocation from;
    PoolAllocation target;
    from.set(buffer, 1024);
    void* ptr = from.allocate(128, &target);

    TEST_CHECK(from.size() == 1024 - 128);
    TEST_CHECK(target.size() == 128);
    TEST_CHECK(from.size() + target.size() == 1024);

    TEST_CHECK(from.ptr() == buffer);
    TEST_CHECK(target.ptr() == ptr);
    TEST_CHECK(target.ptr() == buffer + from.size());
  }

  void test_join_next() {
    char buffer[1024];
    PoolAllocation from;
    PoolAllocation to;
    PoolAllocation next;
    from.set(buffer, 128, true);
    to.set(buffer + 128, 64, true);
    next.set(buffer + 128, 32, true);
    to.next_free_ = &next;

    TEST_CHECK(from.join_freed(&to));
    TEST_CHECK(from.ptr() == buffer);
    TEST_CHECK(from.size() == 128 + 64);
    TEST_CHECK(to.ptr() == NULL);
    TEST_CHECK(to.size() == 64);
    TEST_CHECK(to.free_ = true);
    TEST_CHECK(to.next_free_ == &next);
  }
};

int main(int, const char** )
{
  PoolAllocationTest test;
  test.test_pool_alloc_null();
  test.test_set();
  test.test_allocate_partial();
  // full allocation should not be done
  test.test_join_next();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);
  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

