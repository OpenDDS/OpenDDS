
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
// TODO: Convert to GTEST.
#include "../../../DCPS/common/TestSupport.h"

#include "dds/DCPS/Source.h"

#include "ace/Select_Reactor.h"
#include "ace/Reactor.h"

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
typedef SampleCache<Sample> SampleCacheType;
typedef SampleCacheType::SampleCachePtr SampleCachePtrType;
typedef SampleCacheType::SampleList SampleList;
typedef InternalDataReader<Sample> InternalDataReaderType;
typedef RcHandle<InternalDataReaderType> InternalDataReaderPtrType;
typedef Source<Sample> SourceType;
typedef SourceType::SourcePtr SourcePtrType;
typedef RcHandle<TestSchedulable> ObserverPtrType;

void test_InternalDataReader_initialize()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink1;
  InternalDataReaderType sink2(-1);
  sink2.set_observer(observer);

  sink1.write(Sample("a", 1), source_timestamp, publication_handle_1);
  sink2.initialize(sink1);

  TEST_ASSERT(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink2.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink2.lookup_instance(Sample("a", 0));

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_initialize_limit_depth()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink1(2);
  InternalDataReaderType sink2(1);
  sink2.set_observer(observer);

  sink1.write(Sample("a", 0), source_timestamp, publication_handle_1);
  sink1.write(Sample("a", 1), source_timestamp, publication_handle_1);
  sink2.initialize(sink1);

  TEST_ASSERT(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink2.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink2.lookup_instance(key);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_register_instance()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  TEST_ASSERT(ih == 0);

  sink.register_instance(key, source_timestamp, 0);

  TEST_ASSERT(observer->scheduled == true);

  ih = sink.lookup_instance(key);
  TEST_ASSERT(ih != 0);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));

  sink.register_instance(key, source_timestamp, publication_handle_1);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 0);
  TEST_ASSERT(sample_info_list.length() == 0);

  sink.register_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 0);

  TEST_ASSERT(sample_info_list.length() == 0);
}

void test_InternalDataReader_write()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.write(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.write(Sample("b", 2), source_timestamp_1, publication_handle_2);
  sink.write(Sample("a", 3), source_timestamp_2, publication_handle_2);
  sink.write(Sample("b", 4), source_timestamp_2, publication_handle_1);

  TEST_ASSERT(observer->scheduled == true);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(Sample("a", 0));
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  TEST_ASSERT(sample_list.size() == 4);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));
  TEST_ASSERT(sample_list[2] == Sample("b", 2));
  TEST_ASSERT(sample_list[3] == Sample("b", 4));

  TEST_ASSERT(sample_info_list.length() == 4);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, a_ih, publication_handle_2, 0, 0, 0, 0, 0, true));
  TEST_ASSERT(sample_info_list[2] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, b_ih, publication_handle_2, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[3] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_register_write()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.register_instance(Sample("a", 1), source_timestamp_1, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 0, 0, 0, false));

  sink.write(Sample("a", 1), source_timestamp_1, publication_handle_1);

  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_unregister_instance()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.register_instance(key, source_timestamp, 0);

  TEST_ASSERT(observer->scheduled == true);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  TEST_ASSERT(ih != 0);

  sink.unregister_instance(key, source_timestamp, 0);

  TEST_ASSERT(sink.lookup_instance(key) == ih);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_dispose_instance()
{
  std::cout << __func__ << std::endl;

  ObserverPtrType observer = make_rch<TestSchedulable>();

  InternalDataReaderType sink(-1);
  sink.set_observer(observer);

  sink.register_instance(key, source_timestamp, 0);

  DDS::InstanceHandle_t ih = sink.lookup_instance(key);
  TEST_ASSERT(ih != 0);

  sink.dispose_instance(key, source_timestamp, 0);

  TEST_ASSERT(observer->scheduled == true);

  TEST_ASSERT(sink.lookup_instance(key) == ih);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == key);

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_read_not_read_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_not_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);
  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_not_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_not_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("a", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_not_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_read_not_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_read_read_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_read_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);
  sink.dispose_instance(key, source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_read_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink.register_instance(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_read_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.read(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_not_read_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_not_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);
  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_not_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_not_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("a", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(Sample("a", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 2));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_not_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_take_not_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, false));
}

void test_InternalDataReader_take_read_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_take_read_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(key, source_timestamp, 0);
  sink.register_instance(key, source_timestamp, 0);
  sink.dispose_instance(key, source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_take_read_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);
  sink.register_instance(Sample("a", 1), source_timestamp, 0);
  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, 1, DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 1, true));
}

void test_InternalDataReader_take_read_not_new_alive()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_read_not_new_not_alive_disposed()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.dispose_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_read_not_new_not_alive_no_writers()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read(sample_list, sample_info_list, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));

  sink.unregister_instance(Sample("a", 1), source_timestamp, 0);

  sink.take(sample_list, sample_info_list, -1, DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));

  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_next_sample()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("a", 2), source_timestamp_2, 0);
  sink.write(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  Sample sample;
  DDS::SampleInfo sample_info;
  bool flag = sink.read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 1));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 2));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.read_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 3));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_next_sample()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("a", 2), source_timestamp_2, 0);
  sink.write(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  Sample sample;
  DDS::SampleInfo sample_info;
  bool flag = sink.take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 1));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 2));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, ih, 0, 0, 0, 0, 0, 0, true));

  flag = sink.take_next_sample(sample, sample_info);

  TEST_ASSERT(flag);
  TEST_ASSERT(sample == Sample("a", 3));
  TEST_ASSERT(sample_info == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("b", 2), source_timestamp_2, 0);
  sink.write(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.read_instance(sample_list, sample_info_list, -1, ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_take_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("b", 2), source_timestamp_2, 0);
  sink.write(Sample("a", 3), source_timestamp_3, 0);

  const DDS::InstanceHandle_t ih = sink.lookup_instance(key);

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;
  sink.take_instance(sample_list, sample_info_list, -1, ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("a", 3));

  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, ih, 0, 0, 0, 1, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, ih, 0, 0, 0, 0, 0, 0, true));
}

void test_InternalDataReader_read_next_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.write(Sample("b", 2), source_timestamp_2, publication_handle_1);
  sink.write(Sample("a", 3), source_timestamp_3, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  if (a_ih < b_ih) {
    sink.read_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.length() == 2);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.length() == 1);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.length() == 0);
  } else {
    sink.read_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.length() == 1);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.length() == 2);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.read_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.length() == 0);
  }
}

void test_InternalDataReader_take_next_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, publication_handle_1);
  sink.write(Sample("b", 2), source_timestamp_2, publication_handle_1);
  sink.write(Sample("a", 3), source_timestamp_3, publication_handle_1);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  if (a_ih < b_ih) {
    sink.take_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.length() == 2);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.length() == 1);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.length() == 0);
  } else {
    sink.take_next_instance(sample_list, sample_info_list, -1, 0, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 1);
    TEST_ASSERT(sample_list[0] == Sample("b", 2));

    TEST_ASSERT(sample_info_list.length() == 1);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, b_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 2);
    TEST_ASSERT(sample_list[0] == Sample("a", 1));
    TEST_ASSERT(sample_list[1] == Sample("a", 3));

    TEST_ASSERT(sample_info_list.length() == 2);
    TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih, publication_handle_1, 0, 0, 1, 0, 0, true));
    TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_3, a_ih, publication_handle_1, 0, 0, 0, 0, 0, true));

    sink.take_next_instance(sample_list, sample_info_list, -1, a_ih, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

    TEST_ASSERT(sample_list.size() == 0);
    TEST_ASSERT(sample_info_list.length() == 0);
  }
}

void test_InternalDataReader_get_key_value()
{
  std::cout << __func__ << std::endl;

  InternalDataReaderType sink;

  sink.write(Sample("a", 1), source_timestamp_1, 0);
  sink.write(Sample("b", 2), source_timestamp_2, 0);

  const DDS::InstanceHandle_t a_ih = sink.lookup_instance(key);
  const DDS::InstanceHandle_t b_ih = sink.lookup_instance(Sample("b", 0));

  Sample sample;
  bool flag;

  flag = sink.get_key_value(sample, a_ih);
  TEST_ASSERT(flag == true);
  TEST_ASSERT(sample == Sample("a", 1));

  flag = sink.get_key_value(sample, b_ih);
  TEST_ASSERT(flag == true);
  TEST_ASSERT(sample == Sample("b", 2));

  flag = sink.get_key_value(sample, 0);
  TEST_ASSERT(flag == false);
}

}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    test_InternalDataReader_initialize();
    test_InternalDataReader_initialize_limit_depth();
    test_InternalDataReader_register_instance();
    test_InternalDataReader_write();
    test_InternalDataReader_register_write();
    test_InternalDataReader_unregister_instance();
    test_InternalDataReader_dispose_instance();
    test_InternalDataReader_read_not_read_new_alive();
    test_InternalDataReader_read_not_read_new_not_alive_disposed();
    test_InternalDataReader_read_not_read_new_not_alive_no_writers();
    test_InternalDataReader_read_not_read_not_new_alive();
    test_InternalDataReader_read_not_read_not_new_not_alive_disposed();
    test_InternalDataReader_read_not_read_not_new_not_alive_no_writers();
    test_InternalDataReader_read_read_new_alive();
    test_InternalDataReader_read_read_new_not_alive_disposed();
    test_InternalDataReader_read_read_new_not_alive_no_writers();
    test_InternalDataReader_read_read_not_new_alive();
    test_InternalDataReader_read_read_not_new_not_alive_disposed();
    test_InternalDataReader_read_read_not_new_not_alive_no_writers();
    test_InternalDataReader_take_not_read_new_alive();
    test_InternalDataReader_take_not_read_new_not_alive_disposed();
    test_InternalDataReader_take_not_read_new_not_alive_no_writers();
    test_InternalDataReader_take_not_read_not_new_alive();
    test_InternalDataReader_take_not_read_not_new_not_alive_disposed();
    test_InternalDataReader_take_not_read_not_new_not_alive_no_writers();
    test_InternalDataReader_take_read_new_alive();
    test_InternalDataReader_take_read_new_not_alive_disposed();
    test_InternalDataReader_take_read_new_not_alive_no_writers();
    test_InternalDataReader_take_read_not_new_alive();
    test_InternalDataReader_take_read_not_new_not_alive_disposed();
    test_InternalDataReader_take_read_not_new_not_alive_no_writers();
    test_InternalDataReader_read_next_sample();
    test_InternalDataReader_take_next_sample();
    test_InternalDataReader_read_instance();
    test_InternalDataReader_take_instance();
    test_InternalDataReader_read_next_instance();
    test_InternalDataReader_take_next_instance();
    test_InternalDataReader_get_key_value();
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  return 0;
}
