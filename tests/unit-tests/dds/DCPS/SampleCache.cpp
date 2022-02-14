#include <gtest/gtest.h>

#include "dds/DCPS/SampleCache.h"

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
typedef SampleCache2<Sample> SampleCache2Type;
typedef SampleCache2Type::SampleCache2Ptr SampleCachePtrType;
typedef SampleCache2Type::SampleList SampleList;
}

TEST(SampleCache, register_instance_new)
{
  SampleCacheType sample_cache;
  sample_cache.register_instance(publication_handle_1, key, source_timestamp);
  EXPECT_EQ(sample_cache.instance_count(), 1U);
  DDS::ViewStateKind view_state;
  DDS::InstanceStateKind instance_state;
  const bool has_instance = sample_cache.instance_state(key, view_state, instance_state);
  EXPECT_TRUE(has_instance);
  EXPECT_EQ(view_state, DDS::NEW_VIEW_STATE);
  EXPECT_EQ(instance_state, DDS::ALIVE_INSTANCE_STATE);
}

TEST(SampleCache, register_instance_twice)
{
  SampleCacheType sample_cache;
  sample_cache.register_instance(publication_handle_1, key, source_timestamp);
  sample_cache.register_instance(publication_handle_1, key, source_timestamp);
  EXPECT_EQ(sample_cache.instance_count(), 1U);
  DDS::ViewStateKind view_state;
  DDS::InstanceStateKind instance_state;
  const bool has_instance = sample_cache.instance_state(key, view_state, instance_state);
  EXPECT_TRUE(has_instance);
  EXPECT_EQ(view_state, DDS::NEW_VIEW_STATE);
  EXPECT_EQ(instance_state, DDS::ALIVE_INSTANCE_STATE);
}

TEST(SampleCache, unregister_instance)
{
  SampleCacheType sample_cache;
  sample_cache.register_instance(publication_handle_1, key, source_timestamp);
  sample_cache.unregister_instance(publication_handle_1, key, source_timestamp);
  EXPECT_EQ(sample_cache.instance_count(), 1U);
  DDS::ViewStateKind view_state;
  DDS::InstanceStateKind instance_state;
  const bool has_instance = sample_cache.instance_state(key, view_state, instance_state);
  EXPECT_TRUE(has_instance);
  EXPECT_EQ(view_state, DDS::NEW_VIEW_STATE);
  EXPECT_EQ(instance_state, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
}

TEST(SampleCache, dispose_instance)
{
  SampleCacheType sample_cache;
  sample_cache.register_instance(publication_handle_1, key, source_timestamp);
  sample_cache.dispose_instance(publication_handle_1, key, source_timestamp);
  EXPECT_EQ(sample_cache.instance_count(), 1U);
  DDS::ViewStateKind view_state;
  DDS::InstanceStateKind instance_state;
  const bool has_instance = sample_cache.instance_state(key, view_state, instance_state);
  EXPECT_TRUE(has_instance);
  EXPECT_EQ(view_state, DDS::NEW_VIEW_STATE);
  EXPECT_EQ(instance_state, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
}



TEST(SampleCache2, ctor)
{
  const DDS::InstanceHandle_t ih = 87;

  SampleCache2Type sample_cache(ih);
  EXPECT_TRUE(sample_cache.empty());
  EXPECT_TRUE(sample_cache.size() == 0);
  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 0);
  EXPECT_TRUE(sample_cache.state() == SampleCache2State(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
  EXPECT_TRUE(sample_cache.get_instance_handle() == ih);
}

// Setup a cache with three not read samples and two read samples.
void setup1(SampleCache2Type& sample_cache)
{
  EXPECT_TRUE(sample_cache.empty());

  sample_cache.store(Sample("a", 0), source_timestamp, publication_handle_1);
  sample_cache.store(Sample("a", 1), source_timestamp, publication_handle_1);

  EXPECT_TRUE(sample_cache.new_size() == 2);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_cache.not_new_size() == 2);

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  sample_cache.store(Sample("a", 2), source_timestamp, publication_handle_1);
  sample_cache.store(Sample("a", 3), source_timestamp, publication_handle_1);
  sample_cache.store(Sample("a", 4), source_timestamp, publication_handle_1);

  EXPECT_TRUE(!sample_cache.empty());
  EXPECT_TRUE(sample_cache.size() == 5);
  EXPECT_TRUE(sample_cache.new_size() == 3);
  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.state() == SampleCache2State( DDS::READ_SAMPLE_STATE | DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
}

TEST(SampleCache2, resize)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  sample_cache.resize(5);
  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.new_size() == 3);

  sample_cache.resize(4);
  EXPECT_TRUE(sample_cache.not_new_size() == 1);
  EXPECT_TRUE(sample_cache.new_size() == 3);

  sample_cache.resize(3);
  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 3);

  sample_cache.resize(2);
  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 2);

  sample_cache.resize(1);
  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 1);

  sample_cache.resize(0);
  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, register_instance)
{
  SampleCache2Type sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  EXPECT_TRUE(sample_cache.writer_count() == 1);
  EXPECT_TRUE(sample_cache.state() == SampleCache2State(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE));
}

TEST(SampleCache2, unregister_instance)
{
  SampleCache2Type sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.unregister_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  EXPECT_TRUE(sample_cache.writer_count() == 0);
  EXPECT_TRUE(sample_cache.state() == SampleCache2State(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE));
}

TEST(SampleCache2, dispose_instance)
{
  SampleCache2Type sample_cache(0);
  sample_cache.register_instance(key, source_timestamp, 0);
  sample_cache.dispose_instance(key, source_timestamp, 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, a_ih, 0, 0, 0, 0, 0, 0, false));

  EXPECT_TRUE(sample_cache.writer_count() == 1);
  EXPECT_TRUE(sample_cache.state() == SampleCache2State(DDS::NO_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE));
}

TEST(SampleCache2, initialize)
{
  SampleCache2Type sample_cache1(instance_handle_a);
  setup1(sample_cache1);

  SampleCache2Type sample_cache2(instance_handle_a);
  sample_cache2.initialize(sample_cache1);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sample_cache2.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  EXPECT_TRUE(sample_list.size() == 5);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));
  EXPECT_TRUE(sample_list[2] == Sample("a", 2));
  EXPECT_TRUE(sample_list[3] == Sample("a", 3));
  EXPECT_TRUE(sample_list[4] == Sample("a", 4));

  EXPECT_TRUE(sample_info_list.length() == 5);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 4, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 3, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[4] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, instance_handle_a, publication_handle_1, 0, 0, 0, 0, 0, true));
}

TEST(SampleCache2, read_all_not_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 3);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));
  EXPECT_TRUE(sample_list[2] == Sample("a", 4));

  EXPECT_TRUE(sample_info_list.length() == 3);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 5);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, read_N_not_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 4);
  EXPECT_TRUE(sample_cache.new_size() == 1);
}

TEST(SampleCache2, read_all_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.new_size() == 3);
}

TEST(SampleCache2, read_N_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.new_size() == 3);
}

TEST(SampleCache2, read_all_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 5);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));
  EXPECT_TRUE(sample_list[2] == Sample("a", 4));
  EXPECT_TRUE(sample_list[3] == Sample("a", 0));
  EXPECT_TRUE(sample_list[4] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 5);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 4, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[4] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 5);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, read_N2_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 2, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 4);
  EXPECT_TRUE(sample_cache.new_size() == 1);
}

TEST(SampleCache2, read_N4_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.read(sample_list, sample_info_list, 4, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 4);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));
  EXPECT_TRUE(sample_list[2] == Sample("a", 4));
  EXPECT_TRUE(sample_list[3] == Sample("a", 0));

  EXPECT_TRUE(sample_info_list.length() == 4);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 5);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, take_all_not_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 3);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));
  EXPECT_TRUE(sample_list[2] == Sample("a", 4));

  EXPECT_TRUE(sample_info_list.length() == 3);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, take_N_not_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, DDS::NOT_READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 2);
  EXPECT_TRUE(sample_cache.new_size() == 1);
}

TEST(SampleCache2, take_all_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 3);
}

TEST(SampleCache2, take_N_read)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 1);
  EXPECT_TRUE(sample_cache.new_size() == 3);
}

TEST(SampleCache2, take_all_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 5);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));
  EXPECT_TRUE(sample_list[2] == Sample("a", 2));
  EXPECT_TRUE(sample_list[3] == Sample("a", 3));
  EXPECT_TRUE(sample_list[4] == Sample("a", 4));

  EXPECT_TRUE(sample_info_list.length() == 5);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 4, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[4] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 0);
}

TEST(SampleCache2, take_N2_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 2, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 3);
}

TEST(SampleCache2, take_N4_any)
{
  SampleCache2Type sample_cache(0);
  setup1(sample_cache);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sample_cache.take(sample_list, sample_info_list, 4, DDS::ANY_SAMPLE_STATE);

  const DDS::InstanceHandle_t a_ih = sample_cache.get_instance_handle();

  EXPECT_TRUE(sample_list.size() == 4);
  EXPECT_TRUE(sample_list[0] == Sample("a", 0));
  EXPECT_TRUE(sample_list[1] == Sample("a", 1));
  EXPECT_TRUE(sample_list[2] == Sample("a", 2));
  EXPECT_TRUE(sample_list[3] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 4);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 3, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 2, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

  EXPECT_TRUE(sample_cache.not_new_size() == 0);
  EXPECT_TRUE(sample_cache.new_size() == 1);
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
