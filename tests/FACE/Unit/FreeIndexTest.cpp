#include "dds/DCPS/MemoryPool.h"
#include "ace/OS_main.h"
#include "ace/Log_Msg.h"

#include "test_check.h"

#include <string.h>
#include <iostream>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

using namespace OpenDDS::DCPS;

class FreeIndexTest {
public:

FreeIndexTest()
{

}


void test_setup() {
  FreeIndex index(largest_free_);
  setup(index, 1024*5);
  TEST_CHECK(largest_free_->size() == 1024*5);
  TEST_CHECK(index.size_ == 10);
  TEST_CHECK(index.nodes_[9].ptr() == largest_free_);
}

void test_index_lookup() {
  TEST_CHECK(FreeIndex::node_index(0) == 0);
  TEST_CHECK(FreeIndex::node_index(8) == 0);
  TEST_CHECK(FreeIndex::node_index(9) == 0);
  TEST_CHECK(FreeIndex::node_index(15) == 0);
  TEST_CHECK(FreeIndex::node_index(16) == 1);
  TEST_CHECK(FreeIndex::node_index(31) == 1);
  TEST_CHECK(FreeIndex::node_index(32) == 2);
  TEST_CHECK(FreeIndex::node_index(63) == 2);
  TEST_CHECK(FreeIndex::node_index(64) == 3);
  TEST_CHECK(FreeIndex::node_index(128) == 4);
  TEST_CHECK(FreeIndex::node_index(256) == 5);
  TEST_CHECK(FreeIndex::node_index(512) == 6);
  TEST_CHECK(FreeIndex::node_index(1024) == 7);
  TEST_CHECK(FreeIndex::node_index(2048) == 8);
  TEST_CHECK(FreeIndex::node_index(4096) == 9);
  TEST_CHECK(FreeIndex::node_index(4097) == 9);
  TEST_CHECK(FreeIndex::node_index(9000) == 9);
  TEST_CHECK(FreeIndex::node_index(12000) == 9);
  TEST_CHECK(FreeIndex::node_index(120000) == 9);
}

// add should insert
void test_add() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512);
  index.add(alloc);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc);
};

// add odd size should insert
void test_add_odd_size() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  index.add(alloc);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc);
};

// add two of size should insert ahead
void test_add_same_size() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 80);
  index.add(alloc);
  index.add(alloc2);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc2);
};

// add larger should ignore
void test_add_larger() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  index.add(alloc);
  index.add(alloc2);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc);
};

// Add smaller should replace
void test_add_smaller() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 180);
  FreeHeader* alloc2 = free_block(512 + 80);
  index.add(alloc);
  index.add(alloc2);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc2);
};

// Add and remove
void test_add_remove() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512);
  index.add(alloc);
  TEST_CHECK(index.find(512, pool_ptr_) == alloc);
  list_remove(alloc);
  index.remove(alloc, alloc->larger_free(pool_ptr_));
  TEST_CHECK(index.find(512, pool_ptr_) == largest_free_);
}

void test_removes() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  FreeHeader* alloc3 = free_block(512);
  index.add(alloc);
  index.add(alloc2);
  index.add(alloc3);
  TEST_CHECK(index.find(512, pool_ptr_) == alloc3);
  list_remove(alloc3);
  index.remove(alloc3, alloc3->larger_free(pool_ptr_));
  TEST_CHECK(index.find(512, pool_ptr_) == alloc);
  list_remove(alloc);
  index.remove(alloc, alloc->larger_free(pool_ptr_));
  TEST_CHECK(index.find(512, pool_ptr_) == alloc2);
}

// Find should find only alloc
void test_simple_find() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  FreeHeader* alloc3 = free_block(512);
  index.add(alloc);
  index.add(alloc2);
  index.add(alloc3);
  TEST_CHECK(index.find(512, pool_ptr_) == alloc3);
};

// Find should find when lots of free nodes
void test_full_find() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  FreeHeader* alloc3 = free_block(512);
  index.add(alloc);
  index.add(alloc2);
  index.add(alloc3);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc3);
  TEST_CHECK(index.find(512, pool_ptr_) == alloc3);
  TEST_CHECK(index.find(513, pool_ptr_) == alloc);
  TEST_CHECK(index.find(512 + 80, pool_ptr_) == alloc);
  TEST_CHECK(index.find(512 + 81, pool_ptr_) == alloc2);
}

// Should find when smaller than smallest
void test_too_small_find() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  FreeHeader* alloc3 = free_block(512);
  index.add(alloc);
  index.add(alloc2);
  index.add(alloc3);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc3);
  TEST_CHECK(index.find(500, pool_ptr_) == alloc3);
  TEST_CHECK(index.find(480, pool_ptr_) == alloc3);
  TEST_CHECK(index.find(256, pool_ptr_) == alloc3);
  TEST_CHECK(index.find(8, pool_ptr_) == alloc3);
}

// Should give NULL when larger than largest
void test_too_large_find() {
  FreeIndex index(largest_free_);
  setup(index, 1024);
  FreeHeader* alloc = free_block(512 + 80);
  FreeHeader* alloc2 = free_block(512 + 180);
  FreeHeader* alloc3 = free_block(512);
  index.add(alloc);
  index.add(alloc2);
  index.add(alloc3);
  TEST_CHECK(index.nodes_[6].size() == 512);
  TEST_CHECK(index.nodes_[6].ptr() == alloc3);
  TEST_CHECK(index.find(512+181, pool_ptr_) == largest_free_);
  TEST_CHECK(index.find(largest_free_->size(), pool_ptr_) == largest_free_);
  TEST_CHECK(index.find(largest_free_->size() + 1, pool_ptr_) == NULL);
}

private:
  // Helpers
  void setup(FreeIndex& index, size_t free_size = 1024) {
    memset(pool_ptr_, 0, sizeof(pool_ptr_));
    next_alloc_ = pool_ptr_;
    largest_free_ = reinterpret_cast<FreeHeader*>(pool_ptr_);
    if (free_size) {
      largest_free_->init_free_block(static_cast<unsigned int>(free_size + sizeof(AllocHeader)));
      index.init(largest_free_);
      next_alloc_ += free_size + (sizeof(AllocHeader)*2) + 32;
    }
  }

  FreeHeader* free_block(size_t size) {
    FreeHeader* block = reinterpret_cast<FreeHeader*>(next_alloc_);
    block->set_size(size);
    block->set_free();
    next_alloc_ += size + (sizeof(AllocHeader)*2) + 32;
    list_add(block);
    return block;
  }

  void list_add(FreeHeader* block) {
    if (block->size() > largest_free_->size()) {
      largest_free_->set_larger_free(block, pool_ptr_);
      block->set_smaller_free(largest_free_, pool_ptr_);
      largest_free_ = block;
    } else {
      // Find insertion point
      for (FreeHeader* free_node = largest_free_;
           free_node;
           free_node = free_node->smaller_free(pool_ptr_))
      {
        FreeHeader* next_node = free_node->smaller_free(pool_ptr_);
        if (next_node) {
          if (block->size() > next_node->size()) {
            // Insert here
            free_node->set_smaller_free(block, pool_ptr_);
            block->set_larger_free(free_node, pool_ptr_);
            block->set_smaller_free(next_node, pool_ptr_);
            next_node->set_larger_free(block, pool_ptr_);
            break;
          }
        } else {
          // smallest free
          block->set_larger_free(free_node, pool_ptr_);
          free_node->set_smaller_free(block, pool_ptr_);
          break;
        }
      }
    }
  }

  void list_remove(FreeHeader* block) {
    if (block == largest_free_) {
      FreeHeader* smaller = block->smaller_free(pool_ptr_);
      if (smaller) {
        smaller->set_larger_free(NULL, NULL);
      }
      block->set_smaller_free(NULL, NULL);
    } else {
      // Find in list
      for (FreeHeader* free_node = largest_free_;
           free_node;
           free_node = free_node->smaller_free(pool_ptr_))
      {
        if (free_node == block) {
          FreeHeader* smaller_node = block->smaller_free(pool_ptr_);
          FreeHeader* larger_node = block->larger_free(pool_ptr_);

          if (smaller_node) {
            smaller_node->set_larger_free(larger_node, pool_ptr_);
          }

          if (larger_node) {
            larger_node->set_smaller_free(smaller_node, pool_ptr_);
          }
          break;
        }
      }
    }
    block->set_smaller_free(NULL, NULL);
    block->set_larger_free(NULL, NULL);
  }

  static unsigned char pool_ptr_[1024*1024];
  unsigned char* next_alloc_;
  FreeHeader* largest_free_;
};  // end class

unsigned char
FreeIndexTest::pool_ptr_[1024*1024];

int ACE_TMAIN(int, ACE_TCHAR* [] )
{
  FreeIndexTest test;

  test.test_setup();

  test.test_index_lookup();

  test.test_add();
  test.test_add_odd_size();
  test.test_add_same_size();
  test.test_add_larger();
  test.test_add_smaller();

  test.test_add_remove();
  test.test_removes();

  test.test_simple_find();
  test.test_full_find();
  test.test_too_small_find();
  test.test_too_large_find();

  printf("%d assertions failed, %d passed\n", failed, assertions - failed);
  return failed;
}

