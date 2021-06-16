
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
// TODO: Convert to GTEST.
#include "../../../DCPS/common/TestSupport.h"

#include "dds/DCPS/InternalDataWriter.h"

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
typedef InternalDataWriter<Sample> InternalDataWriterType;
typedef InternalDataWriterType::InternalDataWriterPtr InternalDataWriterPtrType;
typedef RcHandle<TestSchedulable> ObserverPtrType;

void test_InternalDataWriter_register_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataWriterPtrType source_1 = make_rch<InternalDataWriterType>(publication_handle_1, -1);
  InternalDataWriterPtrType source_2 = make_rch<InternalDataWriterType>(publication_handle_2, -1);
  TEST_ASSERT(source_1->get_publication_handle() == publication_handle_1);
  TEST_ASSERT(source_2->get_publication_handle() == publication_handle_2);

  InternalDataReaderPtrType sink_1 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_2 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_3 = make_rch<InternalDataReaderType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_2);

  const DDS::InstanceHandle_t a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const DDS::InstanceHandle_t a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_3 = sink_3->lookup_instance(Sample("b", 2));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_1, publication_handle_1, 0, 0, 0, 0, 0, false));

  sink_2->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih_2, publication_handle_2, 0, 0, 0, 0, 0, false));

  sink_3->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_3, publication_handle_1, 0, 0, 0, 0, 0, false));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih_3, publication_handle_2, 0, 0, 0, 0, 0, false));
}

void test_InternalDataWriter_write()
{
  std::cout << __func__ << std::endl;

  InternalDataWriterPtrType source_1 = make_rch<InternalDataWriterType>(publication_handle_1, -1);
  InternalDataWriterPtrType source_2 = make_rch<InternalDataWriterType>(publication_handle_2, -1);

  InternalDataReaderPtrType sink_1 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_2 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_3 = make_rch<InternalDataReaderType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->write(Sample("a", 1), source_timestamp_1);
  source_2->write(Sample("b", 2), source_timestamp_2);

  const DDS::InstanceHandle_t a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const DDS::InstanceHandle_t a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_3 = sink_3->lookup_instance(Sample("b", 2));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_1, publication_handle_1, 0, 0, 0, 0, 0, true));

  sink_2->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih_2, publication_handle_2, 0, 0, 0, 0, 0, true));

  sink_3->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_3, publication_handle_1, 0, 0, 0, 0, 0, true));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_2, b_ih_3, publication_handle_2, 0, 0, 0, 0, 0, true));
}

void test_InternalDataWriter_unregister_instance()
{
  std::cout << __func__ << std::endl;

 InternalDataWriterPtrType source_1 = make_rch<InternalDataWriterType>(publication_handle_1, -1);
  InternalDataWriterPtrType source_2 = make_rch<InternalDataWriterType>(publication_handle_2, -1);

  InternalDataReaderPtrType sink_1 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_2 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_3 = make_rch<InternalDataReaderType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_1);

  source_1->unregister_instance(Sample("a", 1), source_timestamp_2);
  source_2->unregister_instance(Sample("b", 2), source_timestamp_3);

  const DDS::InstanceHandle_t a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const DDS::InstanceHandle_t a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_3 = sink_3->lookup_instance(Sample("b", 2));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_2, a_ih_1, publication_handle_1, 0, 0, 0, 0, 0, false));

  sink_2->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_3, b_ih_2, publication_handle_2, 0, 0, 0, 0, 0, false));

  sink_3->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_2, a_ih_3, publication_handle_1, 0, 0, 0, 0, 0, false));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_3, b_ih_3, publication_handle_2, 0, 0, 0, 0, 0, false));
}

void test_InternalDataWriter_dispose_instance()
{
  std::cout << __func__ << std::endl;

  InternalDataWriterPtrType source_1 = make_rch<InternalDataWriterType>(publication_handle_1, -1);
  InternalDataWriterPtrType source_2 = make_rch<InternalDataWriterType>(publication_handle_2, -1);

  InternalDataReaderPtrType sink_1 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_2 = make_rch<InternalDataReaderType>(-1);
  InternalDataReaderPtrType sink_3 = make_rch<InternalDataReaderType>(-1);

  source_1->connect(sink_1);
  source_1->connect(sink_3);

  source_2->connect(sink_2);
  source_2->connect(sink_3);

  source_1->register_instance(Sample("a", 1), source_timestamp_1);
  source_2->register_instance(Sample("b", 2), source_timestamp_1);

  source_1->dispose_instance(Sample("a", 1), source_timestamp_2);
  source_2->dispose_instance(Sample("b", 2), source_timestamp_3);

  const DDS::InstanceHandle_t a_ih_1 = sink_1->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_2 = sink_2->lookup_instance(Sample("b", 2));
  const DDS::InstanceHandle_t a_ih_3 = sink_3->lookup_instance(Sample("a", 1));
  const DDS::InstanceHandle_t b_ih_3 = sink_3->lookup_instance(Sample("b", 2));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp_2, a_ih_1, publication_handle_1, 0, 0, 0, 0, 0, false));

  sink_2->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp_3, b_ih_2, publication_handle_2, 0, 0, 0, 0, 0, false));

  sink_3->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 2);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_list[1] == Sample("b", 2));
  TEST_ASSERT(sample_info_list.length() == 2);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp_2, a_ih_3, publication_handle_1, 0, 0, 0, 0, 0, false));
  TEST_ASSERT(sample_info_list[1] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp_3, b_ih_3, publication_handle_2, 0, 0, 0, 0, 0, false));
}

void test_InternalDataWriter_disconnect()
{
  std::cout << __func__ << std::endl;

  InternalDataWriterPtrType source_1 = make_rch<InternalDataWriterType>(publication_handle_1, -1);

  InternalDataReaderPtrType sink_1 = make_rch<InternalDataReaderType>(-1);

  source_1->connect(sink_1);
  source_1->write(Sample("a", 1), source_timestamp_1);
  source_1->disconnect(sink_1);
  source_1->write(Sample("b", 2), source_timestamp_2);

  const DDS::InstanceHandle_t a_ih_1 = sink_1->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  sink_1->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  TEST_ASSERT(sample_list.size() == 1);
  TEST_ASSERT(sample_list[0] == Sample("a", 1));
  TEST_ASSERT(sample_info_list.length() == 1);
  TEST_ASSERT(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_1, a_ih_1, publication_handle_1, 0, 0, 0, 0, 0, true));
}

}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    test_InternalDataWriter_register_instance();
    test_InternalDataWriter_write();
    test_InternalDataWriter_unregister_instance();
    test_InternalDataWriter_dispose_instance();
    test_InternalDataWriter_disconnect();
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  return 0;
}
