#include "dds/DCPS/PoolAllocator.h"
#include "ace/Log_Msg.h"

#include "test_check.h"

#include <string.h>
#include <iostream>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

using namespace OpenDDS::DCPS;

void test_string() {
  OPENDDS_STRING foo("foo");
  OPENDDS_STRING bar("bar");
  char buffer[] = "hello";

  TEST_CHECK(foo == "foo");
  TEST_CHECK("foo" == foo);
  TEST_CHECK(bar == "bar");
  TEST_CHECK(foo != bar);
  TEST_CHECK(bar < foo);
  TEST_CHECK(foo > bar);

  TEST_CHECK(OPENDDS_STRING("hello") == buffer);
  TEST_CHECK(OPENDDS_STRING(buffer) == "hello");
}

typedef OPENDDS_MAP(OPENDDS_STRING, int) StringToIntMap;
typedef OPENDDS_MAP(int, int) IntToIntMap;

void test_map() {
  IntToIntMap map;
  map[1] = 4;
  map[5] = 23;
  map[19] = 1000000;
  map[5] = 20;

  TEST_CHECK(map.size() == 3);
  TEST_CHECK(map[5] == 20);
  TEST_CHECK(map.find(14) == map.end());
  TEST_CHECK(map.begin()->first == 1);
  TEST_CHECK(map.begin()->second == 4);
}

void test_string_key_map() {
  StringToIntMap map;
  map["one"] = 4;
  map["five"] = 23;
  map.insert(std::make_pair(OPENDDS_STRING("nineteen"), 1000000));
  map[OPENDDS_STRING("five")] = 20;

  TEST_CHECK(map.size() == 3);
  TEST_CHECK(map["five"] == 20);
  TEST_CHECK(map[OPENDDS_STRING("nineteen")] == 1000000);
  TEST_CHECK(map.find("fourteen") == map.end());
  TEST_CHECK(map.begin()->first == "five");
  TEST_CHECK(map.begin()->second == 20);
}

int main(int, const char** )
{
  test_string();
  test_map();
  test_string_key_map();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);
  if (failed) {
    printf("test FAILED\n");
  } else {
    printf("test PASSED\n");
  }
  return failed;
}

