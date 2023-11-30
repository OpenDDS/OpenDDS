#include <dds/DCPS/InternalDataReader.h>

#include <dds/DCPS/Qos_Helper.h>

#include <tests/Utils/gtestWrapper.h>

using namespace OpenDDS::DCPS;
typedef SampleInfoWrapper SIW;

namespace {
  struct Sample {
    std::string key;

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

std::ostream& operator<<(std::ostream& out, const SIW& siw)
{
  out << '('
      << "ss=" << siw.si.sample_state << ','
      << "vs=" << siw.si.view_state << ','
      << "is=" << siw.si.instance_state << ','
      << "st=" << siw.si.source_timestamp.sec << '.' << siw.si.source_timestamp.nanosec << ','
      << "ih=" << siw.si.instance_handle << ','
      << "ph=" << siw.si.publication_handle << ','
      << "dgc=" << siw.si.disposed_generation_count << ','
      << "nwgc=" << siw.si.no_writers_generation_count << ','
      << "sr=" << siw.si.sample_rank << ','
      << "gr=" << siw.si.generation_rank << ','
      << "agr=" << siw.si.absolute_generation_rank << ','
      << "vd=" << siw.si.valid_data << ')';
  return out;
}

namespace {
  typedef InternalDataReader<Sample> ReaderType;

  class Listener : public InternalDataReaderListener<Sample> {
  public:
    Listener(JobQueue_rch job_queue)
      : InternalDataReaderListener<Sample>(job_queue)
    {}

    MOCK_METHOD1(on_data_available, void(RcHandle<ReaderType>));
  };
}

TEST(dds_DCPS_InternalDataReader, durable)
{
  {
    RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());
    EXPECT_FALSE(reader->durable());
  }
  {
    RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable().durability_transient_local());
    EXPECT_TRUE(reader->durable());
  }
}

TEST(dds_DCPS_InternalDataReader, write)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);
  // Twice to exercise history.
  reader->write(writer, sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, write_keep_all)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable().history_keep_all());

  reader->write(writer, sample);
  reader->write(writer, sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 2U);
  ASSERT_EQ(infos.size(), 2U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 1, 0, 0, true)));

  EXPECT_EQ(samples[1], sample);
  EXPECT_EQ(SIW(infos[1]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, unregister_instance)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);
  reader->unregister_instance(writer, sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, dispose)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);
  reader->dispose(writer, sample);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, remove_publication_autodispose)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);
  reader->remove_publication(writer, true);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, remove_publication)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);
  reader->remove_publication(writer, false);
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, listener)
{
  Sample sample("key");

  JobQueue_rch job_queue = make_rch<JobQueue>(ACE_Reactor::instance());
  RcHandle<Listener> listener = make_rch<Listener>(job_queue);
  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable(),
                                                     listener);

  EXPECT_EQ(reader->get_listener(), listener);

  // This increases the reference count on reader which prevents the destruction and check of the listener.
  EXPECT_CALL(*listener.get(), on_data_available(reader));

  reader->write(writer, sample);

  ACE_Time_Value tv(1,0);
  ACE_Reactor::run_event_loop(tv);

  reader->set_listener(RcHandle<Listener>());
}

TEST(dds_DCPS_InternalDataReader, read)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, read_instance_state)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE | DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->dispose(writer, sample);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE | DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  // Revive the instance.
  reader->write(writer, sample);
  reader->unregister_instance(writer, sample);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE | DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 1, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, take)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);
}

TEST(dds_DCPS_InternalDataReader, take_instance_state)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE | DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->dispose(writer, sample);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE | DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NOT_NEW_VIEW_STATE, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE, 0, 0, 0, 0, 0, false)));

  // Revive the instance.
  reader->write(writer, sample);
  reader->unregister_instance(writer, sample);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE | DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);

  reader->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, 1, 0, 0, 0, 0, true)));
}

TEST(dds_DCPS_InternalDataReader, read_instance)
{
  Sample sample1("a");
  Sample sample2("b");
  Sample sample3("c");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample1);
  reader->write(writer, sample2);
  reader->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, sample1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample1);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, sample3, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);
}

TEST(dds_DCPS_InternalDataReader, take_instance)
{
  Sample sample1("a");
  Sample sample2("b");
  Sample sample3("c");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer, sample1);
  reader->write(writer, sample2);
  reader->take_instance(samples, infos, DDS::LENGTH_UNLIMITED, sample2, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample2);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));

  reader->take_instance(samples, infos, DDS::LENGTH_UNLIMITED, sample3, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 0U);
  ASSERT_EQ(infos.size(), 0U);
}

TEST(dds_DCPS_InternalDataReader, remove_publication_does_not_dispose_everything)
{
  Sample sample("key");
  ReaderType::SampleSequence samples;
  InternalSampleInfoSequence infos;

  RcHandle<InternalEntity> writer1 = make_rch<InternalEntity>();
  RcHandle<InternalEntity> writer2 = make_rch<InternalEntity>();
  RcHandle<ReaderType> reader = make_rch<ReaderType>(DataReaderQosBuilder().reliability_reliable());

  reader->write(writer1, sample);
  reader->remove_publication(writer2, true);
  reader->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  ASSERT_EQ(samples.size(), 1U);
  ASSERT_EQ(infos.size(), 1U);

  EXPECT_EQ(samples[0], sample);
  EXPECT_EQ(SIW(infos[0]), SIW(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE, 0, 0, 0, 0, 0, true)));
}
