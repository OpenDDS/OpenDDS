#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "MessengerTypeSupportImpl.h"

#include <cstdlib>
#include <iostream>

#if defined __GNUC__ && __GNUC__ == 4 && __GNUC_MINOR__ <= 1
# define INT64_LITERAL_SUFFIX(X) X ## ll
#else
# define INT64_LITERAL_SUFFIX(X) X
#endif

using namespace std;
using namespace DDS;
using namespace OpenDDS::DCPS;
using namespace Messenger;

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
    cout << "ERROR: wait(rc) failed" << endl;
    return false;
  }
  return true;
}

template <typename F>
size_t takeSamples(const DataReader_var& dr, F filter)
{
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  size_t count(0);
  while (true) {
    MessageSeq data;
    SampleInfoSeq infoseq;
    ReturnCode_t ret = mdr->take(data, infoseq, LENGTH_UNLIMITED,
      ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    if (ret == RETCODE_NO_DATA) {
      break;
    }
    if (ret != RETCODE_OK) {
      cout << "ERROR: take() should have returned some data" << endl;
      return 0;
    }
    for (CORBA::ULong i(0); i < data.length(); ++i) {
      if (infoseq[i].valid_data) {
        ++count;
        cout << dr << " received data with key == " << data[i].key << endl;
        if (!filter(data[i].key)) {
          cout << "ERROR: data should be filtered" << endl;
          return 0;
        }
      }
    }
  }
  return count;
}

bool run_filtering_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub, const Subscriber_var& sub2)
{
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic", type_name,
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  DataWriter_var dw =
    pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample = {0, 0};
  if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false; // durable, filtered
  sample.key = 99;
  if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false; // durable, not filtered

  DataWriter_var dw2 =
    pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  DataReaderQos dr_qos_durable = dr_qos;
  dr_qos_durable.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  ContentFilteredTopic_var cft = dp->create_contentfilteredtopic(
    "MyTopic-Filtered", topic, "key > 1", StringSeq());
  if (!cft) {
    cout << "ERROR: creating cft failed" << endl;
    return false;
  }
  DataReader_var dr =
    sub->create_datareader(cft, dr_qos_durable, 0, DEFAULT_STATUS_MASK);
  TopicDescription_var td = dr->get_topicdescription();
  ContentFilteredTopic_var cft_from_td = ContentFilteredTopic::_narrow(td);
  if (!cft_from_td) {
    cout << "ERROR: get_topicdescription() did not return the CFT\n";
    return false;
  }
  DataReader_var dr2 =
    sub->create_datareader(topic, dr_qos, 0, DEFAULT_STATUS_MASK);

  DataReader_var sub2_dr =
    sub2->create_datareader(cft, dr_qos, 0, DEFAULT_STATUS_MASK);
  DDS::StringSeq mytopicfiltered2_params(1);
  mytopicfiltered2_params.length(1);
  ContentFilteredTopic_var cft2 = dp->create_contentfilteredtopic(
    "MyTopic-Filtered2", topic, "key > %0", mytopicfiltered2_params);
  DataReader_var sub2_dr2 =
    sub2->create_datareader(cft2, dr_qos, 0, DEFAULT_STATUS_MASK);
  const int N_MATCHES = 4; // each writer matches 4 readers

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  PublicationMatchedStatus status;
  while (dw->get_publication_matched_status(status) == DDS::RETCODE_OK
         && status.current_count < N_MATCHES) {
    ConditionSeq active;
    if (ws->wait(active, infinite) != DDS::RETCODE_OK) {
      return false;
    }
  }
  ws->detach_condition(dw_sc);

  // read durable data from dr
  if (!waitForSample(dr)) return false;
  if (takeSamples(dr, bind2nd(greater<CORBA::Long>(), 98)) != 1) {
    cout << "ERROR: take() should have returned a valid durable sample (99)"
         << endl;
    return false;
  }

  // Create a cft where the parameter sequence doesn't match the size
  DDS::StringSeq paramssize(2);
  paramssize.length(2);
  ContentFilteredTopic_var cft3 = dp->create_contentfilteredtopic(
    "MyTopic-Filtered3", topic, "key > %0", paramssize);
  if (cft3) {
    cout << "ERROR: creating cft3 with invalid parameter size should fail"
         << endl;
    return false;
  }

  // Try to set expression parameters that have a parameter too much
  // compared to the expression, this should fail
  DDS::StringSeq params_large(2);
  params_large.length(2);
  params_large[0] = "2";
  params_large[1] = "2";
  if (cft2->set_expression_parameters(params_large) != RETCODE_ERROR) {
    cout << "ERROR: setting too much parameters should return an error"
         << endl;
    return false;
  }

  DDS::StringSeq params_empty(0);
  params_empty.length(0);
  if (cft2->set_expression_parameters(params_empty) != RETCODE_ERROR) {
    cout << "ERROR: setting empty parameters should return an error"
         << endl;
    return false;
  }

  DDS::StringSeq params(1);
  params.length(1);
  params[0] = "2";
  if (cft2->set_expression_parameters(params) != RETCODE_OK) {
    cout << "ERROR: setting expression parameters failed"
         << endl;
    return false;
  }

  for (sample.key = 1; sample.key < 4; ++sample.key) {
    if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;
  }

  if (!waitForSample(dr)) return false;

  size_t taken = takeSamples(dr, bind2nd(greater<CORBA::Long>(), 1));
  if (taken == 1) {
    cout << "INFO: partial read on DataReader \"dr\"\n";
    if (!waitForSample(dr)) return false;
    taken += takeSamples(dr, bind2nd(greater<CORBA::Long>(), 1));
  }

  if (taken != 2) {
    cout << "ERROR: take() should have returned two valid samples" << endl;
    return false;
  }

  if (!waitForSample(sub2_dr2)) return false;

  if (takeSamples(sub2_dr2, bind2nd(greater<CORBA::Long>(), 2)) != 1) {
    cout << "ERROR: take() should have returned one valid sample" << endl;
    return false;
  }

  if (sub->delete_datareader(dr2) != RETCODE_OK) {
    cout << "ERROR: delete_datareader(dr2)" << endl;
    return false;
  }
  dr2 = DataReader::_nil();

  sample.key = 0; // no DataLink receives this sample
  if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;

  Duration_t wfa = {60 /*seconds*/, 0 /*nanoseconds*/};
  if (mdw->wait_for_acknowledgments(wfa) != RETCODE_OK) {
    cout << "ERROR: wait_for_acknowledgments 1" << endl;
    return false;
  }

  // To set up a more difficult wait_for_acknowledgements() scenario,
  // make sure the sub2 datalink's two readers have different "latest"
  // sequence numbers, and the sub datalink needs no customization.
  sample.key = 2;
  if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;

  if (mdw->wait_for_acknowledgments(wfa) != RETCODE_OK) {
    cout << "ERROR: wait_for_acknowledgments 2" << endl;
    return false;
  }

  if (dp->delete_contentfilteredtopic(cft) != RETCODE_PRECONDITION_NOT_MET) {
    cout << "ERROR: delete_contentfilteredtopic should return "
      "RETCODE_PRECONDITION_NOT_MET when datareader still exists" << endl;
    return false;
  }

  if (sub->delete_datareader(dr) != RETCODE_OK) {
    cout << "ERROR: delete_datareader" << endl;
    return false;
  }
  dr = DataReader::_nil();

  if (sub2->delete_datareader(sub2_dr) != RETCODE_OK) {
    cout << "ERROR: delete_datareader(sub2_dr)" << endl;
    return false;
  }
  sub2_dr = DataReader::_nil();

  if (dp->delete_contentfilteredtopic(cft) != RETCODE_OK) {
    cout << "ERROR: delete_contentfilteredtopic" << endl;
    return false;
  }
  return true;
}

bool run_unsignedlonglong_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub, const Subscriber_var& sub)
{
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic2", type_name,
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);
  DataWriter_var dw =
    pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  DDS::StringSeq params(1);
  params.length(1);
  params[0] = "1485441228338";
  ContentFilteredTopic_var cft = dp->create_contentfilteredtopic(
    "MyTopic2-Filtered", topic, "ull > %0 AND ull < 1485441228340", params);
  DataReader_var dr =
    sub->create_datareader(cft, DATAREADER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  PublicationMatchedStatus status;
  while (dw->get_publication_matched_status(status) == DDS::RETCODE_OK
         && status.current_count < 1) {
    ConditionSeq active;
    if (ws->wait(active, infinite) != DDS::RETCODE_OK) {
      return false;
    }
  }
  ws->detach_condition(dw_sc);

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample = {0, INT64_LITERAL_SUFFIX(1485441228338)};
  for (; sample.key < 3; ++sample.key, ++sample.ull) {
    if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;
  }

  if (!waitForSample(dr)) return false;
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  size_t count(0);
  while (true) {
    MessageSeq data;
    SampleInfoSeq infoseq;
    ReturnCode_t ret = mdr->take(data, infoseq, LENGTH_UNLIMITED,
                                 ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    if (ret == RETCODE_NO_DATA) {
      break;
    }
    if (ret != RETCODE_OK) {
      cout << "ERROR: take() should have returned some data" << endl;
      return 0;
    }
    for (CORBA::ULong i(0); i < data.length(); ++i) {
      if (infoseq[i].valid_data) {
        ++count;
        cout << dr << " received data with ull == " << data[i].ull << endl;
        if (data[i].ull != INT64_LITERAL_SUFFIX(1485441228339)) {
          cout << "ERROR: received unexpected value\n";
          return false;
        }
      }
    }
  }
  if (count != 1) {
    cout << "ERROR: expected 1 message, received " << count << endl;
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

  TransportRegistry::instance()->bind_config("c1", pub);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);

  TransportRegistry::instance()->bind_config("c2", sub);

  Subscriber_var sub2 = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                              DEFAULT_STATUS_MASK);

  TransportRegistry::instance()->bind_config("c3", sub2);

  bool passed = run_filtering_test(dp, ts, pub, sub, sub2);
  passed &= run_unsignedlonglong_test(dp, ts, pub, sub);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = EXIT_FAILURE;
  try
  {
    ret = run_test(argc, argv);
  }
  catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in ContentFilteredTopicTest.cpp:");
    return 1;
  }
  catch (const OpenDDS::DCPS::Transport::MiscProblem&)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Transport::MiscProblem caught.\n")), -1);
  }

  // cleanup
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
