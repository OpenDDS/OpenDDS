#include <dds/DCPS/InternalTopic.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

struct Sample {
  bool operator<(const Sample&) const
  {
    return false;
  }
};

typedef InternalTopic<Sample> TopicType;
typedef InternalDataWriter<Sample> WriterType;
typedef InternalDataReader<Sample> ReaderType;

TEST(dds_DCPS_InternalTopic, connect_writer)
{
  RcHandle<TopicType> topic = make_rch<TopicType>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer = make_rch<WriterType>(false);

  topic->connect(reader);
  topic->connect(writer);

  EXPECT_TRUE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalTopic, connect_reader)
{
  RcHandle<TopicType> topic = make_rch<TopicType>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer = make_rch<WriterType>(false);

  topic->connect(writer);
  topic->connect(reader);

  EXPECT_TRUE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalTopic, disconnect_writer)
{
  RcHandle<TopicType> topic = make_rch<TopicType>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer = make_rch<WriterType>(false);

  topic->connect(reader);
  topic->connect(writer);
  topic->disconnect(writer);

  EXPECT_FALSE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalTopic, disconnect_reader)
{
  RcHandle<TopicType> topic = make_rch<TopicType>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer = make_rch<WriterType>(false);

  topic->connect(writer);
  topic->connect(reader);
  topic->disconnect(reader);

  EXPECT_FALSE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalTopic, connect_multiple)
{
  RcHandle<TopicType> topic = make_rch<TopicType>();
  RcHandle<ReaderType> reader1 = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer1 = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader2 = make_rch<ReaderType>(false);
  RcHandle<WriterType> writer2 = make_rch<WriterType>(false);

  topic->connect(reader1);
  topic->connect(writer1);
  topic->connect(reader2);
  topic->connect(writer2);

  EXPECT_TRUE(writer1->has_reader(reader1));
  EXPECT_TRUE(writer1->has_reader(reader2));
  EXPECT_TRUE(writer2->has_reader(reader1));
  EXPECT_TRUE(writer2->has_reader(reader2));
}
