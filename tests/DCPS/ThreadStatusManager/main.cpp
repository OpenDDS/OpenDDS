#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/InstanceState.h>

#include <ace/Task.h>

using namespace OpenDDS::DCPS;

const int EVENT1_DETAIL1 = 1;
const int EVENT1_DETAIL2 = 2;
const int EVENT2_DETAIL1 = 3;
const int EVENT2_DETAIL2 = 4;

namespace {
  const ACE_Time_Value config_poll_interval(0, 100000);
  const DDS::Duration_t config_wait_time = {10, 0};
}

class TestThread : public ACE_Task_Base {
public:

  enum TestThreadState {
    TTS_NONE = 0,
    TTS_START = 1,
    TTS_E1_BEGIN = 2,
    TTS_E2_BEGIN = 3,
    TTS_E2_END = 4,
    TTS_E1_END = 5,
    TTS_STOP = 6
  };

  TestThread()
    : mutex_()
    , cv_(mutex_)
    , state_(TTS_NONE)
    , id_("dds_DCPS_ThreadStatusManager_TestThread")
  {}

  int start()
  {
    return activate(THR_NEW_LWP | THR_JOINABLE, 1);
  }

  String id() const { return id_; }

  void wait_for_state(TestThreadState state) const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    while (state != state_) {
      if (cv_.wait() != 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) TestThread::wait_for_state: failed waiting for state %d on cv_\n", state));
        throw false;
      }
    }
  }

  void set_state(TestThreadState state)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    // Currently, state progression progresses precisely by one each time
    if (state != state_ + 1) {
      ACE_ERROR((LM_ERROR, "(%P|%t) TestThread::set_state: Unexpected state '%d' when current state is '%d'\n", state, state_));
      throw false;
    }
    const bool notify = state != state_;
    state_ = state;
    if (notify) {
      cv_.broadcast();
    }
  }

private:
  int svc()
  {
    ThreadStatusManager& tsm = TheServiceParticipant->get_thread_status_manager();

    // For each event, wait for the main thread to signal to proceed
    ThreadStatusManager::Start start(tsm, id_);

    wait_for_state(TTS_START);

    {
      ThreadStatusManager::Event event1(tsm, EVENT1_DETAIL1, EVENT1_DETAIL2);

      wait_for_state(TTS_E1_BEGIN);

      {
        ThreadStatusManager::Event event2(tsm, EVENT2_DETAIL1, EVENT2_DETAIL2);

        wait_for_state(TTS_E2_BEGIN);
      }

      wait_for_state(TTS_E2_END);
    }

    wait_for_state(TTS_E1_END);

    return 0;
  }

  mutable ACE_Thread_Mutex mutex_;
  mutable ACE_Condition<ACE_Thread_Mutex> cv_;
  TestThreadState state_;
  String id_;
};

bool wait_for_thread_status_interval(const TimeDuration& expected)
{
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + TimeDuration(config_wait_time);
  while (MonotonicTimePoint::now() < deadline) {
    if (TheServiceParticipant->get_thread_status_manager().thread_status_interval() == expected) {
      return true;
    }
    ACE_OS::sleep(config_poll_interval);
  }
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: wait_for_thread_status_interval - timed out waiting for interval to become {%d sec, %u nsec}\n",
    expected.value().sec(), expected.value().usec() * 1000u));
  return false;
}

bool read_status(DDS::WaitSet_var ws, InternalThreadBuiltinTopicDataDataReader_var dr, MonotonicTimePoint& timestamp,
                 TestThread& task, TestThread::TestThreadState new_state, const char* status,
                 bool thread_end = false, int expected_detail1 = 0, int expected_detail2 = 0)
{
  const DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq active;
  bool found = false;

  while (!found) {
    DDS::ReturnCode_t rc = ws->wait(active, infinite);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: read_status - failed to wait\n"));
      break;
    }

    DDS::InstanceHandle_t prev_handle = DDS::HANDLE_NIL;
    while (!found && rc == DDS::RETCODE_OK) {
      InternalThreadBuiltinTopicDataSeq datas;
      DDS::SampleInfoSeq infos;
      rc = dr->read_next_instance(datas, infos, DDS::LENGTH_UNLIMITED, prev_handle,
        DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      if (rc == DDS::RETCODE_OK) {
        if (datas.length() != 1u) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expect 1 sample, received %u for instance key \"%C\"\n",
            datas.length(), datas[0].thread_id.in()));
        }
        prev_handle = infos[0].instance_handle;
        const String thread_id = datas[0].thread_id.in();
        if (thread_id.find(task.id()) != String::npos) {
          found = true;
          if (!thread_end) {
            timestamp = MonotonicTimePoint(datas[0].monotonic_timestamp);
            if (!infos[0].valid_data) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expected valid data but received invalid\n"));
            }
            if (datas[0].detail1 != expected_detail1) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expected detail1 (%d) != received detail1 (%d)\n",
                expected_detail1, datas[0].detail1));
            }
            if (datas[0].detail2 != expected_detail2) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expected detail2 (%d) != received detail2 (%d)\n",
                expected_detail2, datas[0].detail2));
            }
          } else {
            if (infos[0].valid_data) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expected invalid data but received valid\n"));
            }
            if (infos[0].instance_state != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: read_status - expect instance_state NOT_ALIVE_DISPOSED, received %C\n",
                InstanceState::instance_state_string(infos[0].instance_state)));
            }
          }
        }
      }
    }
  }

  if (!found) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: read_status - failed to read test thread's %C status\n", status));
  } else {
    task.set_state(new_state);
  }
  return found;
}

int tsm_test(InternalThreadBuiltinTopicDataDataReader_var itbtd_reader, DDS::WaitSet_var ws)
{
  TestThread task;

  if (task.start() != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main - failed to start test thread\n"));
    return EXIT_FAILURE;
  }

  MonotonicTimePoint thread_start, event1_start, event1_end, event2_start, event2_end, thread_end;

  // Notify the test thread to proceed after reading the thread status in each step
  if (!read_status(ws, itbtd_reader, thread_start, task, TestThread::TTS_START, "start")) {
    return EXIT_FAILURE;
  }

  if (!read_status(ws, itbtd_reader, event1_start, task, TestThread::TTS_E1_BEGIN, "event1 begin", false, EVENT1_DETAIL1, EVENT1_DETAIL2)) {
    return EXIT_FAILURE;
  }

  if (!read_status(ws, itbtd_reader, event2_start, task, TestThread::TTS_E2_BEGIN, "event2 begin", false, EVENT2_DETAIL1, EVENT2_DETAIL2)) {
    return EXIT_FAILURE;
  }

  if (!read_status(ws, itbtd_reader, event2_end, task, TestThread::TTS_E2_END, "event2 end")) {
    return EXIT_FAILURE;
  }

  if (!read_status(ws, itbtd_reader, event1_end, task, TestThread::TTS_E1_END, "event1 end")) {
    return EXIT_FAILURE;
  }

  // The thread status sample corresponding to the thread end event is a disposed sample,
  // so thread_end is just passed here, but its value is not used for anything.
  if (!read_status(ws, itbtd_reader, thread_end, task, TestThread::TTS_STOP, "stop", true)) {
    return EXIT_FAILURE;
  }

  task.wait();

  // Check that the timestamps are in the correct order
  int ret = EXIT_SUCCESS;
  if (!(thread_start < event1_start)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) ERROR: main - thread_start ({%d sec, %u nsec}) should be before event1_start ({%d sec, %u nsec})\n",
      thread_start.to_idl_struct().sec, thread_start.to_idl_struct().nanosec,
      event1_start.to_idl_struct().sec, event1_start.to_idl_struct().nanosec));
    ret = EXIT_FAILURE;
  }
  if (!(event1_start < event2_start)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) ERROR: main - event1_start ({%d sec, %u nsec}) should be before event2_start ({%d sec, %u nsec})\n",
      event1_start.to_idl_struct().sec, event1_start.to_idl_struct().nanosec,
      event2_start.to_idl_struct().sec, event2_start.to_idl_struct().nanosec));
    ret = EXIT_FAILURE;
  }
  if (!(event2_start < event2_end)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) ERROR: main - event2_start ({%d sec, %u nsec}) should be before event2_end ({%d sec, %u nsec})\n",
      event2_start.to_idl_struct().sec, event2_start.to_idl_struct().nanosec,
      event2_end.to_idl_struct().sec, event2_end.to_idl_struct().nanosec));
    ret = EXIT_FAILURE;
  }
  if (!(event2_end < event1_end)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) ERROR: main - event2_end ({%d sec, %u nsec}) should be before event1_end ({%d sec, %u nsec})\n",
      event2_end.to_idl_struct().sec, event2_end.to_idl_struct().nanosec,
      event1_end.to_idl_struct().sec, event1_end.to_idl_struct().nanosec));
    ret = EXIT_FAILURE;
  }

  return ret;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipantQos participant_qos;
  dpf->get_default_participant_qos(participant_qos);
  DDS::PropertySeq& properties = participant_qos.property.value;
  Qos_Helper::append(properties, OpenDDS::RTPS::RTPS_HARVEST_THREAD_STATUS, "true");
  const DDS::DomainId_t domain = 0;
  DDS::DomainParticipant_var participant = dpf->create_participant(domain, participant_qos, 0, DEFAULT_STATUS_MASK);
  if (!participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main - failed to create participant\n"));
    return EXIT_FAILURE;
  }

  DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
  DDS::DataReader_var reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  InternalThreadBuiltinTopicDataDataReader_var itbtd_reader = InternalThreadBuiltinTopicDataDataReader::_narrow(reader);
  if (!itbtd_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main - failed to narrow internal thread builtin topic data reader\n"));
    return EXIT_FAILURE;
  }

  DDS::ReadCondition_var read_cond = reader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(read_cond);

  // Run test with initial config
  const DDS::UInt32 initial_interval = TheServiceParticipant->config_store()->get_uint32(
    OpenDDS::DCPS::COMMON_DCPS_THREAD_STATUS_INTERVAL, 0);
  if (!wait_for_thread_status_interval(TimeDuration(initial_interval))) {
    return EXIT_FAILURE;
  }
  int ret = tsm_test(itbtd_reader, ws);

  if (ret == EXIT_SUCCESS) {
    const char* const status_prop = OpenDDS::DCPS::COMMON_DCPS_THREAD_STATUS_INTERVAL;
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::ConfigStoreImpl> config_store =
      TheServiceParticipant->config_store();

    // Turn off thread status reporting and expect no messages
    const DDS::UInt32 orig = config_store->get_uint32(status_prop, 0);
    config_store->set_uint32(status_prop, 0);
    // Unfortunatly, we don't have a "clean" way to wait for the config change to propagate all the
    // way into the SPDP timer's cancelation, only into the Service_Partipant config store,
    // so the safest thing to do here is just wait until we know for sure the timer is definitely done
    ACE_OS::sleep(orig * 2);
    const DDS::Duration_t waittime = {5, 0};
    DDS::ConditionSeq active;
    DDS::ReturnCode_t rc = ws->wait(active, waittime);
    if (rc != DDS::RETCODE_TIMEOUT) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: got unexpected thread status message\n"));
      ret = EXIT_FAILURE;
    }

    // Turn it back on do the testing again
    if (ret == EXIT_SUCCESS) {
      config_store->set_uint32(status_prop, orig);
      if (!wait_for_thread_status_interval(TimeDuration(orig))) {
        ret = EXIT_FAILURE;
      } else {
        ret = tsm_test(itbtd_reader, ws);
      }
    }
  }

  // Cleanup
  ws->detach_condition(read_cond);
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return ret;
}
