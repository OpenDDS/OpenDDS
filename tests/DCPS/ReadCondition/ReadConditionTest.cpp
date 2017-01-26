#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/StaticIncludes.h"

#include "MessengerTypeSupportImpl.h"
#include <iostream>
using namespace std;

void received_data(const Messenger::MessageSeq& data,
                   Messenger::MessageDataWriter_ptr mdw,
                   Messenger::Message& msg)
{
  for (CORBA::ULong j(0); j < data.length(); ++j) {
    cout << "took sample " << data[j].subject_id << endl;
    if (data[j].subject_id > 8) mdw->dispose(msg, DDS::HANDLE_NIL);
  }
}

int run_test_instance(DDS::DomainParticipant_ptr dp)
{
  using namespace DDS;
  using namespace OpenDDS::DCPS;
  using namespace Messenger;
  WaitSet_var ws = new WaitSet;
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic", type_name,
    TOPIC_QOS_DEFAULT, 0, ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataReader_var dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(dw_sc);

  ReturnCode_t ret = RETCODE_OK;
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message msg = {0};
  for (int i(0); i < 12; ++i) {
    ++msg.subject_id;
    ret = mdw->write(msg, HANDLE_NIL);
    if (ret != RETCODE_OK) return ret;
  }

  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);
  ReadCondition_var dr_rc2 = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ws->attach_condition(dr_rc);
  ws->attach_condition(dr_rc2);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  bool passed = true, done = false;
  while (!done) {
    ret = ws->wait(active, infinite);
    if (ret != RETCODE_OK) {
      passed = false;
      break;
    }
    cout << "wait returned" << endl;
    for (CORBA::ULong i(0); i < active.length(); ++i) {
      if (active[i] == dr_rc) {
        // To test both take_w_condition and
        // take_next_instance_w_condition, we'll limit the "take" to 3
        // samples and then use take_next_instance_w_condition.
        MessageSeq data;
        SampleInfoSeq info;
        ret = mdr->take_w_condition(data, info, 3, dr_rc);
        if (ret == RETCODE_NO_DATA) break;
        if (ret != RETCODE_OK) {
          cout << "ERROR: take_w_condition returned " << ret << endl;
          passed = false;
          done = true;
        }
        InstanceHandle_t handle = HANDLE_NIL;
        received_data(data, mdw, msg);
        handle = info[info.length() - 1].instance_handle;
        if (handle == HANDLE_NIL) {
          cout << "ERROR: instance handle is nil" << endl;
          passed = false;
          done = true;
          break;
        }
        cout << "testing take_instance_w_condition" << endl;
        while (true) {
          ret = mdr->take_instance_w_condition(data, info, 1,
                                               handle, dr_rc);
          if (ret == RETCODE_NO_DATA) break;
          if (ret != RETCODE_OK) {
            cout << "ERROR: take_instance_w_condition returned "
                 << ret << endl;
            passed = false;
            done = true;
            break;
          }
          received_data(data, mdw, msg);
        }
      } else if (active[i] == dr_rc2) {
        cout << "an instance has been disposed, exiting" << endl;
        done = true;
      }
    }
  }
  ws->detach_condition(dr_rc);
  ws->detach_condition(dr_rc2);

  dp->delete_contained_entities();
  return passed ? 0 : 1;
}

int run_test_next_instance(DDS::DomainParticipant_ptr dp)
{
  using namespace DDS;
  using namespace OpenDDS::DCPS;
  using namespace Messenger;
  WaitSet_var ws = new WaitSet;
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic", type_name,
    TOPIC_QOS_DEFAULT, 0, ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataReader_var dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0,
    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(dw_sc);

  ReturnCode_t ret = RETCODE_OK;
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message msg = {0};
  for (int i(0); i < 12; ++i) {
    ++msg.subject_id;
    ret = mdw->write(msg, HANDLE_NIL);
    if (ret != RETCODE_OK) return ret;
  }

  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);
  ReadCondition_var dr_rc2 = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ws->attach_condition(dr_rc);
  ws->attach_condition(dr_rc2);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  bool passed = true, done = false;
  while (!done) {
    ret = ws->wait(active, infinite);
    if (ret != RETCODE_OK) {
      passed = false;
      break;
    }
    cout << "wait returned" << endl;
    for (CORBA::ULong i(0); i < active.length(); ++i) {
      if (active[i] == dr_rc) {
        // To test both take_w_condition and
        // take_next_instance_w_condition, we'll limit the "take" to 3
        // samples and then use take_next_instance_w_condition.
        MessageSeq data;
        SampleInfoSeq info;
        ret = mdr->take_w_condition(data, info, 3, dr_rc);
        if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
          cout << "ERROR: take_w_condition returned " << ret << endl;
          passed = false;
          done = true;
        }
        if (ret == RETCODE_OK) {
          received_data(data, mdw, msg);
        }
        cout << "testing take_next_instance_w_condition" << endl;
        InstanceHandle_t handle = HANDLE_NIL;
        while (true) {
          ret = mdr->take_next_instance_w_condition(data, info, 1,
                                                    handle, dr_rc);
          if (ret == RETCODE_NO_DATA) break;
          if (ret != RETCODE_OK) {
            cout << "ERROR: take_next_instance_w_condition returned "
                 << ret << endl;
            passed = false;
            done = true;
          }
          received_data(data, mdw, msg);
          handle = info[info.length() - 1].instance_handle;
        }
      } else if (active[i] == dr_rc2) {
        cout << "an instance has been disposed, exiting" << endl;
        done = true;
      }
    }
  }
  ws->detach_condition(dr_rc);
  ws->detach_condition(dr_rc2);

  dp->delete_contained_entities();
  return passed ? 0 : 1;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  using namespace DDS;
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0, ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  int ret = run_test_next_instance(dp);
  ret += run_test_instance(dp);

  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
