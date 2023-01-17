#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#include <dds/DCPS/XTypes/DynamicDataFactory.h>
#include <dds/DCPS/XTypes/DynamicTypeSupport.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/WaitForSample.h"

#include <ace/Argv_Type_Converter.h>

#include <cstdlib>
#include <functional>
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

enum DynamicConfig {
  DynamicNone,
  DynamicWriter,
  DynamicReader,
} dynamic = DynamicNone;

bool waitForPublicationMatched(const DataWriter_var& dw, const int count = 1)
{
  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  PublicationMatchedStatus status;
  while (dw->get_publication_matched_status(status) == DDS::RETCODE_OK
         && status.current_count < count) {
    ConditionSeq active;
    if (ws->wait(active, infinite) != DDS::RETCODE_OK) {
      cerr << "Publication Matched Failed" << endl;
      return true;
    }
  }
  ws->detach_condition(dw_sc);

  return false;
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

DynamicData_var copy(const Message& sample, DDS::TypeSupport* ts)
{
  DynamicType_var type = ts->get_type();
  DynamicData_var dd = DynamicDataFactory::get_instance()->create_data(type);
  dd->set_int32_value(dd->get_member_id_by_name("key"), sample.key);
  dd->set_uint64_value(dd->get_member_id_by_name("ull"), sample.ull);
  return dd;
}

struct Writers {
  explicit Writers(DataWriter* dw)
    : msg_writer_(dynamic == DynamicWriter ? 0 : MessageDataWriter::_narrow(dw))
    , dyn_writer_(dynamic == DynamicWriter ? DynamicDataWriter::_narrow(dw) : 0)
  {
    if (dyn_writer_) {
      Topic_var topic = dyn_writer_->get_topic();
      CORBA::String_var type = topic->get_type_name();
      DomainParticipant_var participant = topic->get_participant();
      type_support_ = Registered_Data_Types->lookup(participant, type);
    }
  }

  ReturnCode_t write(const Message& sample, InstanceHandle_t ih = HANDLE_NIL)
  {
    if (msg_writer_) {
      return msg_writer_->write(sample, ih);
    }
    DynamicData_var dyn = copy(sample, type_support_);
    return dyn_writer_->write(dyn, ih);
  }

  MessageDataWriter_var msg_writer_;
  DynamicDataWriter_var dyn_writer_;
  DDS::TypeSupport_var type_support_;
};

bool run_filtering_test(const MessageTypeSupport_var& ts,
  const Publisher_var& pub, const Subscriber_var& sub, const Subscriber_var& sub2)
{
  CORBA::String_var type_name = ts->get_type_name();
  DomainParticipant_var dp_pub = pub->get_participant();
  Topic_var topic_pub = dp_pub->create_topic("MyTopic", type_name, TOPIC_QOS_DEFAULT, 0, 0);

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  DataWriter_var dw = pub->create_datawriter(topic_pub, dw_qos, 0, 0);
  Message sample = {0, 0};
  Writers writers(dw);
  if (writers.write(sample) != RETCODE_OK) return false; // durable, filtered
  sample.key = 99;
  if (writers.write(sample) != RETCODE_OK) return false; // durable, not filtered

  DataWriter_var dw2 = pub->create_datawriter(topic_pub, dw_qos, 0, 0);

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  DataReaderQos dr_qos_durable = dr_qos;
  dr_qos_durable.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DomainParticipant_var dp_sub = sub->get_participant();
  Topic_var topic_sub = dp_sub->create_topic("MyTopic", type_name, TOPIC_QOS_DEFAULT, 0, 0);

  ContentFilteredTopic_var cft = dp_sub->create_contentfilteredtopic(
    "MyTopic-Filtered", topic_sub, "key > 1", StringSeq());
  if (!cft) {
    cout << "ERROR: creating cft failed" << endl;
    return false;
  }
  DataReader_var dr = sub->create_datareader(cft, dr_qos_durable, 0, 0);
  TopicDescription_var td = dr->get_topicdescription();
  ContentFilteredTopic_var cft_from_td = ContentFilteredTopic::_narrow(td);
  if (!cft_from_td) {
    cout << "ERROR: get_topicdescription() did not return the CFT\n";
    return false;
  }
  DataReader_var dr2 = sub->create_datareader(topic_sub, dr_qos, 0, 0);

  DataReader_var sub2_dr =
    sub2->create_datareader(cft, dr_qos, 0, DEFAULT_STATUS_MASK);
  DDS::StringSeq mytopicfiltered2_params(1);
  mytopicfiltered2_params.length(1);
  ContentFilteredTopic_var cft2 = dp_sub->create_contentfilteredtopic(
    "MyTopic-Filtered2", topic_sub, "key > %0", mytopicfiltered2_params);
  DataReader_var sub2_dr2 =
    sub2->create_datareader(cft2, dr_qos, 0, DEFAULT_STATUS_MASK);

  waitForPublicationMatched(dw, 4); // each writer matches 4 readers

  // read durable data from dr
  if (!Utils::waitForSample(dr)) return false;
#ifdef ACE_HAS_CPP11
  if (takeSamples(dr, [](ACE_CDR::Long i){ return i > 98; }) != 1) {
#else
  if (takeSamples(dr, bind2nd(greater<CORBA::Long>(), 98)) != 1) {
#endif
    cout << "ERROR: take() should have returned a valid durable sample (99)"
         << endl;
    return false;
  }

  // Create a cft where the parameter sequence doesn't match the size
  DDS::StringSeq paramssize(2);
  paramssize.length(2);
  ContentFilteredTopic_var cft3 = dp_sub->create_contentfilteredtopic(
    "MyTopic-Filtered3", topic_sub, "key > %0", paramssize);
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
    if (writers.write(sample) != RETCODE_OK) return false;
  }

  if (!Utils::waitForSample(dr)) return false;

#ifdef ACE_HAS_CPP11
  size_t taken = takeSamples(dr, bind(greater<CORBA::Long>(), placeholders::_1, 1));
#else
  size_t taken = takeSamples(dr, bind2nd(greater<CORBA::Long>(), 1));
#endif
  if (taken == 1) {
    cout << "INFO: partial read on DataReader \"dr\"\n";
    if (!Utils::waitForSample(dr)) return false;
#ifdef ACE_HAS_CPP11
    taken += takeSamples(dr, bind(greater<CORBA::Long>(), placeholders::_1, 1));
#else
    taken += takeSamples(dr, bind2nd(greater<CORBA::Long>(), 1));
#endif
  }

  if (taken != 2) {
    cout << "ERROR: take() should have returned two valid samples" << endl;
    return false;
  }

  if (!Utils::waitForSample(sub2_dr2)) return false;

#ifdef ACE_HAS_CPP11
  if (takeSamples(sub2_dr2, bind(greater<CORBA::Long>(), placeholders::_1, 2)) != 1) {
#else
  if (takeSamples(sub2_dr2, bind2nd(greater<CORBA::Long>(), 2)) != 1) {
#endif
    cout << "ERROR: take() should have returned one valid sample" << endl;
    return false;
  }

  if (sub->delete_datareader(dr2) != RETCODE_OK) {
    cout << "ERROR: delete_datareader(dr2)" << endl;
    return false;
  }
  dr2 = DataReader::_nil();

  sample.key = 0; // no DataLink receives this sample
  if (writers.write(sample) != RETCODE_OK) return false;

  Duration_t wfa = {60 /*seconds*/, 0 /*nanoseconds*/};
  if (dw->wait_for_acknowledgments(wfa) != RETCODE_OK) {
    cout << "ERROR: wait_for_acknowledgments 1" << endl;
    return false;
  }

  // To set up a more difficult wait_for_acknowledgments() scenario,
  // make sure the sub2 datalink's two readers have different "latest"
  // sequence numbers, and the sub datalink needs no customization.
  sample.key = 2;
  if (writers.write(sample) != RETCODE_OK) return false;

  if (dw->wait_for_acknowledgments(wfa) != RETCODE_OK) {
    cout << "ERROR: wait_for_acknowledgments 2" << endl;
    return false;
  }

  if (dp_sub->delete_contentfilteredtopic(cft) != RETCODE_PRECONDITION_NOT_MET) {
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

  if (dp_sub->delete_contentfilteredtopic(cft) != RETCODE_OK) {
    cout << "ERROR: delete_contentfilteredtopic" << endl;
    return false;
  }
  return true;
}

bool run_unsignedlonglong_test(const MessageTypeSupport_var& ts, const Publisher_var& pub, const Subscriber_var& sub)
{
  CORBA::String_var type_name = ts->get_type_name();
  DomainParticipant_var dp_pub = pub->get_participant();
  Topic_var topic_pub = dp_pub->create_topic("MyTopic2", type_name, TOPIC_QOS_DEFAULT, 0, 0);
  DataWriter_var dw = pub->create_datawriter(topic_pub, DATAWRITER_QOS_DEFAULT, 0, 0);

  DomainParticipant_var dp_sub = sub->get_participant();
  Topic_var topic_sub = dp_sub->create_topic("MyTopic2", type_name, TOPIC_QOS_DEFAULT, 0, 0);

  DDS::StringSeq params(1);
  params.length(1);
  params[0] = "1485441228338";
  ContentFilteredTopic_var cft = dp_sub->create_contentfilteredtopic(
    "MyTopic2-Filtered", topic_sub, "ull > %0 AND ull < 1485441228340", params);
  DataReader_var dr =
    sub->create_datareader(cft, DATAREADER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  if (waitForPublicationMatched(dw)) {
    return false;
  }

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample = {0, INT64_LITERAL_SUFFIX(1485441228338)};
  for (; sample.key < 3; ++sample.key, ++sample.ull) {
    if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;
  }

  if (!Utils::waitForSample(dr)) return false;
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

/*
 * NOTE: There is a test almost exactly like this in the QueryConditon test to
 * test the same situation with QueryConditions.
 */
bool run_single_dispose_filter_test(const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub, const char* filter, bool expect_dispose)
{
  Duration_t max_wait_time = {3, 0}; // 3 sec
  ReturnCode_t ret;

  // Create Topic
  CORBA::String_var type_name = ts->get_type_name();
  DomainParticipant_var dp_pub = pub->get_participant();
  Topic_var topic_pub = dp_pub->create_topic("Messenger Topic", type_name, TOPIC_QOS_DEFAULT, 0, 0);

  // Create ContentFilteredTopic
  DomainParticipant_var dp_sub = sub->get_participant();
  Topic_var topic_sub = dp_sub->create_topic("Messenger Topic", type_name, TOPIC_QOS_DEFAULT, 0, 0);
  ContentFilteredTopic_var cft = dp_sub->create_contentfilteredtopic(
    expect_dispose ? "Safe Filtered Messenger Topic" : "Unsafe Filtered Messenger Topic",
    topic_sub, filter, StringSeq());
  if (!cft) {
    cerr << "ERROR: run_single_dispose_filter_test: creating ContentFilteredTopic failed" << endl;
    return false;
  }

  // Create Writer
  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  DataWriter_var dw = pub->create_datawriter(topic_pub, dw_qos, 0, 0);

  // Create Reader
  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  DataReader_var dr = sub->create_datareader(cft, dr_qos, 0, 0);

  if (waitForPublicationMatched(dw)) {
    return false;
  }

  // Write Sample with Valid Data
  if (dynamic == DynamicWriter) {
    //TODO
  } else {
    MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
    Message sample;
    sample.key = 0;
    sample.ull = 0;
    ret = mdw->write(sample, HANDLE_NIL);
    if (ret != RETCODE_OK) {
      cerr << "ERROR: run_single_dispose_filter_test: write failed" << endl;
      return false;
    }

    // Create Dispose Sample with Invalid Data by Disposing the instance
    mdw->dispose(sample, HANDLE_NIL);
  }

  // Wait for samples matching the filter from the disposed instance
  ReadCondition_var disposed_condition = dr->create_readcondition(
    ANY_SAMPLE_STATE, ANY_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  if (!disposed_condition) {
    cerr << "ERROR: run_single_dispose_filter_test: "
      "create disposed read condition failed" << endl;
    return false;
  }
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(disposed_condition);
  ConditionSeq active;
  if (ws->wait(active, max_wait_time) != RETCODE_OK) {
    // In the case of ContentFilteredTopic a DISPOSE/UNREGISTER sample
    // doesn't dispose or unregister the instance if the filter has non-key
    // fields.
    if (expect_dispose) {
      cerr << "ERROR: run_single_dispose_filter_test: "
        "wait for disposed sample failed" << endl;
      return false;
    }
  }
  ws->detach_condition(disposed_condition);

  // Read the Number of Invalid Messages Taken
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  MessageSeq data;
  SampleInfoSeq infoseq;
  if (expect_dispose) {
    ret = mdr->take_w_condition(data, infoseq, LENGTH_UNLIMITED,
      disposed_condition);
  } else {
    ret = mdr->take(data, infoseq, LENGTH_UNLIMITED,
      ANY_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
  }
  if (ret != RETCODE_OK) {
    cerr << "ERROR: run_single_dispose_filter_test: take failed" << endl;
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

  dr->delete_readcondition(disposed_condition);
  sub->delete_datareader(dr);
  pub->delete_datawriter(dw);
  dp_sub->delete_contentfilteredtopic(cft);
  dp_sub->delete_topic(topic_sub);
  dp_pub->delete_topic(topic_pub);
  return true;
}

bool run_dispose_filter_tests(const MessageTypeSupport_var& ts, const Publisher_var& pub, const Subscriber_var& sub)
{
  /*
   * Run a "Safe" Filter that just references a key.
   * Assert a normal message and a dispose message are in the results.
   */
  if (!run_single_dispose_filter_test(ts, pub, sub, "key >= 0", true)) {
    cerr << "ERROR: run_dispose_filter_tests: safe filter test failed!" << endl;
    return false;
  }

  /*
   * Setup a "Unsafe" Filter that references a key and a normal field.
   * Assert that there is a normal field in the results and the instance
   * is not disposed.
   */
  if (!run_single_dispose_filter_test(ts, pub, sub, "key >= 0 AND ull >= 0", false)) {
    cerr << "ERROR: run_dispose_filter_tests: unsafe filter test failed!" << endl;
    return false;
  }

  return true;
}

void register_dynamic(DDS::TypeSupport* ts, DDS::DomainParticipant* dp)
{
  DDS::DynamicType_var dt = ts->get_type();
  DDS::TypeSupport_var dts = new DDS::DynamicTypeSupport(dt);
  CORBA::String_var type_name = ts->get_type_name();
  dts->register_type(dp, type_name);
}

int run_test(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  
  ACE_Argv_Type_Converter conv(argc, argv);
  char** const argva = conv.get_ASCII_argv();
  for (int i = 1; i < argc; ++i) {
    if (0 == std::strcmp("-dynamic-writer", argva[i])) {
      dynamic = DynamicWriter;
      break;
    }
    if (0 == std::strcmp("-dynamic-reader", argva[i])) {
      dynamic = DynamicReader;
      break;
    }
  }

  DomainParticipant_var dp1 = dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0, 0);

  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  if (dynamic == DynamicWriter) {
    register_dynamic(ts, dp1);
  } else {
    ts->register_type(dp1, "");
  }

  Publisher_var pub = dp1->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
  TransportRegistry::instance()->bind_config("c1", pub);

  DomainParticipant_var dp2 = dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0, 0);

  if (dynamic == DynamicReader) {
    register_dynamic(ts, dp2);
  } else {
    ts->register_type(dp2, "");
  }

  Subscriber_var sub = dp2->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);
  TransportRegistry::instance()->bind_config("c2", sub);

  Subscriber_var sub2 = dp2->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);
  TransportRegistry::instance()->bind_config("c3", sub2);

  bool passed = true;
  passed &= run_filtering_test(ts, pub, sub, sub2);
  if (dynamic == DynamicNone) { //TODO
    passed &= run_unsignedlonglong_test(ts, pub, sub);
    passed &= run_dispose_filter_tests(ts, pub, sub);
  }

  dp1->delete_contained_entities();
  dpf->delete_participant(dp1);
  dp2->delete_contained_entities();
  dpf->delete_participant(dp2);
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}


int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = EXIT_FAILURE;
  try {
    ret = run_test(argc, argv);
  } catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in ContentFilteredTopicTest.cpp:");
    return 1;
  } catch (const OpenDDS::DCPS::Transport::MiscProblem&) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Transport::MiscProblem caught.\n")), -1);
  }

  // cleanup
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
