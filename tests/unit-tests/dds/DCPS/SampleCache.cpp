
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
// TODO: Convert to GTEST.
#include "../../../DCPS/common/TestSupport.h"

#include "dds/DCPS/SampleCache.h"

#include <string.h>

using namespace OpenDDS::DCPS;

namespace {

struct Sample {
  std::string key;
  int value;

  Sample()
    : value(0)
  {}

  Sample(const std::string& a_key, int a_value)
    : key(a_key)
    , value(a_value)
  {}

  bool operator<(const Sample& other) const
  {
    return key < other.key;
  }

  bool operator==(const Sample& other) const
  {
    return key == other.key && value == other.value;
  }
};

const Sample key("a", 0);
const DDS::Time_t source_timestamp = { 0, 0 };
const DDS::InstanceHandle_t instance_handle_a = 1;
const DDS::InstanceHandle_t publication_handle_1 = 1;
typedef SampleCache<Sample> SampleCacheType;
typedef SampleCacheType::SampleCachePtr SampleCachePtrType;
typedef SampleCacheType::SampleList SampleList;

void test_SampleCache_ctor()
{
  std::cout << __func__ << std::endl;

  const DDS::InstanceHandle_t ih = 87;

  SampleCacheType sample_cache(ih);
  TEST_ASSERT(sample_cache.empty());
  TEST_ASSERT(sample_cache.size() == 0);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 0);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
  TEST_ASSERT(sample_cache.get_instance_handle() == ih);
}

// Setup a cache with three not read samples and two read samples.
void setup1(SampleCacheType& sample_cache)
{
  TEST_ASSERT(sample_cache.empty());

  sample_cache.write(Sample("a", 0), source_timestamp, publication_handle_1);
  sample_cache.write(Sample("a", 1), source_timestamp, publication_handle_1);

  TEST_ASSERT(sample_cache.new_size() == 2);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_cache.not_new_size() == 2);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  sample_cache.write(Sample("a", 2), source_timestamp, publication_handle_1);
  sample_cache.write(Sample("a", 3), source_timestamp, publication_handle_1);
  sample_cache.write(Sample("a", 4), source_timestamp, publication_handle_1);

  TEST_ASSERT(!sample_cache.empty());
  TEST_ASSERT(sample_cache.size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 3);
  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.state() == SampleCacheState( DDS::READ_SAMPLE_STATE | DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
}

void test_SampleCache_resize()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  sample_cache.resize(5);
  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 3);

  sample_cache.resize(4);
  TEST_ASSERT(sample_cache.not_new_size() == 1);
  TEST_ASSERT(sample_cache.new_size() == 3);

  sample_cache.resize(3);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 3);

  sample_cache.resize(2);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 2);

  sample_cache.resize(1);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 1);

  sample_cache.resize(0);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_register_instance()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 1);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
}

void test_SampleCache_unregister_instance()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.unregister_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 0);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE));
}

void test_SampleCache_dispose_instance()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.dispose_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 1);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE));
}

void test_SampleCache_initialize()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache1(instance_handle_a);
  setup1(sample_cache1);

  SampleCacheType sample_cache2(instance_handle_a);
  sample_cache2.initialize(sample_cache1);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache2.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));
  TEST_ASSERT(sample_list[4] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.length() == 5);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 4, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 3, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[4] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 0, 0, 0, true));
}

void test_SampleCache_read_all_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 3);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.length() == 3);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_read_N_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 4);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_read_all_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_read_N_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_read_all_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));
  TEST_ASSERT(sample_list[3] == Sample("a", 0));
  TEST_ASSERT(sample_list[4] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 5);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 4, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[4] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_read_N2_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 4);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_read_N4_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 4, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));
  TEST_ASSERT(sample_list[3] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.length() == 4);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_all_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 3);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.length() == 3);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_N_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_take_all_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_N_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 1);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_all_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));
  TEST_ASSERT(sample_list[4] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.length() == 5);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 4, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[4] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_N2_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_N4_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 4, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 4);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    test_SampleCache_ctor();
    test_SampleCache_resize();
    test_SampleCache_register_instance();
    test_SampleCache_unregister_instance();
    test_SampleCache_dispose_instance();
    test_SampleCache_initialize();
    test_SampleCache_read_all_not_read();
    test_SampleCache_read_N_not_read();
    test_SampleCache_read_all_read();
    test_SampleCache_read_N_read();
    test_SampleCache_read_all_any();
    test_SampleCache_read_N2_any();
    test_SampleCache_read_N4_any();
    test_SampleCache_take_all_not_read();
    test_SampleCache_take_N_not_read();
    test_SampleCache_take_all_read();
    test_SampleCache_take_N_read();
    test_SampleCache_take_all_any();
    test_SampleCache_take_N2_any();
    test_SampleCache_take_N4_any();
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  return 0;
}
