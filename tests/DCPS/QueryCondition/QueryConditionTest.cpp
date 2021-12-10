#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "MessengerTypeSupportImpl.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include <cstdlib>
#include <iostream>
using namespace std;
using namespace DDS;
using namespace OpenDDS::DCPS;
using namespace Messenger;

const Duration_t max_wait_time = {3, 0};

class MessengerListener
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  MessengerListener(DDS::ReadCondition_ptr rd)
    : rc_(DDS::ReadCondition::_duplicate(rd))
    , enabled_(true)
  {}

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedDeadlineMissedStatus & /*status*/) {}

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedIncompatibleQosStatus & /*status*/) {}

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::LivelinessChangedStatus & /*status*/) {}

  virtual void on_subscription_matched(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SubscriptionMatchedStatus & /*status*/) {}

  virtual void on_sample_rejected(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SampleRejectedStatus& /*status*/) {}

  virtual void on_data_available(DDS::DataReader_ptr reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    if (!enabled_) {
      return;
    }

    MessageDataReader_var mdr = MessageDataReader::_narrow(reader);
    MessageSeq data;
    SampleInfoSeq infoseq;
    ReturnCode_t rc = mdr->read_w_condition(data, infoseq, LENGTH_UNLIMITED, rc_);
    if (rc != RETCODE_OK && rc != RETCODE_NO_DATA) {
      cerr << "ERROR: read_w_condition failed: " << retcode_to_string(rc) << endl;
    }
  }

  virtual void on_sample_lost(DDS::DataReader_ptr /*reader*/,
                              const DDS::SampleLostStatus& /*status*/) {}

  void disable()
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    enabled_ = false;
  }

private:
  DDS::ReadCondition_var rc_;
  bool enabled_;
  mutable ACE_Thread_Mutex mutex_;
};

bool test_setup(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub, const char* topicName, DataWriter_var& dw,
  DataReader_var& dr)
{
  CORBA::String_var typeName = ts->get_type_name();
  Topic_var topic = dp->create_topic(topicName, typeName,
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);
  if (!topic) {
    cerr << "ERROR: test_setup: create_topic failed" << endl;
    return false;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dw = pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);
  if (!dw) {
    cerr << "ERROR: test_setup: create_datawriter failed" << endl;
    return false;
  }

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dr = sub->create_datareader(topic, dr_qos, 0, DEFAULT_STATUS_MASK);
  if (!dr) {
    cerr << "ERROR: test_setup: create_datareader failed" << endl;
    return false;
  }

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  if (ws->wait(active, infinite) != DDS::RETCODE_OK) {
    return false;
  }
  ws->detach_condition(dw_sc);
  return true;
}

bool complex_test_setup(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub, const char* topicName, DataWriter_var& dw,
  DataReader_var& dr1, DataReader_var& dr2)
{
  CORBA::String_var typeName = ts->get_type_name();
  Topic_var topic = dp->create_topic(topicName, typeName,
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);
  if (!topic) {
    cerr << "ERROR: complex_test_setup: create_topic failed" << endl;
    return false;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  dw = pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);
  if (!dw) {
    cerr << "ERROR: complex_test_setup: create_datawriter failed" << endl;
    return false;
  }

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dr1 = sub->create_datareader(topic, dr_qos, 0, DEFAULT_STATUS_MASK);
  if (!dr1) {
    cerr << "ERROR: complex_test_setup: 1st create_datareader failed" << endl;
    return false;
  }
  dr2 = sub->create_datareader(topic, dr_qos, 0, DEFAULT_STATUS_MASK);
  if (!dr2) {
    cerr << "ERROR: complex_test_setup: 2nd create_datareader failed" << endl;
    return false;
  }

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  PublicationMatchedStatus status;
  while (true) {
    if (dw->get_publication_matched_status(status) != DDS::RETCODE_OK) {
      return false;
    }
    if (status.current_count >= 2) {
      break;
    }
    if (ws->wait(active, infinite) != DDS::RETCODE_OK) {
      return false;
    }
  }
  ws->detach_condition(dw_sc);
  return true;
}

bool test_cleanup(
  const DomainParticipant_var& dp,
  const Publisher_var& pub, const Subscriber_var& sub,
  DataWriter_var& dw, DataReader_var& dr,
  bool complex_test = false)
{
  Topic_var topic = dw->get_topic();
  ReturnCode_t r = sub->delete_datareader(dr);
  if (r != DDS::RETCODE_OK) {
    cerr << "ERROR: " << (complex_test ? "complex_" : "") << "test_cleanup: "
      << "delete " << (complex_test ? "1st " : "")
      << "datareader failed: " << retcode_to_string(r) << endl;
    return false;
  }
  r = pub->delete_datawriter(dw);
  if (r != DDS::RETCODE_OK) {
    cerr << "ERROR: " << (complex_test ? "complex_" : "") << "test_cleanup: "
      << "delete datawriter failed: " << retcode_to_string(r) << endl;
    return false;
  }
  r = dp->delete_topic(topic);
  if (r != DDS::RETCODE_OK) {
    cerr << "ERROR: " << (complex_test ? "complex_" : "") << "test_cleanup: "
      << "delete topic failed: " << retcode_to_string(r) << endl;
    return false;
  }
  return true;
}

bool complex_test_cleanup(
  const DomainParticipant_var& dp,
  const Publisher_var& pub, const Subscriber_var& sub,
  DataWriter_var& dw, DataReader_var& dr1, DataReader_var& dr2)
{
  ReturnCode_t rc = sub->delete_datareader(dr2);
  if (rc != DDS::RETCODE_OK) {
    cerr << "ERROR: complex_test_cleanup: delete 2nd datareader failed: "
      << retcode_to_string(rc) << endl;
    return false;
  }
  return test_cleanup(dp, pub, sub, dw, dr1, true);
}

bool waitForSample(const DataReader_var& dr)
{
  ReadCondition_var dr_rc = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_rc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ReturnCode_t ret = ws->wait(active, infinite);
  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: wait(rc) failed" << endl;
    return false;
  }
  return true;
}

bool run_filtering_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  DataWriter_var dw;
  DataReader_var dr;
  if (!test_setup(dp, ts, pub, sub, "MyTopic2", dw, dr)) {
    cerr << "ERROR: run_filtering_test: setup failed" << endl;
    return false;
  }

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 1;
  sample.nest.value = B;
  ReturnCode_t ret = mdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_filtering_test: write failed: " << retcode_to_string(ret) << endl;
    return false;
  }
  if (!waitForSample(dr)) return false;

  ReadCondition_var dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "key > 1", DDS::StringSeq());
  if (!dr_qc) {
    cerr << "ERROR: failed to create QueryCondition" << endl;
    return false;
  }
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_qc);
  ConditionSeq active;
  ret = ws->wait(active, max_wait_time);
  // expect a timeout because the sample doesn't match the query string
  if (ret != RETCODE_TIMEOUT) {
    cerr << "ERROR: wait(qc) should have timed out" << endl;
    return false;
  }
  ws->detach_condition(dr_qc);

  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  MessageSeq data;
  SampleInfoSeq infoseq;
  ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_NO_DATA) {
    cerr << "ERROR: expected no data, but take_w_condition(qc) returned: "
      << retcode_to_string(ret) << endl;
    return false;
  }

  SampleInfo info;
  if (mdr->take_next_sample(sample, info) != RETCODE_OK) {
    cerr << "ERROR: take_next_sample() failed: " << retcode_to_string(ret) << endl;
    return false;
  }

  sample.key = 2;
  ret = mdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: take_next_sample() failed: " << retcode_to_string(ret) << endl;
    return false;
  }
  if (!waitForSample(dr)) return false;

  ws->attach_condition(dr_qc);
  ret = ws->wait(active, max_wait_time);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: wait(qc) should not time out" << endl;
    return false;
  }
  ws->detach_condition(dr_qc);

  ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: take_w_condition(qc) failed: " << retcode_to_string(ret) << endl;
    return false;
  }

  ret = dr->delete_readcondition(dr_qc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: delete dr_qc failed: " << retcode_to_string(ret) << endl;
    return false;
  }
  if (!test_cleanup(dp, pub, sub, dw, dr)) {
    cerr << "ERROR: run_filtering_test: cleanup failed" << endl;
    return false;
  }
  return true;
}

bool run_complex_filtering_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  DataWriter_var dw;
  DataReader_var dr1;
  DataReader_var dr2;
  if (!complex_test_setup(dp, ts, pub, sub, "MyTopicComplex", dw, dr1, dr2)) {
    cerr << "ERROR: run_complex_filtering_test: setup failed" << endl;
    return false;
  }

  DDS::StringSeq params(2);
  params.length (2);
  params[0] = "5";
  params[1] = "10";
  QueryCondition_var dr_qc1 = dr1->create_querycondition(NOT_READ_SAMPLE_STATE,
    NEW_VIEW_STATE | NOT_NEW_VIEW_STATE, ANY_INSTANCE_STATE, "( (iteration > %0) AND (iteration < %1) )", params);
  if (!dr_qc1) {
    cerr << "ERROR: failed to create QueryCondition 1" << endl;
    return false;
  }
  QueryCondition_var dr_qc2 = dr2->create_querycondition(NOT_READ_SAMPLE_STATE,
    NEW_VIEW_STATE | NOT_NEW_VIEW_STATE, ANY_INSTANCE_STATE, "( (iteration < %0) OR (iteration > %1) )", params);
  if (!dr_qc2) {
    cerr << "ERROR: failed to create QueryCondition 2" << endl;
    return false;
  }

  MessengerListener* ml1p = new MessengerListener(dr_qc1);
  DDS::DataReaderListener_var ml1 = ml1p;
  MessengerListener* ml2p = new MessengerListener(dr_qc2);
  DDS::DataReaderListener_var ml2 = ml2p;

  MessageDataReader_var mdr1 = MessageDataReader::_narrow(dr1);
  MessageDataReader_var mdr2 = MessageDataReader::_narrow(dr2);
  mdr1->set_listener(ml1, DDS::DATA_AVAILABLE_STATUS);
  mdr2->set_listener(ml2, DDS::DATA_AVAILABLE_STATUS);
  MessageSeq data;
  SampleInfoSeq infoseq;
  ReturnCode_t ret = mdr1->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc1);
  if (ret != RETCODE_NO_DATA) {
    cerr << "ERROR: take_w_condition(qc1): expected no data but got: " << retcode_to_string(ret) << endl;
    return false;
  }
  ret = mdr2->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc2);
  if (ret != RETCODE_NO_DATA) {
    cerr << "ERROR: take_w_condition(qc2): expected no data but got: " << retcode_to_string(ret) << endl;
    return false;
  }

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.nest.value = A;
  for (CORBA::Long i = 0; i < 6; i++) {
    sample.key = i;
    if (mdw->register_instance(sample) == HANDLE_NIL) {
      cerr << "ERROR: Registering instance failed" << endl;
      return false;
    }
  }

  for (CORBA::Long i = 0; i < 6; i++) {
    for (CORBA::Long j = 0; j < 15; j++) {
      sample.key = i;
      const DDS::InstanceHandle_t hnd = mdw->lookup_instance(sample);
      if (hnd == HANDLE_NIL) {
        cerr << "ERROR: Lookup instance failed" << endl;
        return false;
      }
      sample.iteration = j;
      ret = mdw->write(sample, hnd);
      if (ret != RETCODE_OK) {
        cerr << "ERROR: run_complex_filtering_test: write failed: "
          << retcode_to_string(ret) << endl;
        return false;
      }
    }
  }

  for (CORBA::Long i = 0; i < 6; i++) {
    sample.key = i;
    const DDS::InstanceHandle_t hnd = mdw->lookup_instance(sample);
    if (hnd == HANDLE_NIL) {
      cerr << "ERROR: Lookup instance failed" << endl;
      return false;
    }
    ret = mdw->dispose(sample, hnd);
    if (ret != RETCODE_OK) {
      cerr << "ERROR: Dispose instance " << i << " failed: " << retcode_to_string(ret) << endl;
      return false;
    }
    ret = mdw->unregister_instance(sample, hnd);
    if (ret != RETCODE_OK) {
      cerr << "ERROR: Unregistering instance " << i << " failed: "<< retcode_to_string(ret) << endl;
      return false;
    }
  }

  ml1p->disable();
  mdr1->set_listener(0, 0);
  ml2p->disable();
  mdr2->set_listener(0, 0);
  mdr1 = 0;
  mdr2 = 0;

  ret = dr1->delete_readcondition(dr_qc1);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: delete dr_qc1 failed: " << retcode_to_string(ret) << endl;
    return false;
  }
  ret = dr2->delete_readcondition(dr_qc2);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: delete dr_qc2 failed: " << retcode_to_string(ret) << endl;
    return false;
  }
  if (!complex_test_cleanup(dp, pub, sub, dw, dr1, dr2)) {
    cerr << "ERROR: run_complex_filtering_test: cleanup failed" << endl;
    return false;
  }
  return true;
}

bool run_sorting_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  DataWriter_var dw;
  DataReader_var dr;
  if (!test_setup(dp, ts, pub, sub, "MyTopic", dw, dr)) {
    cerr << "ERROR: run_sorting_test: setup failed" << endl;
    return false;
  }

  ReturnCode_t ret = RETCODE_OK;
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 0;
  sample.name = "data_X";
  sample.nest.value = B;
  for (int i(0); i < 20; ++i, ++sample.key) {
    //replace the 'X' with a random letter
    sample.name.inout()[5] = static_cast<char>((rand() % 26) + 'A');
    ret = mdw->write(sample, HANDLE_NIL);
    if (ret != RETCODE_OK) return false;
    if (!(i % 4)) { //once in a while write more than 1 sample per instance
      Message sample2(sample);
      sample2.nest.value = A;
      sample2.name.inout()[5] = static_cast<char>((rand() % 26) + 'A');
      ret = mdw->write(sample2, HANDLE_NIL);
      if (ret != RETCODE_OK) return false;
      sample2.nest.value = C;
      ret = mdw->write(sample2, HANDLE_NIL);
      if (ret != RETCODE_OK) return false;
    }
  }

  DDS::StringSeq empty_query_params;
  ReadCondition_var dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "ORDER BY name, nest.value",
    empty_query_params);
  if (!dr_qc) {
    cerr << "ERROR: failed to create QueryCondition" << endl;
    return false;
  }
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_qc);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  Duration_t five_seconds = {5, 0};
  bool passed = true, done = false;
  if (sub->delete_datareader(dr) != DDS::RETCODE_PRECONDITION_NOT_MET) {
    cerr << "ERROR: the deletion of a DataReader is not allowed if there are "
      "any existing QueryCondition objects that are attached to the DataReader." << endl;
    passed = false;
    done = true;
  }
  while (!done) {
    ConditionSeq active;
    ret = ws->wait(active, five_seconds);
    if (ret == RETCODE_TIMEOUT) {
      cout << "timeout ";
      done = true; //continue to try and read
    } else if (ret != RETCODE_OK) {
      passed = false;
      break;
    }
    cout << "wait returned" << endl;
    MessageSeq data;
    SampleInfoSeq info;
    ret = mdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_qc);
    if (ret == RETCODE_NO_DATA) {
      // fall-through
    } else if (ret != RETCODE_OK) {
      cerr << "ERROR: take_w_condition returned " << ret << endl;
      passed = false;
      done = true;
    } else {
      char largest = 0;
      for (CORBA::ULong i(0); i < data.length(); ++i)  {
        cout << "Info:\tinstance_handle = " << info[i].instance_handle <<
          "\tsample_rank = " << info[i].sample_rank << '\n';
        if (info[i].valid_data) {
          cout << "Data:\tkey = " << data[i].key <<
            " \tname = " << data[i].name <<
            "\tnest.value = " << data[i].nest.value <<
            '\n';
          if (data[i].name[5] >= largest) {
            largest = data[i].name[5];
          } else {
            cerr << "ERROR: data is not sorted for key: " <<
              data[i].key << endl;
            passed = false;
          }
        }
        cout << endl;
      }
    }
  }

  MessageSeq data;
  SampleInfoSeq info;
  ret = mdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_NO_DATA) {
    cerr << "WARNING: there is still data in the reader\n";
  }

  ws->detach_condition(dr_qc);
  dr->delete_readcondition(dr_qc);
  if (!test_cleanup(dp, pub, sub, dw, dr)) {
    cerr << "ERROR: run_sorting_test: cleanup failed" << endl;
    return false;
  }
  return passed;
}

bool run_change_parameter_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  DataWriter_var dw;
  DataReader_var dr;
  if (!test_setup(dp, ts, pub, sub, "MyTopic3", dw, dr)) {
    cerr << "ERROR: run_change_parameter_test: setup failed" << endl;
    return false;
  }

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 3;
  sample.name = "data_B";
  sample.nest.value = A;
  ReturnCode_t ret = mdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  if (!waitForSample(dr)) return false;

  DDS::StringSeq params_empty(0);
  params_empty.length(0);
  ReadCondition_var dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "key = %0", params_empty);
  if (dr_qc) {
    cerr << "ERROR: Creating QueryCondition with 1 token and 0 parameters should have failed" << endl;
    return false;
  }

  DDS::StringSeq params_three(3);
  params_three.length(3);
  params_three[0] = "2";
  params_three[1] = "2";
  params_three[2] = "2";
  dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "key = %0", params_three);
  if (dr_qc) {
    cerr << "ERROR: Creating QueryCondition with 1 token and 3 parameters should have failed" << endl;
    return false;
  }

  const char* params1_value = "data_B";
  DDS::StringSeq params(2);
  params.length(2);
  params[0] = "2";
  params[1] = params1_value;
  dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "key = %0 AND name = %1", params);
  if (!dr_qc) {
    cerr << "ERROR: failed to create QueryCondition" << endl;
    return false;
  }

  QueryCondition_var query_cond = QueryCondition::_narrow(dr_qc);
  CORBA::String_var expr = query_cond->get_query_expression();
  if (std::string("key = %0 AND name = %1") != expr.in()) {
    cerr << "ERROR: get_query_expression() query expression should match " << endl;
    return false;
  }

  params = DDS::StringSeq();
  ret = query_cond->get_query_parameters(params);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: get_query_parameters() failed " << endl;
    return false;
  } else if (params.length() != 2 || std::string(params[0]) != "2" || std::string(params[1]) != params1_value) {
    cerr << "ERROR: get_query_parameters() query parameters doesn't match " << endl;
    return false;
  }

  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_qc);
  ConditionSeq active;
  ret = ws->wait(active, max_wait_time);
  // expect a timeout because the sample doesn't match the query string
  if (ret != RETCODE_TIMEOUT) {
    cerr << "ERROR: wait(qc) should have timed out" << endl;
    return false;
  }
  ws->detach_condition(dr_qc);

  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  MessageSeq data;
  SampleInfoSeq infoseq;
  ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_NO_DATA) {
    cerr << "ERROR: take_w_condition(qc) shouldn't have returned data" << endl;
    return false;
  }

  if (query_cond->set_query_parameters(params_empty) != RETCODE_ERROR) {
    cerr << "ERROR: Setting 0 parameters for query condition with 2 tokens should have failed " << endl;
    return false;
  }

  if (query_cond->set_query_parameters(params_three) != RETCODE_ERROR) {
    cerr << "ERROR: Setting 3 parameters for query condition with 2 tokens should have failed" << endl;
    return false;
  }

  params = DDS::StringSeq(2);
  params.length(2);
  params[0] = "3";
  params[1] = params1_value;
  ret = query_cond->set_query_parameters(params);

  params = DDS::StringSeq();
  ret = query_cond->get_query_parameters(params);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: get_query_parameters() failed " << endl;
    return false;
  } else if (params.length() != 2 || std::string(params[0]) != "3" || std::string(params[1]) != params1_value) {
    cerr << "ERROR: get_query_parameters() query parameters doesn't match " << endl;
    return false;
  }

  ws->attach_condition(dr_qc);
  ret = ws->wait(active, max_wait_time);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: wait(qc) should not time out" << endl;
    return false;
  } else {
    for (CORBA::ULong i(0); i < data.length(); ++i) {
      cout << "Info:\tinstance_handle = " << infoseq[i].instance_handle <<
        "\tsample_rank = " << infoseq[i].sample_rank << '\n';
      if (infoseq[i].valid_data) {
        cout << "Data:\tkey = " << data[i].key <<
          " \tname = " << data[i].name <<
          "\tnest.value = " << data[i].nest.value <<
          '\n';
      }
    }
  }
  ws->detach_condition(dr_qc);

  ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: take_w_condition(qc) should have returned data" << endl;
    return false;
  }

  dr->delete_readcondition(dr_qc);
  if (!test_cleanup(dp, pub, sub, dw, dr)) {
    cerr << "ERROR: run_change_parameter_test: cleanup failed" << endl;
    return false;
  }
  return true;
}

bool run_single_dispose_filter_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub,
  const char* query, bool expect_dispose)
{
  ReturnCode_t ret;

  DataWriter_var dw;
  DataReader_var dr;
  const char* topic_name = expect_dispose ?
    "Dispose with Safe Query" : "Dispose with Unsafe Query";
  if (!test_setup(dp, ts, pub, sub, topic_name, dw, dr)) {
    cerr << "ERROR: run_single_dispose_filter_test: setup failed" << endl;
    return false;
  }

  // Create QueryCondition
  ReadCondition_var dr_qc = dr->create_querycondition(
    ANY_SAMPLE_STATE, ANY_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE,
    query, DDS::StringSeq());
  if (!dr_qc) {
    cerr << "ERROR: run_single_dispose_filter_test: create read condition failed" << endl;
    return false;
  }
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_qc);
  ConditionSeq active;

  // Write Sample with Valid Data
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 0;
  sample.iteration = 0;
  sample.nest.value = A;
  ret = mdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: write failed: "
      << retcode_to_string(ret) << endl;
    return false;
  }

  // Create Dispose Sample with Invalid Data by Disposing the Sample Instance
  ret = mdw->dispose(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: "
      "Dispose instance failed: " << retcode_to_string(ret) << endl;
    return false;
  }

  // Wait for samples matching the query from the disposed instance
  if (ws->wait(active, max_wait_time) != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: wait failed" << endl;
    return false;
  }
  ws->detach_condition(dr_qc);

  // Read the Number of Invalid Messages Taken
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  MessageSeq data;
  SampleInfoSeq infoseq;
  ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: take_w_condition failed: "
      << retcode_to_string(ret) << endl;
    return false;
  }
  unsigned num_valid = 0;
  unsigned num_invalid = 0;
  for (CORBA::ULong i = 0; i < infoseq.length(); i++) {
    if (infoseq[i].valid_data) {
      num_valid++;
    } else {
      num_invalid++;
    }
  }

  // Compare Numbers to what was Expected
  if (num_valid != 1) {
    cerr << "ERROR: run_single_dispose_filter_test: "
      "expected one sample with valid data, got " << num_valid << endl;
    return false;
  }
  if (num_invalid != (expect_dispose ? 1 : 0)) {
    cerr << "ERROR: run_single_dispose_filter_test: expected "
      << (expect_dispose ? "one sample" : "no samples")
      << " with invalid data, got " << num_invalid << endl;
    return false;
  }

  // Cleanup
  ret = dr->delete_readcondition(dr_qc);
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: delete_readcondition failed: "
        << retcode_to_string(ret) << endl;
  }
  if (!test_cleanup(dp, pub, sub, dw, dr)) {
    cerr << "ERROR: run_single_dispose_filter_test: setup failed" << endl;
    return false;
  }

  return true;
}

bool run_dispose_filter_tests(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  /*
   * Run a "Safe" Query that just references key;
   * assert a normal message and a dispose message are in the results.
   */
  if (!run_single_dispose_filter_test(dp, ts, pub, sub, "key >= 0", true)) {
    cerr << "ERROR: run_dispose_filter_tests: safe query test failed!" << endl;
    return false;
  }

  /*
   * Setup a "Unsafe" Query that references key and a normal field;
   * assert just a normal message is in the results.
   */
  if (!run_single_dispose_filter_test(dp, ts, pub, sub, "key >= 0 AND iteration >= 0", false)) {
    cerr << "ERROR: run_dispose_filter_tests: unsafe query test failed!" << endl;
    return false;
  }

  return true;
}

int run_test(int argc, ACE_TCHAR *argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, "");

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                           DEFAULT_STATUS_MASK);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);

  bool passed = true;
  passed &= run_sorting_test(dp, ts, pub, sub);
  passed &= run_filtering_test(dp, ts, pub, sub);
  passed &= run_change_parameter_test(dp, ts, pub, sub);
  passed &= run_complex_filtering_test(dp, ts, pub, sub);
  passed &= run_dispose_filter_tests(dp, ts, pub, sub);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  return passed ? 0 : 1;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = 1;
  try
  {
    ret = run_test(argc, argv);
  }
  catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in QueryConditionTest.cpp:");
    return 1;
  }

  // cleanup
  TheServiceParticipant->shutdown ();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
