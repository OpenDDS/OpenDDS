#include <gtest/gtest.h>

#include "dds/DCPS/SafetyProfilePool.h"
#include "ace/Log_Msg.h"

#include <string.h>
#include <iostream>

#ifdef OPENDDS_SAFETY_PROFILE
using namespace OpenDDS::DCPS;

// Malloc should return pointer
void test_malloc() {
  SafetyProfilePool pool;
  pool.configure_pool(1024, sizeof(void*));
  EXPECT_TRUE(pool.malloc(24));
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
  EXPECT_TRUE(p1 = pool.malloc(24));
  EXPECT_TRUE(p2 = pool.malloc(44));
  EXPECT_TRUE(p3 = pool.malloc(24));
  EXPECT_TRUE(p4 = pool.malloc(64));
  EXPECT_TRUE(p5 = pool.malloc(32));
  EXPECT_TRUE(p1 > p2);
  EXPECT_TRUE(p2 > p3);
  EXPECT_TRUE(p3 > p4);
  EXPECT_TRUE(p4 > p5);
}

TEST(dds_DCPS_SafetyProfilePool, maintest)
{
  test_malloc();
  test_mallocs();
}
#endif
