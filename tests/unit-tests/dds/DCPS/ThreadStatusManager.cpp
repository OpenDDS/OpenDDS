#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/StaticIncludes.h>

#include <ace/Task.h>

#include <gtest/gtest.h>

#ifndef DDS_HAS_MINIMUM_BIT

using namespace OpenDDS::DCPS;

const int EVENT1_DETAIL1 = 1;
const int EVENT1_DETAIL2 = 2;
const int EVENT2_DETAIL1 = 3;
const int EVENT2_DETAIL2 = 4;

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

    // For each event, wait for the main thread to signal to proceed
    ThreadStatusManager::Start start(tsm, id_);
    EXPECT_EQ(cv_.wait(), 0);

    {
      ThreadStatusManager::Event event1(tsm, EVENT1_DETAIL1, EVENT1_DETAIL2);
      EXPECT_EQ(cv_.wait(), 0);

      {
        ThreadStatusManager::Event event2(tsm, EVENT2_DETAIL1, EVENT2_DETAIL2);
        EXPECT_EQ(cv_.wait(), 0);
      }
      // Wait for the thread status correponding to the end of event2 to be read
      EXPECT_EQ(cv_.wait(), 0);
    }
    // Similar for the end of event1
    EXPECT_EQ(cv_.wait(), 0);

    return 0;
  }

private:
  ACE_Condition<ACE_Thread_Mutex>& cv_;
  String id_;
};

bool read_status(DDS::WaitSet_var ws, InternalThreadBuiltinTopicDataDataReader_var dr, MonotonicTimePoint& timestamp,
                 const TestThread& task, bool thread_end = false, int expected_detail1 = 0, int expected_detail2 = 0)
{
  const DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq active;
  bool found = false;

  while (!found) {
    DDS::ReturnCode_t rc = ws->wait(active, infinite);
    EXPECT_EQ(rc, DDS::RETCODE_OK);
    if (rc != DDS::RETCODE_OK) {
      break;
    }

    DDS::InstanceHandle_t prev_handle = DDS::HANDLE_NIL;
    while (!found && rc == DDS::RETCODE_OK) {
      InternalThreadBuiltinTopicDataSeq datas;
      DDS::SampleInfoSeq infos;
      rc = dr->read_next_instance(datas, infos, DDS::LENGTH_UNLIMITED, prev_handle,
        DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      if (rc == DDS::RETCODE_OK) {
        EXPECT_EQ(datas.length(), 1u);
        prev_handle = infos[0].instance_handle;
        const String thread_id = datas[0].thread_id.in();
        if (thread_id.find(task.id()) != String::npos) {
          found = true;
          if (!thread_end) {
            timestamp = MonotonicTimePoint(datas[0].monotonic_timestamp);
            EXPECT_EQ(datas[0].detail1, expected_detail1);
            EXPECT_EQ(datas[0].detail2, expected_detail2);
            EXPECT_TRUE(infos[0].valid_data);
          } else {
            EXPECT_FALSE(infos[0].valid_data);
            EXPECT_EQ(infos[0].instance_state, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
          }
        }
      }
    }
  }
  return found;
}

TEST(dds_DCPS_ThreadStatusManager, Lifecycle)
{
  // Create a participant that harvests thread status every one second
  ACE_TCHAR* argv[] = { const_cast<ACE_TCHAR*>(ACE_TEXT("-DCPSThreadStatusInterval")),
                        const_cast<ACE_TCHAR*>(ACE_TEXT("1")),
                        const_cast<ACE_TCHAR*>(ACE_TEXT("-DCPSDefaultDiscovery")),
                        const_cast<ACE_TCHAR*>(ACE_TEXT("DEFAULT_RTPS")) };
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

  DDS::ReadCondition_var read_cond = reader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(read_cond);

  MonotonicTimePoint thread_start, event1_start, event1_end, event2_start, event2_end, thread_end;

  // Notify the test thread to proceed after reading the thread status in each step
  EXPECT_TRUE(read_status(ws, itbtd_reader, thread_start, task));
  cv.broadcast();

  EXPECT_TRUE(read_status(ws, itbtd_reader, event1_start, task, false, EVENT1_DETAIL1, EVENT1_DETAIL2));
  cv.broadcast();

  EXPECT_TRUE(read_status(ws, itbtd_reader, event2_start, task, false, EVENT2_DETAIL1, EVENT2_DETAIL2));
  cv.broadcast();

  EXPECT_TRUE(read_status(ws, itbtd_reader, event2_end, task));
  cv.broadcast();

  EXPECT_TRUE(read_status(ws, itbtd_reader, event1_end, task));
  cv.broadcast();

  // The thread status sample corresponding to the thread end event is a disposed sample,
  // so thread_end is just passed here, but its value is not used for anything.
  EXPECT_TRUE(read_status(ws, itbtd_reader, thread_end, task, true));

  // Check that the timestamps are in the correct order
  EXPECT_LT(thread_start, event1_start);
  EXPECT_LT(event1_start, event2_start);
  EXPECT_LT(event2_start, event2_end);
  EXPECT_LT(event2_end, event1_end);

  // Cleanup
  ws->detach_condition(read_cond);
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();
}

#endif
