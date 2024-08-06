#include <dds/DCPS/InternalDataWriter.h>

#include <dds/DCPS/Qos_Helper.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
typedef SampleInfoWrapper SIW;

namespace {
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
}

typedef InternalDataWriter<Sample> WriterType;
typedef InternalDataReader<Sample> ReaderType;

TEST(dds_DCPS_InternalDataWriter, add_reader)
{
  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);

  EXPECT_TRUE(writer->has_reader(reader));
}

TEST(dds_DCPS_InternalDataWriter, add_reader_durable)
{
  Sample sample1("key1");
  Sample sample2("key2");
  ASSERT_LT(sample1, sample2);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder().durability_transient_local().history_depth(2));
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable().durability_transient_local().history_keep_all());

  writer->write(sample1);
  writer->write(sample2);
  writer->write(sample2);

  writer->add_reader(reader);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 3U);
  ASSERT_EQ(infos.size(), 3U);

  EXPECT_EQ(samples[0], sample1);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  EXPECT_EQ(samples[1], sample2);
  EXPECT_EQ(SIW(infos[1]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, true)));

  EXPECT_EQ(samples[2], sample2);
  EXPECT_EQ(SIW(infos[2]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, add_reader_durable_history1)
{
  Sample sample1("key1");
  Sample sample2("key2");
  ASSERT_LT(sample1, sample2);

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder().durability_transient_local());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable().durability_transient_local());

  writer->write(sample1);
  writer->write(sample2);
  writer->write(sample2);

  writer->add_reader(reader);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample1);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  EXPECT_EQ(samples[1], sample2);
  EXPECT_EQ(SIW(infos[1]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, remove_reader)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);
  writer->write(sample);
  writer->remove_reader(reader);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  EXPECT_FALSE(writer->has_reader(reader));

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, write)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);

  writer->write(sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, unregister_instance)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);

  writer->write(sample);
  writer->unregister_instance(sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, unregister_instance_no_dispose)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder().writer_data_lifecycle_autodispose_unregistered_instances(false));
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);

  writer->write(sample);
  writer->unregister_instance(sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, dispose)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  writer->add_reader(reader);

  writer->write(sample);
  writer->dispose(sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataWriter, add_instance_reader)
{
  ReaderType::SampleSequence interesting_samples;
  interesting_samples.push_back(Sample("a"));
  interesting_samples.push_back(Sample("c"));

  RcHandle<WriterType> writer = make_rch<WriterType>(DataWriterQosBuilder());
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
  reader->set_interesting_instances(interesting_samples);
  writer->add_reader(reader);

  EXPECT_TRUE(writer->has_reader(reader));
  writer->write(Sample("a"));
  writer->write(Sample("b"));
  writer->write(Sample("c"));

  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], Sample("a"));
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
  EXPECT_EQ(samples[1], Sample("c"));
  EXPECT_EQ(SIW(infos[1]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  writer->dispose(Sample("a"));
  writer->dispose(Sample("b"));

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], Sample("a"));
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, false)));

  writer->unregister_instance(Sample("a"));
  writer->unregister_instance(Sample("b"));
  writer->unregister_instance(Sample("c"));

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], Sample("c"));
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, false)));
}
