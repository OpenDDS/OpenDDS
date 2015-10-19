#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/OS_main.h"
#include "ace/Log_Msg.h"

#include "test_check.h"

#include <string.h>
#include <iostream>

#ifdef OPENDDS_SAFETY_PROFILE
namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

using namespace OpenDDS::DCPS;

// Malloc should return pointer
void test_malloc() {
  SafetyProfilePool pool;
  pool.configure_pool(1024, sizeof(void*));
  TEST_CHECK(pool.malloc(24));
}

// Malloc should return pointer
void test_mallocs() {
  SafetyProfilePool pool;
  pool.configure_pool(1024, sizeof(void*));
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
  TEST_CHECK(p1 > p2);
  TEST_CHECK(p2 > p3);
  TEST_CHECK(p3 > p4);
  TEST_CHECK(p4 > p5);
}

int ACE_TMAIN(int, ACE_TCHAR* [] )
{
  test_malloc();
  test_mallocs();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);
  return failed;
}
#endif
