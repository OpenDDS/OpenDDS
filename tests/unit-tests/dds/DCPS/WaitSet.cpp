/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/GuardCondition.h>
#include "key_annotationTypeSupportImpl.h"

#ifdef ACE_HAS_CPP11
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#endif

using namespace OpenDDS;
using namespace OpenDDS::DCPS;

TEST(dds_DCPS_WaitSet, DefaultConstructor)
{
  DDS::WaitSet ws;
  DDS::ConditionSeq conditions;
  EXPECT_EQ(ws.get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), 0u);
}

TEST(dds_DCPS_WaitSet, VarDuplicate)
{
  DDS::WaitSet_var ws2;
  {
    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), 1u);

    ws2 = ws;
  }
  DDS::ConditionSeq conditions;
  EXPECT_EQ(ws2->get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), 1u);
}

TEST(dds_DCPS_WaitSet, AttachDetachFailures)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);
  // Succeeds because it was already in set (despite internal insert "failure")
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  {
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), 1u);
  }

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
  // Real failure, because it can't find the condition in the waitset
  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_PRECONDITION_NOT_MET);

  {
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), 0u);
  }
}

TEST(dds_DCPS_WaitSet, AttachDetach)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  const CORBA::ULong test_len = 3;

  for (CORBA::ULong i = 0; i < test_len; ++i) {
    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), i + 1);
  }

  DDS::ConditionSeq conditions;
  EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), test_len);

  for (CORBA::ULong i = 0; i < conditions.length(); ++i) {
    EXPECT_EQ(ws->detach_condition(conditions[i]), DDS::RETCODE_OK);
  }

  EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), 0u);
}

TEST(dds_DCPS_WaitSet, AttachDetachAll)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  const CORBA::ULong test_len = 3;

  for (CORBA::ULong i = 0; i < test_len; ++i) {
    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), i + 1);
  }

  DDS::ConditionSeq conditions;
  EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), test_len);

  EXPECT_EQ(ws->detach_conditions(conditions), DDS::RETCODE_OK);

  EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
  EXPECT_EQ(conditions.length(), 0u);
}

TEST(dds_DCPS_WaitSet, AttachNoDetach)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  const CORBA::ULong test_len = 3;

  for (CORBA::ULong i = 0; i < test_len; ++i) {
    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);
    DDS::ConditionSeq conditions;
    EXPECT_EQ(ws->get_conditions(conditions), DDS::RETCODE_OK);
    EXPECT_EQ(conditions.length(), i + 1);
  }
}

TEST(dds_DCPS_WaitSet, WaitBogusDeadline)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  const ::DDS::Duration_t negative_three_seconds = { -3, 0 };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, negative_three_seconds), DDS::RETCODE_BAD_PARAMETER);
  }

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}

TEST(dds_DCPS_WaitSet, WaitDeadlineTimeout)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  const ::DDS::Duration_t tiny = { 0, 100 };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, tiny), DDS::RETCODE_TIMEOUT);
  }

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}

TEST(dds_DCPS_WaitSet, WaitDeadlineTriggered)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  gc->set_trigger_value(true);

  const ::DDS::Duration_t tiny = { 0, 100 };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, tiny), DDS::RETCODE_OK);
    EXPECT_EQ(active.length(), 1u);
    EXPECT_EQ(active[0], gc);
  }

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}

TEST(dds_DCPS_WaitSet, WaitLivelinessLost)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory->get_instance();
  OpenDDS::RTPS::RtpsDiscovery_rch disc =
      OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
  TheServiceParticipant->add_discovery(disc);
  TheServiceParticipant->set_default_discovery(disc->key());
  TheServiceParticipant->set_repo_domain(42, disc->key());

  DDS::DomainParticipant_var dp =
      dpf->create_participant(42, PARTICIPANT_QOS_DEFAULT, 0, 0);
  EXPECT_TRUE(!CORBA::is_nil(dp));

  key_annotation::UnkeyedStructTypeSupport_var ts = new key_annotation::UnkeyedStructTypeSupportImpl;
  CORBA::String_var type_name = ts->get_type_name();
  EXPECT_EQ(ts->register_type(dp, type_name), DDS::RETCODE_OK);

  DDS::Topic_var topic =
      dp->create_topic("FooTopic", type_name, TOPIC_QOS_DEFAULT, 0, 0);
  EXPECT_TRUE(!CORBA::is_nil(topic));

  DDS::Publisher_var publisher = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
  EXPECT_TRUE(!CORBA::is_nil(publisher));

  DDS::DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
  dw_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  dw_qos.liveliness.lease_duration.sec = 1;
  dw_qos.liveliness.lease_duration.nanosec = 0;
  DDS::DataWriter_var writer = publisher->create_datawriter(topic, dw_qos, 0, 0);
  EXPECT_TRUE(!CORBA::is_nil(writer));
  EXPECT_EQ(writer->assert_liveliness(), DDS::RETCODE_OK);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  DDS::StatusCondition_var sc = writer->get_statuscondition();
  sc->set_enabled_statuses(DDS::LIVELINESS_LOST_STATUS);
  EXPECT_EQ(ws->attach_condition(sc), DDS::RETCODE_OK);

  const ::DDS::Duration_t two_seconds = {2, 0 };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, two_seconds), DDS::RETCODE_OK);
  }

  EXPECT_EQ(publisher->delete_datawriter(writer), DDS::RETCODE_OK);
  EXPECT_EQ(dp->delete_publisher(publisher), DDS::RETCODE_OK);
  EXPECT_EQ(dp->delete_topic(topic), DDS::RETCODE_OK);
  EXPECT_EQ(dp->delete_contained_entities(), DDS::RETCODE_OK);
  EXPECT_EQ(dpf->delete_participant(dp), DDS::RETCODE_OK);
  EXPECT_EQ(ws->detach_condition(sc), DDS::RETCODE_OK);
}

#ifdef ACE_HAS_CPP11
TEST(dds_DCPS_WaitSet, WaitForever)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  std::thread thread([&](){
    gc->set_trigger_value(true);
  });

  const ::DDS::Duration_t forever = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, forever), DDS::RETCODE_OK);
    EXPECT_EQ(active.length(), 1u);
    EXPECT_EQ(active[0], gc);
  }

  thread.join();

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}

TEST(dds_DCPS_WaitSet, WaitDeadline)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  std::thread thread([&](){
    gc->set_trigger_value(true);
  });

  const ::DDS::Duration_t three_seconds = { 3, 0 };
  {
    DDS::ConditionSeq active;
    EXPECT_EQ(ws->wait(active, three_seconds), DDS::RETCODE_OK);
    EXPECT_EQ(active.length(), 1u);
    EXPECT_EQ(active[0], gc);
  }

  thread.join();

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}

TEST(dds_DCPS_WaitSet, WaitMulti)
{
  DDS::WaitSet_var ws = new DDS::WaitSet;

  DDS::GuardCondition_var gc = new DDS::GuardCondition;
  EXPECT_EQ(ws->attach_condition(gc), DDS::RETCODE_OK);

  const ::DDS::Duration_t forever = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

  // Number of threads / concurrent waits
  const CORBA::ULong test_len = 3;

  // Tools to coordinate thread status
  CORBA::ULong threads_complete = 0;
  std::mutex threads_complete_mutex;
  std::condition_variable threads_complete_cv;

  // Create threads
  std::vector<DDS::ReturnCode_t> wait_results(test_len, DDS::RETCODE_ERROR);
  std::vector<std::thread> threads;
  for (CORBA::ULong i = 0; i < test_len; ++i) {
    threads.push_back(
      std::thread([&, i](){
        DDS::WaitSet_var my_ws = ws;
        DDS::ConditionSeq active;
        wait_results[i] = my_ws->wait(active, forever);
        if (wait_results[i] == DDS::RETCODE_OK) {
          EXPECT_EQ(active.length(), 1u);
          EXPECT_EQ(active[0], gc);
        }
        std::unique_lock<std::mutex> lock(threads_complete_mutex);
        ++threads_complete;
        threads_complete_cv.notify_all();
      })
    );
  }

  // Wait until test_len - 1 threads fail and exit
  {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    std::unique_lock<std::mutex> lock(threads_complete_mutex);
    std::cv_status cvs = std::cv_status::no_timeout;
    while (threads_complete < (test_len - 1) && cvs != std::cv_status::timeout) {
      cvs = threads_complete_cv.wait_until(lock, deadline);
    }
    EXPECT_EQ(cvs, std::cv_status::no_timeout);
  }

  // Now trigger so the last thread can exit
  gc->set_trigger_value(true);

  // Check to make sure we only had one successful wait
  CORBA::ULong total_ok = 0;
  for (CORBA::ULong i = 0; i < test_len; ++i) {
    threads[i].join();
    if (wait_results[i] == DDS::RETCODE_OK) {
      ++total_ok;
    }
  }
  EXPECT_EQ(total_ok, 1u);

  EXPECT_EQ(ws->detach_condition(gc), DDS::RETCODE_OK);
}
#endif

