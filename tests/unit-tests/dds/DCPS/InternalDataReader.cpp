#include <gtest/gtest.h>


#include "dds/DCPS/InternalDataReader.h"
#include "dds/DCPS/InternalDataWriter.h"

#include "ace/Select_Reactor.h"
#include "ace/Reactor.h"

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

class TestSchedulable : public Schedulable {
public:
  TestSchedulable()
    : scheduled(false)
  {}

  virtual void schedule()
  {
    scheduled = true;
  }

  bool scheduled;
};

const Sample key("a", 0);
const DDS::Time_t source_timestamp = { 0, 0 };
const DDS::Time_t source_timestamp_1 = { 1, 0 };
const DDS::Time_t source_timestamp_2 = { 2, 0 };
const DDS::Time_t source_timestamp_3 = { 3, 0 };
const DDS::InstanceHandle_t publication_handle_1 = 1;
const DDS::InstanceHandle_t publication_handle_2 = 2;
typedef SampleCache2<Sample> SampleCache2Type;
typedef SampleCache2Type::SampleCache2Ptr SampleCache2PtrType;
typedef SampleCache2Type::SampleList SampleList;
typedef InternalDataReader<Sample> InternalDataReaderType;
typedef RcHandle<InternalDataReaderType> InternalDataReaderPtrType;
typedef InternalDataWriter<Sample> InternalDataWriterType;
typedef InternalDataWriterType::InternalDataWriterPtr InternalDataWriterPtrType;
typedef RcHandle<TestSchedulable> ObserverPtrType;
}

TEST(InternalDataReader, initialize)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink1;
  InternalDataReaderType sink2(-1);
  sink2.set_observer(observer);

  sink1.store(Sample("a", 1), source_timestamp, publication_handle_1);
  sink2.initialize(sink1);

  EXPECT_TRUE(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink2.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink2.lookup_instance(Sample("a", 0));

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, initialize_limit_depth)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink1(2);
  InternalDataReaderType sink2(1);
  sink2.set_observer(observer);

  sink1.store(Sample("a", 0), source_timestamp, publication_handle_1);
  sink1.store(Sample("a", 1), source_timestamp, publication_handle_1);
  sink2.initialize(sink1);

  EXPECT_TRUE(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink2.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink2.lookup_instance(key);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, register_instance)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  EXPECT_TRUE(ih == 0);

  sink.register_instance(key, source_timestamp, 0);

  EXPECT_TRUE(observer->scheduled == true);

  ih = sink.lookup_instance(key);
  EXPECT_TRUE(ih != 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));

  sink.register_instance(key, source_timestamp, publication_handle_1);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 0);
  EXPECT_TRUE(sample_info_list.length() == 0);

  sink.register_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 0);

  EXPECT_TRUE(sample_info_list.length() == 0);
}

TEST(InternalDataReader, store)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.store(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.store(Sample("b", 2), source_timestamp_1, publication_handle_2);
  sink.store(Sample("a", 3), source_timestamp_2, publication_handle_2);
  sink.store(Sample("b", 4), source_timestamp_2, publication_handle_1);

  EXPECT_TRUE(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(Sample("a", 0));
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  EXPECT_TRUE(sample_list.size() == 4);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));
  EXPECT_TRUE(sample_list[2] == Sample("b", 2));
  EXPECT_TRUE(sample_list[3] == Sample("b", 4));

  EXPECT_TRUE(sample_info_list.length() == 4);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, a_ih, publication_handle_2, 0, 0, 0, 0, 0, true));
  EXPECT_TRUE(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, b_ih, publication_handle_2, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, register_store)
{
  InternalDataReaderType sink;

  sink.register_instance(Sample("a", 1), source_timestamp_1, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 0, 0, 0, false));

  sink.store(Sample("a", 1), source_timestamp_1, publication_handle_1);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, unregister_instance)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.register_instance(key, source_timestamp, 0);

  EXPECT_TRUE(observer->scheduled == true);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  EXPECT_TRUE(ih != 0);

  sink.unregister_instance(key, source_timestamp, 0);

  EXPECT_TRUE(sink.lookup_instance(key) == ih);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, dispose_instance)
{
  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.register_instance(key, source_timestamp, 0);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  EXPECT_TRUE(ih != 0);

  sink.dispose_instance(key, source_timestamp, 0);

  EXPECT_TRUE(observer->scheduled == true);

  EXPECT_TRUE(sink.lookup_instance(key) == ih);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == key);

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, read_not_read_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_not_read_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);
  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_not_read_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_not_read_not_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("a", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_not_read_not_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, read_not_read_not_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, read_read_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, read_read_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);
  sink.dispose_instance(key, source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, read_read_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink.register_instance(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, read_read_not_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_read_not_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_read_not_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_not_read_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_not_read_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);
  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_not_read_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_not_read_not_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("a", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 2));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_not_read_not_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, take_not_read_not_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

TEST(InternalDataReader, take_read_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, take_read_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);
  sink.dispose_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, take_read_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink.register_instance(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

TEST(InternalDataReader, take_read_not_new_alive)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_read_not_new_not_alive_disposed)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_read_not_new_not_alive_no_writers)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));

  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_next_sample)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("a", 2), source_timestamp_2, 0);
  sink.store(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  Sample sample;
  DDS::SampleInfo sample_info;
  bool flag = sink.read_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 1));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.read_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 2));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.read_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 3));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_next_sample)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("a", 2), source_timestamp_2, 0);
  sink.store(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  Sample sample;
  DDS::SampleInfo sample_info;
  bool flag = sink.take_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 1));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.take_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 2));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.take_next_sample(sample, sample_info);

  EXPECT_TRUE(flag);
  EXPECT_TRUE(sample == Sample("a", 3));
  EXPECT_TRUE(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_instance)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("b", 2), source_timestamp_2, 0);
  sink.store(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read_instance(sample_list, sample_info_list, -1, ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, take_instance)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("b", 2), source_timestamp_2, 0);
  sink.store(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take_instance(sample_list, sample_info_list, -1, ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_TRUE(sample_list.size() == 2);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_list[1] == Sample("a", 3));

  EXPECT_TRUE(sample_info_list.length() == 2);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 1, 0, 0, true));
  EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

TEST(InternalDataReader, read_next_instance)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.store(Sample("b", 2), source_timestamp_2, publication_handle_1);
  sink.store(Sample("a", 3), source_timestamp_3, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  if (a_ih < b_ih) {
    sink.read_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 2);
    EXPECT_TRUE(sample_list[0] == Sample("a", 1));
    EXPECT_TRUE(sample_list[1] == Sample("a", 3));

    EXPECT_TRUE(sample_info_list.length() == 2);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 1);
    EXPECT_TRUE(sample_list[0] == Sample("b", 2));

    EXPECT_TRUE(sample_info_list.length() == 1);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 0);
    EXPECT_TRUE(sample_info_list.length() == 0);
  } else {
    sink.read_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 1);
    EXPECT_TRUE(sample_list[0] == Sample("b", 2));

    EXPECT_TRUE(sample_info_list.length() == 1);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 2);
    EXPECT_TRUE(sample_list[0] == Sample("a", 1));
    EXPECT_TRUE(sample_list[1] == Sample("a", 3));

    EXPECT_TRUE(sample_info_list.length() == 2);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 0);
    EXPECT_TRUE(sample_info_list.length() == 0);
  }
}

TEST(InternalDataReader, take_next_instance)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.store(Sample("b", 2), source_timestamp_2, publication_handle_1);
  sink.store(Sample("a", 3), source_timestamp_3, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  if (a_ih < b_ih) {
    sink.take_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 2);
    EXPECT_TRUE(sample_list[0] == Sample("a", 1));
    EXPECT_TRUE(sample_list[1] == Sample("a", 3));

    EXPECT_TRUE(sample_info_list.length() == 2);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 1);
    EXPECT_TRUE(sample_list[0] == Sample("b", 2));

    EXPECT_TRUE(sample_info_list.length() == 1);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 0);
    EXPECT_TRUE(sample_info_list.length() == 0);
  } else {
    sink.take_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 1);
    EXPECT_TRUE(sample_list[0] == Sample("b", 2));

    EXPECT_TRUE(sample_info_list.length() == 1);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 2);
    EXPECT_TRUE(sample_list[0] == Sample("a", 1));
    EXPECT_TRUE(sample_list[1] == Sample("a", 3));

    EXPECT_TRUE(sample_info_list.length() == 2);
    EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    EXPECT_TRUE(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    EXPECT_TRUE(sample_list.size() == 0);
    EXPECT_TRUE(sample_info_list.length() == 0);
  }
}

TEST(InternalDataReader, get_key_value)
{
  InternalDataReaderType sink;

  sink.store(Sample("a", 1), source_timestamp_1, 0);
  sink.store(Sample("b", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  Sample sample;
  bool flag;

  flag = sink.get_key_value(sample, a_ih);
  EXPECT_TRUE(flag == true);
  EXPECT_TRUE(sample == Sample("a", 1));

  flag = sink.get_key_value(sample, b_ih);
  EXPECT_TRUE(flag == true);
  EXPECT_TRUE(sample == Sample("b", 2));

  flag = sink.get_key_value(sample, 0);
  EXPECT_TRUE(flag == false);
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
