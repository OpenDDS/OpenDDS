#include <dds/DCPS/InternalDataWriter.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

struct Sample {
  std::string key;

  Sample()
  {}

  Sample(const std::string& a_key)
    : key(a_key)
  {}

  bool operator==(const Sample& other) const
  {
    return key == other.key;
  }

  bool operator<(const Sample& other) const
  {
    return key < other.key;
  }
};

typedef InternalDataWriter<Sample> WriterType;
typedef InternalDataReader<Sample> ReaderType;

TEST(dds_DCPS_InternalDataWriter, add_reader)
{
  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);

  EXPECT_TRUE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalDataWriter, add_reader_durable)
{
  Sample sample1("key1");
  Sample sample2("key2");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(true);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(true);

  writer->register_instance(sample1);
  writer->write(sample2);

  writer->add_reader(reader);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample1);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, writer->publication_handle()));

  EXPECT_EQ(samples[1], sample2);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_SAMPLE, writer->publication_handle()));
}

TEST(dds_DCPS_InternalDataWriter, remove_reader)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);
  writer->register_instance(sample);
  writer->remove_reader(reader);
  reader->take(samples, infos);

  EXPECT_FALSE(writer->has_reader(reader));

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, writer->publication_handle()));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_UNREGISTER, writer->publication_handle()));
}

TEST(dds_DCPS_InternalDataWriter, register_instance)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);

  writer->register_instance(sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, writer->publication_handle()));
}

TEST(dds_DCPS_InternalDataWriter, write)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);

  writer->write(sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_SAMPLE, writer->publication_handle()));
}

TEST(dds_DCPS_InternalDataWriter, unregister_instance)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);

  writer->register_instance(sample);
  writer->unregister_instance(sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, writer->publication_handle()));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_UNREGISTER, writer->publication_handle()));
}

TEST(dds_DCPS_InternalDataWriter, dispose)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(false);
  RcHandle<ReaderType> reader = make_rch<ReaderType>(false);
  writer->add_reader(reader);

  writer->register_instance(sample);
  writer->dispose(sample);
  reader->take(samples, infos);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(infos[0], InternalSampleInfo(ISIK_REGISTER, writer->publication_handle()));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(infos[1], InternalSampleInfo(ISIK_DISPOSE, writer->publication_handle()));
}
