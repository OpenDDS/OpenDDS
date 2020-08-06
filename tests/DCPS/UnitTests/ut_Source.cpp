
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
#include "../common/TestSupport.h"

#include "dds/DCPS/Source.h"

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

// std::ostream& operator<<(std::ostream& out, const Sample& sample)
// {
//   out << "Sample(" << sample.key << ',' << sample.value << ")";
//   return out;
// }

const Sample key("a", 0);
const MonotonicTimePoint source_timestamp;
const MonotonicTimePoint source_timestamp_1(ACE_Time_Value(1,0));
const MonotonicTimePoint source_timestamp_2(ACE_Time_Value(2,0));
const MonotonicTimePoint source_timestamp_3(ACE_Time_Value(3,0));
const PublicationHandle publication_handle_1(PublicationHandle(1));
const PublicationHandle publication_handle_2(PublicationHandle(2));
typedef SampleCache<Sample> SampleCacheType;
typedef SampleCacheType::SampleCachePtr SampleCachePtrType;
typedef SampleCacheType::SampleList SampleList;
typedef Sink<Sample> SinkType;
typedef SinkType::SinkPtr SinkPtrType;
typedef Source<Sample> SourceType;
typedef SourceType::SourcePtr SourcePtrType;

void test_SampleCache_ctor()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  TEST_ASSERT(sample_cache.empty());
  TEST_ASSERT(sample_cache.size() == 0);
  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 0);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE));
}

// Setup a cache with three not read samples and two read samples.
void setup1(SampleCacheType& sample_cache)
{
  TEST_ASSERT(sample_cache.empty());

  sample_cache.write(Sample("a", 0), source_timestamp, 0);
  sample_cache.write(Sample("a", 1), source_timestamp, 0);

  TEST_ASSERT(sample_cache.new_size() == 2);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sample_cache.read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_cache.not_new_size() == 2);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  sample_cache.write(Sample("a", 2), source_timestamp, 0);
  sample_cache.write(Sample("a", 3), source_timestamp, 0);
  sample_cache.write(Sample("a", 4), source_timestamp, 0);

  TEST_ASSERT(!sample_cache.empty());
  TEST_ASSERT(sample_cache.size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 3);
  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(ANY_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE));
}

void test_SampleCache_resize()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
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

  SampleCacheType sample_cache;
  sample_cache.register_instance(key, source_timestamp, 0);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 1);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(NO_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE));
}

void test_SampleCache_unregister_instance()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.unregister_instance(key, source_timestamp, 0);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 0);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(NO_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE));
}

void test_SampleCache_dispose_instance()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.dispose_instance(key, source_timestamp, 0);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, false));

  TEST_ASSERT(sample_cache.writer_count() == 1);
  TEST_ASSERT(sample_cache.state() == SampleCacheState(NO_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE));
}

void test_SampleCache_initialize()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache1;
  setup1(sample_cache1);

  SampleCacheType sample_cache2;
  sample_cache2.initialize(sample_cache1);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sample_cache2.take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache2.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));
  TEST_ASSERT(sample_list[4] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.size() == 5);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 4, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 3, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[4] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));
}

void test_SampleCache_read_all_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 3);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.size() == 3);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_read_N_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, NOT_READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 4);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_read_all_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_read_N_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 1, READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_read_all_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));
  TEST_ASSERT(sample_list[3] == Sample("a", 0));
  TEST_ASSERT(sample_list[4] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 5);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 4, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 3, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[4] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_read_N2_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 4);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_read_N4_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 4, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));
  TEST_ASSERT(sample_list[3] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.size() == 4);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 3, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 5);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_all_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 3);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.size() == 3);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_N_not_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, NOT_READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 2);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_SampleCache_take_all_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_N_read()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 1, READ_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 1);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_all_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 5);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));
  TEST_ASSERT(sample_list[4] == Sample("a", 4));

  TEST_ASSERT(sample_info_list.size() == 5);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 4, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 3, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[4] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 0);
}

void test_SampleCache_take_N2_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 3);
}

void test_SampleCache_take_N4_any()
{
  std::cout << __func__ << std::endl;

  SampleCacheType sample_cache;
  setup1(sample_cache);

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 4, ANY_SAMPLE_STATE);

  const InstanceHandle a_ih = sample_cache.get_instance_handle();

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 0));
  TEST_ASSERT(sample_list[1] == Sample("a", 1));
  TEST_ASSERT(sample_list[2] == Sample("a", 2));
  TEST_ASSERT(sample_list[3] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 4);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 3, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 2, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp, a_ih, 0, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));

  TEST_ASSERT(sample_cache.not_new_size() == 0);
  TEST_ASSERT(sample_cache.new_size() == 1);
}

void test_Sink_initialize()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink1 = make_rch<SinkType>(-1);
  SinkPtrType sink2 = make_rch<SinkType>(-1);

  sink1->write(Sample("a", 1), source_timestamp, 0);
  sink2->initialize(sink1);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  const InstanceHandle a_ih = sink2->lookup_instance(Sample("a", 0));

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));
}

void test_Sink_initialize_limit_depth()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink1 = make_rch<SinkType>(2);
  SinkPtrType sink2 = make_rch<SinkType>(1);

  sink1->write(Sample("a", 0), source_timestamp, 0);
  sink1->write(Sample("a", 1), source_timestamp, 0);
  sink2->initialize(sink1);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  const InstanceHandle a_ih = sink2->lookup_instance(key);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, a_ih, 0, true));
}

void test_Sink_register_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  InstanceHandle ih = sink->lookup_instance(key);
  TEST_ASSERT(ih == 0);

  sink->register_instance(key, source_timestamp, 0);

  ih = sink->lookup_instance(key);
  TEST_ASSERT(ih != 0);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));

  sink->register_instance(key, source_timestamp, publication_handle_1);

  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 0);
  TEST_ASSERT(sample_info_list.size() == 0);

  sink->register_instance(key, source_timestamp, 0);

  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 0);

  TEST_ASSERT(sample_info_list.size() == 0);
}

void test_Sink_write()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink->write(Sample("b", 2), source_timestamp_1, publication_handle_2);
  sink->write(Sample("a", 3), source_timestamp_2, publication_handle_2);
  sink->write(Sample("b", 4), source_timestamp_2, publication_handle_1);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  const InstanceHandle a_ih = sink->lookup_instance(Sample("a", 0));
  const InstanceHandle b_ih = sink->lookup_instance(Sample("b", 0));

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("b", 2));
  TEST_ASSERT(sample_list[3] == Sample("b", 4));

  TEST_ASSERT(sample_info_list.size() == 4);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, a_ih, publication_handle_1, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, a_ih, publication_handle_2, true));
  TEST_ASSERT(sample_info_list[2] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, b_ih, publication_handle_2, true));
  TEST_ASSERT(sample_info_list[3] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih, publication_handle_1, true));
}

void test_Sink_register_write()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->register_instance(Sample("a", 1), source_timestamp_1, publication_handle_1);

  const InstanceHandle a_ih = sink->lookup_instance(Sample("a", 0));

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih, publication_handle_1, false));

  sink->write(Sample("a", 1), source_timestamp_1, publication_handle_1);

  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih, publication_handle_1, true));
}

void test_Sink_unregister_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->register_instance(key, source_timestamp, 0);

  InstanceHandle ih = sink->lookup_instance(key);
  TEST_ASSERT(ih != 0);

  sink->unregister_instance(key, source_timestamp, 0);

  TEST_ASSERT(sink->lookup_instance(key) == ih);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_dispose_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->register_instance(key, source_timestamp, 0);

  InstanceHandle ih = sink->lookup_instance(key);
  TEST_ASSERT(ih != 0);

  sink->dispose_instance(key, source_timestamp, 0);

  TEST_ASSERT(sink->lookup_instance(key) == ih);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_read_not_read_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_not_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);
  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_not_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);
  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_not_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("a", 2), source_timestamp_2, 0);

  const InstanceHandle ih = sink->lookup_instance(Sample("a", 0));

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, ih, 0, true));

  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, ih, 0, true));
}

void test_Sink_read_not_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_read_not_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->read(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_read_read_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(key, source_timestamp, 0);
  sink->register_instance(key, source_timestamp, 0);

  sink->read(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_read_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(key, source_timestamp, 0);
  sink->register_instance(key, source_timestamp, 0);
  sink->dispose_instance(key, source_timestamp, 0);

  sink->read(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_read_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink->register_instance(Sample("a", 1), source_timestamp, 0);
  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->read(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_read_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->read(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink->read(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->read(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_not_read_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_not_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);
  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_not_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);
  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_not_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("a", 2), source_timestamp_2, 0);

  const InstanceHandle ih = sink->lookup_instance(Sample("a", 0));

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, ih, 0, true));

  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, ih, 0, true));
}

void test_Sink_take_not_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_take_not_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->take(sample_list, sample_info_list, -1, NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, false));
}

void test_Sink_take_read_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(key, source_timestamp, 0);
  sink->register_instance(key, source_timestamp, 0);

  sink->take(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_take_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(key, source_timestamp, 0);
  sink->register_instance(key, source_timestamp, 0);
  sink->dispose_instance(key, source_timestamp, 0);

  sink->take(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_take_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink->register_instance(Sample("a", 1), source_timestamp, 0);
  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->take(sample_list, sample_info_list, 1, READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 1, source_timestamp, ih, 0, true));
}

void test_Sink_take_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->take(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink->take(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_take_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read(sample_list, sample_info_list, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));

  sink->unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink->take(sample_list, sample_info_list, -1, READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp, ih, 0, true));
}

void test_Sink_read_next_sample()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("a", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  Sample sample;
  SampleInfo sample_info;
  bool flag = sink->read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 1));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, ih, 0, true));

  flag = sink->read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 2));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, ih, 0, true));

  flag = sink->read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 3));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, ih, 0, true));
}

void test_Sink_take_next_sample()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("a", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  Sample sample;
  SampleInfo sample_info;
  bool flag = sink->take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 1));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, ih, 0, true));

  flag = sink->take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 2));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, ih, 0, true));

  flag = sink->take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 3));
  TEST_ASSERT(sample_info == SampleInfo(NOT_READ_SAMPLE_STATE, NOT_NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, ih, 0, true));
}

void test_Sink_read_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("b", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->read_instance(sample_list, sample_info_list, -1, ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, ih, 0, true));
}

void test_Sink_take_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("b", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle ih = sink->lookup_instance(key);

  SampleList sample_list;
  SampleInfoList sample_info_list;
  sink->take_instance(sample_list, sample_info_list, -1, ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, ih, 0, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, ih, 0, true));
}

void test_Sink_read_next_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("b", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle a_ih = sink->lookup_instance(key);
  const InstanceHandle b_ih = sink->lookup_instance(Sample("b", 0));

  SampleList sample_list;
  SampleInfoList sample_info_list;

  if (a_ih < b_ih) {
    sink->read_next_instance(sample_list, sample_info_list, -1, 0, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.size() == 2);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, a_ih, 0, true));
    TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, a_ih, 0, true));

    sink->read_next_instance(sample_list, sample_info_list, -1, a_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.size() == 1);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih, 0, true));

    sink->read_next_instance(sample_list, sample_info_list, -1, b_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.size() == 0);
  } else {
    sink->read_next_instance(sample_list, sample_info_list, -1, 0, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.size() == 1);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih, 0, true));

    sink->read_next_instance(sample_list, sample_info_list, -1, b_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.size() == 2);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, a_ih, 0, true));
    TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, a_ih, 0, true));

    sink->read_next_instance(sample_list, sample_info_list, -1, a_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.size() == 0);
  }
}

void test_Sink_take_next_instance()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("b", 2), source_timestamp_2, 0);
  sink->write(Sample("a", 3), source_timestamp_3, 0);

  const InstanceHandle a_ih = sink->lookup_instance(key);
  const InstanceHandle b_ih = sink->lookup_instance(Sample("b", 0));

  SampleList sample_list;
  SampleInfoList sample_info_list;

  if (a_ih < b_ih) {
    sink->take_next_instance(sample_list, sample_info_list, -1, 0, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.size() == 2);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, a_ih, 0, true));
    TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, a_ih, 0, true));

    sink->take_next_instance(sample_list, sample_info_list, -1, a_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.size() == 1);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih, 0, true));

    sink->take_next_instance(sample_list, sample_info_list, -1, b_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.size() == 0);
  } else {
    sink->take_next_instance(sample_list, sample_info_list, -1, 0, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.size() == 1);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih, 0, true));

    sink->take_next_instance(sample_list, sample_info_list, -1, b_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.size() == 2);
    TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, source_timestamp_1, a_ih, 0, true));
    TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, a_ih, 0, true));

    sink->take_next_instance(sample_list, sample_info_list, -1, a_ih, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.size() == 0);
  }
}

void test_Sink_get_key_value()
{
  std::cout << __func__ << std::endl;

  SinkPtrType sink = make_rch<SinkType>(-1);

  sink->write(Sample("a", 1), source_timestamp_1, 0);
  sink->write(Sample("b", 2), source_timestamp_2, 0);

  const InstanceHandle a_ih = sink->lookup_instance(key);
  const InstanceHandle b_ih = sink->lookup_instance(Sample("b", 0));

  Sample sample;
  bool flag;

  flag = sink->get_key_value(sample, a_ih);
  TEST_ASSERT(flag == true);
  TEST_ASSERT(sample == Sample("a", 1));

  flag = sink->get_key_value(sample, b_ih);
  TEST_ASSERT(flag == true);
  TEST_ASSERT(sample == Sample("b", 2));

  flag = sink->get_key_value(sample, 0);
  TEST_ASSERT(flag == false);
}

void test_Source_register_instance()
{
  std::cout << __func__ << std::endl;

  SourcePtrType source_1 = make_rch<SourceType>(-1);
  SourcePtrType source_2 = make_rch<SourceType>(-1);

  SinkPtrType sink_1 = make_rch<SinkType>(-1);
  SinkPtrType sink_2 = make_rch<SinkType>(-1);
  SinkPtrType sink_3 = make_rch<SinkType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_2);

  const InstanceHandle a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const InstanceHandle a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_3 = sink_3->lookup_instance(Sample("b", 2));
  const PublicationHandle ph_1 = source_1->get_publication_handle();
  const PublicationHandle ph_2 = source_2->get_publication_handle();

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih_1, ph_1, false));

  sink_2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih_2, ph_2, false));

  sink_3->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih_3, ph_1, false));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih_3, ph_2, false));
}

void test_Source_write()
{
  std::cout << __func__ << std::endl;

  SourcePtrType source_1 = make_rch<SourceType>(-1);
  SourcePtrType source_2 = make_rch<SourceType>(-1);

  SinkPtrType sink_1 = make_rch<SinkType>(-1);
  SinkPtrType sink_2 = make_rch<SinkType>(-1);
  SinkPtrType sink_3 = make_rch<SinkType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->write(Sample("a", 1), source_timestamp_1);
  source_2->write(Sample("b", 2), source_timestamp_2);

  const InstanceHandle a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const InstanceHandle a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_3 = sink_3->lookup_instance(Sample("b", 2));
  const PublicationHandle ph_1 = source_1->get_publication_handle();
  const PublicationHandle ph_2 = source_2->get_publication_handle();

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih_1, ph_1, true));

  sink_2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih_2, ph_2, true));

  sink_3->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih_3, ph_1, true));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, b_ih_3, ph_2, true));
}

void test_Source_unregister_instance()
{
  std::cout << __func__ << std::endl;

  SourcePtrType source_1 = make_rch<SourceType>(-1);
  SourcePtrType source_2 = make_rch<SourceType>(-1);

  SinkPtrType sink_1 = make_rch<SinkType>(-1);
  SinkPtrType sink_2 = make_rch<SinkType>(-1);
  SinkPtrType sink_3 = make_rch<SinkType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_1);

  source_1->unregister_instance(Sample("a", 1), source_timestamp_2);
  source_2->unregister_instance(Sample("b", 2), source_timestamp_3);

  const InstanceHandle a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const InstanceHandle a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_3 = sink_3->lookup_instance(Sample("b", 2));
  const PublicationHandle ph_1 = source_1->get_publication_handle();
  const PublicationHandle ph_2 = source_2->get_publication_handle();

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, a_ih_1, ph_1, false));

  sink_2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, b_ih_2, ph_2, false));

  sink_3->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, a_ih_3, ph_1, false));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, b_ih_3, ph_2, false));
}

void test_Source_dispose_instance()
{
  std::cout << __func__ << std::endl;

  SourcePtrType source_1 = make_rch<SourceType>(-1);
  SourcePtrType source_2 = make_rch<SourceType>(-1);

  SinkPtrType sink_1 = make_rch<SinkType>(-1);
  SinkPtrType sink_2 = make_rch<SinkType>(-1);
  SinkPtrType sink_3 = make_rch<SinkType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_1);

  source_1->dispose_instance(Sample("a", 1), source_timestamp_2);
  source_2->dispose_instance(Sample("b", 2), source_timestamp_3);

  const InstanceHandle a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const InstanceHandle a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const InstanceHandle b_ih_3 = sink_3->lookup_instance(Sample("b", 2));
  const PublicationHandle ph_1 = source_1->get_publication_handle();
  const PublicationHandle ph_2 = source_2->get_publication_handle();

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, a_ih_1, ph_1, false));

  sink_2->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, b_ih_2, ph_2, false));

  sink_3->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.size() == 2);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_2, a_ih_3, ph_1, false));
  TEST_ASSERT(sample_info_list[1] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_3, b_ih_3, ph_2, false));
}

void test_Source_disconnect()
{
  std::cout << __func__ << std::endl;

  SourcePtrType source_1 = make_rch<SourceType>(-1);

  SinkPtrType sink_1 = make_rch<SinkType>(-1);

  source_1->connect(sink_1);
  source_1->write(Sample("a", 1), source_timestamp_1);
  source_1->disconnect(sink_1);
  source_1->write(Sample("b", 2), source_timestamp_2);

  const InstanceHandle a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const PublicationHandle ph_1 = source_1->get_publication_handle();

  SampleList sample_list;
  SampleInfoList sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.size() == 1);
  TEST_ASSERT(sample_info_list[0] == SampleInfo(NOT_READ_SAMPLE_STATE, NEW_VIEW_STATE, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, source_timestamp_1, a_ih_1, ph_1, true));
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

    test_Sink_initialize();
    test_Sink_initialize_limit_depth();
    test_Sink_register_instance();
    test_Sink_write();
    test_Sink_register_write();
    test_Sink_unregister_instance();
    test_Sink_dispose_instance();
    test_Sink_read_not_read_new_alive();
    test_Sink_read_not_read_new_not_alive_disposed();
    test_Sink_read_not_read_new_not_alive_no_writers();
    test_Sink_read_not_read_not_new_alive();
    test_Sink_read_not_read_not_new_not_alive_disposed();
    test_Sink_read_not_read_not_new_not_alive_no_writers();
    test_Sink_read_read_new_alive();
    test_Sink_read_read_new_not_alive_disposed();
    test_Sink_read_read_new_not_alive_no_writers();
    test_Sink_read_read_not_new_alive();
    test_Sink_read_read_not_new_not_alive_disposed();
    test_Sink_read_read_not_new_not_alive_no_writers();
    test_Sink_take_not_read_new_alive();
    test_Sink_take_not_read_new_not_alive_disposed();
    test_Sink_take_not_read_new_not_alive_no_writers();
    test_Sink_take_not_read_not_new_alive();
    test_Sink_take_not_read_not_new_not_alive_disposed();
    test_Sink_take_not_read_not_new_not_alive_no_writers();
    test_Sink_take_read_new_alive();
    test_Sink_take_read_new_not_alive_disposed();
    test_Sink_take_read_new_not_alive_no_writers();
    test_Sink_take_read_not_new_alive();
    test_Sink_take_read_not_new_not_alive_disposed();
    test_Sink_take_read_not_new_not_alive_no_writers();
    test_Sink_read_next_sample();
    test_Sink_take_next_sample();
    test_Sink_read_instance();
    test_Sink_take_instance();
    test_Sink_read_next_instance();
    test_Sink_take_next_instance();
    test_Sink_get_key_value();

    test_Source_register_instance();
    test_Source_write();
    test_Source_unregister_instance();
    test_Source_dispose_instance();
    test_Source_disconnect();
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  return 0;
}
