#include <gtest/gtest.h>

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

const DDS::Time_t source_timestamp = { 1, 0 };
const DDS::InstanceHandle_t publication_handle = 1;

typedef SampleCache2<Sample> SampleCache2Type;
typedef SampleCache2Type::SampleList SampleList;

typedef InternalTopic<Sample> InternalTopicType;
typedef RcHandle<InternalTopicType> InternalTopicPtr;
typedef InternalTopicType::InternalDataWriter InternalDataWriterType;
typedef InternalTopicType::InternalDataWriterPtr InternalDataWriterPtr;
typedef InternalTopicType::InternalDataReader InternalDataReaderType;
typedef InternalTopicType::InternalDataReaderPtr InternalDataReaderPtr;
}

TEST(InternalTopic, create_writer_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  EXPECT_TRUE(topic->has_writer(writer));
}

TEST(InternalTopic, create_reader_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);
  EXPECT_TRUE(topic->has_reader(reader));
}

TEST(InternalTopic, create_writer_non_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  InternalDataReaderPtr reader = topic->create_reader(-1);
  EXPECT_TRUE(topic->has_writer(writer));
  EXPECT_TRUE(topic->has_reader(reader));
}

TEST(InternalTopic, create_reader_non_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  EXPECT_TRUE(topic->has_writer(writer));
  EXPECT_TRUE(topic->has_reader(reader));
}

TEST(InternalTopic, remove_writer_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  DDS::DataWriterQos writer_qos;
  assign_default_internal_data_writer_qos(writer_qos);
  InternalDataWriterPtr writer = topic->create_writer(writer_qos);
  EXPECT_TRUE(topic->has_writer(writer));
  topic->remove_writer(writer);
  EXPECT_FALSE(topic->has_writer(writer));
}

TEST(InternalTopic, remove_reader_empty)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);
  EXPECT_TRUE(topic->has_reader(reader));
  topic->remove_reader(reader);
  EXPECT_FALSE(topic->has_reader(reader));
}

TEST(InternalTopic, register_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);

  topic->register_instance(Sample("a", 1), source_timestamp, publication_handle);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_EQ(sample_list.size(), 1U);
  EXPECT_EQ(sample_list[0], Sample("a", 1));
  EXPECT_EQ(sample_info_list.length(), 1U);
  EXPECT_EQ(sample_info_list[0], make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih_1, publication_handle, 0, 0, 0, 0, 0, false));
}

TEST(InternalTopic, store)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);

  topic->store(Sample("a", 1), source_timestamp, publication_handle);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, source_timestamp, a_ih_1, publication_handle, 0, 0, 0, 0, 0, true));
}

TEST(InternalTopic, unregister_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);

  topic->register_instance(Sample("a", 1), source_timestamp, publication_handle);
  topic->unregister_instance(Sample("a", 1), source_timestamp, publication_handle);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, source_timestamp, a_ih_1, publication_handle, 0, 0, 0, 0, 0, false));
}

TEST(InternalTopic, dispose_instance)
{
  InternalTopicPtr topic = make_rch<InternalTopicType>();
  InternalDataReaderPtr reader = topic->create_reader(-1);

  topic->register_instance(Sample("a", 1), source_timestamp, publication_handle);
  topic->dispose_instance(Sample("a", 1), source_timestamp, publication_handle);

  const DDS::InstanceHandle_t a_ih_1 = reader->lookup_instance(Sample("a", 1));

  SampleList sample_list;
  DDS::SampleInfoSeq sample_info_list;

  reader->read(sample_list, sample_info_list, -1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  EXPECT_TRUE(sample_list.size() == 1);
  EXPECT_TRUE(sample_list[0] == Sample("a", 1));
  EXPECT_TRUE(sample_info_list.length() == 1);
  EXPECT_TRUE(sample_info_list[0] == make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, source_timestamp, a_ih_1, publication_handle, 0, 0, 0, 0, 0, false));
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
