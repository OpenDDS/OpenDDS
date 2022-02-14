#include <gtest/gtest.h>

#include "dds/DCPS/InternalDataWriter.h"
#include "dds/DCPS/InternalTopic.h"

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
const DDS::Time_t source_timestamp_1 = { 1, 0 };
const DDS::Time_t source_timestamp_2 = { 2, 0 };
typedef SampleCache2<Sample> SampleCache2Type;
typedef SampleCache2Type::SampleCache2Ptr SampleCache2PtrType;
typedef SampleCache2Type::SampleList SampleList;

typedef InternalTopic<Sample> InternalTopicType;
typedef RcHandle<InternalTopicType> InternalTopicPtr;
typedef InternalTopicType::InternalDataWriter InternalDataWriterType;
typedef InternalTopicType::InternalDataWriterPtr InternalDataWriterPtr;
typedef InternalTopicType::InternalDataReader InternalDataReaderType;
typedef InternalTopicType::InternalDataReaderPtr InternalDataReaderPtr;
typedef RcHandle<TestSchedulable> ObserverPtrType;
}

TEST(InternalDataWriter, register_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  InternalDataReaderPtr reader = topic->create_reader(-1);

  writer->register_instance(Sample("a", 1), source_timestamp_1);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_EQ(sample_list.size(), 1U);
  EXPECT_EQ(sample_list[0], Sample("a", 1));
  EXPECT_EQ(sample_info_list.length(), 1U);
  EXPECT_EQ(sample_info_list[0], make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_1, writer->get_publication_handle(), 0, 0, 0, 0, 0, false));
}

TEST(InternalDataWriter, write)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  InternalDataReaderPtr reader = topic->create_reader(-1);

  writer->write(Sample("a", 1), source_timestamp_1);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp_1, a_ih_1, writer->get_publication_handle(), 0, 0, 0, 0, 0, true));
}

TEST(InternalDataWriter, unregister_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  InternalDataReaderPtr reader = topic->create_reader(-1);

  writer->register_instance(Sample("a", 1), source_timestamp_1);

  writer->unregister_instance(Sample("a", 1), source_timestamp_2);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp_2, a_ih_1, writer->get_publication_handle(), 0, 0, 0, 0, 0, false));
}

TEST(InternalDataWriter, dispose_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  InternalDataReaderPtr reader = topic->create_reader(-1);

  writer->register_instance(Sample("a", 1), source_timestamp_1);
  writer->dispose_instance(Sample("a", 1), source_timestamp_2);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp_2, a_ih_1, writer->get_publication_handle(), 0, 0, 0, 0, 0, false));
}

TEST(InternalDataWriter, assign_default_internal_data_writer_qos)
{
  const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };
  const DDS::Duration_t infinity = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  const DDS::Duration_t one_hundred_ms = { 0, 100000000 };

  DDS::DataWriterQos qos;
  assign_default_internal_data_writer_qos(qos);
  EXPECT_EQ(qos.durability.kind, DDS::VOLATILE_DURABILITY_QOS);
  EXPECT_TRUE(qos.durability_service.service_cleanup_delay == zero);
  EXPECT_EQ(qos.durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.durability_service.history_depth, 1);
  EXPECT_EQ(qos.durability_service.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_TRUE(qos.deadline.period == infinity);
  EXPECT_TRUE(qos.latency_budget.duration == zero);
  EXPECT_EQ(qos.liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(qos.liveliness.lease_duration == infinity);
  EXPECT_EQ(qos.reliability.kind, DDS::RELIABLE_RELIABILITY_QOS);
  EXPECT_TRUE(qos.reliability.max_blocking_time == one_hundred_ms);
  EXPECT_EQ(qos.destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(qos.history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.history.depth, 1);
  EXPECT_EQ(qos.resource_limits.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.transport_priority.value, 0);
  EXPECT_TRUE(qos.lifespan.duration == infinity);
  EXPECT_EQ(qos.user_data.value.length(), 0U);
  EXPECT_EQ(qos.ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
  EXPECT_EQ(qos.ownership_strength.value, 0);
  EXPECT_EQ(qos.writer_data_lifecycle.autodispose_unregistered_instances, true);
  EXPECT_EQ(qos.representation.value.length(), 0U);
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
