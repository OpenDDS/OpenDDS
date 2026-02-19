#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Qos_Helper.h>

#include <ace/Task.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

class TestThread : public ACE_Task_Base {
public:
  TestThread(ACE_Condition<ACE_Thread_Mutex>& cv)
    : cv_(cv)
    , id_("dds_DCPS_ThreadStatusManager_TestThread")
  {}

  int start()
  {
    return activate(THR_NEW_LWP | THR_JOINABLE, 1);
  }

  String id() const { return id_; }

  int svc()
  {
    ThreadStatusManager& tsm = TheServiceParticipant->get_thread_status_manager();
    ThreadStatusManager::Start s(tsm, id_);
    EXPECT_EQ(cv_.wait(), 0);

    {
      ThreadStatusManager::Event event1(tsm, 1, 2);
    }

    return 0;
  }

private:
  ACE_Condition<ACE_Thread_Mutex>& cv_;
  String id_;
};

TEST(dds_DCPS_ThreadStatusManager, Lifecycle)
{
  // Harvesting is done every one second.
  ACE_TCHAR* argv[] = { const_cast<ACE_TCHAR*>(ACE_TEXT("-DCPSThreadStatusInterval")), const_cast<ACE_TCHAR*>(ACE_TEXT("1")) };
  int argc = sizeof(argv) / sizeof(argv[0]);
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipantQos participant_qos;
  dpf->get_default_participant_qos(participant_qos);
  DDS::PropertySeq& properties = participant_qos.property.value;
  Qos_Helper::append(properties, OpenDDS::RTPS::RTPS_HARVEST_THREAD_STATUS, "true");
  const DDS::DomainId_t domain = 0;
  DDS::DomainParticipant_var participant = dpf->create_participant(domain, participant_qos, 0, DEFAULT_STATUS_MASK);
  ASSERT_TRUE(participant.in());

  ACE_Thread_Mutex mutex;
  ACE_Condition<ACE_Thread_Mutex> cv(mutex);
  TestThread task(cv);
  EXPECT_EQ(task.start(), 0);

  DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
  DDS::DataReader_var reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  InternalThreadBuiltinTopicDataDataReader_var itbtd_reader = InternalThreadBuiltinTopicDataDataReader::_narrow(reader);
  ASSERT_TRUE(reader.in());

  // Wait and read the test thread start event
  DDS::ReadCondition_var read_cond = reader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(read_cond);
  const DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq active;
  DDS::ReturnCode_t rc = ws->wait(active, infinite);
  EXPECT_EQ(rc, DDS::RETCODE_OK);

  InternalThreadBuiltinTopicDataSeq datas;
  DDS::SampleInfoSeq infos;
  rc = itbtd_reader->read(datas, infos, DDS::LENGTH_UNLIMITED, DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  EXPECT_EQ(rc, DDS::RETCODE_OK);
  EXPECT_EQ(datas.length(), 1u);
  const String thread_id = datas[0].thread_id.in();
  EXPECT_NE(thread_id.find(task.id()), String::npos);
  const MonotonicTimePoint start(datas[0].monotonic_timestamp);

  // Notify the test thread to proceed
  cv.broadcast();


}
