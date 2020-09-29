#include "XTypesTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;


template<typename T1, typename T2>
bool read(DataReader_var dr, T1 pdr, T2& data, bool verbose)
{
  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    ANY_VIEW_STATE,
    ALIVE_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_rc);
  unsigned counter = 0;
  std::set<int> instances;

  ConditionSeq active;
  const Duration_t max_wait = { 5, 0 };
  ReturnCode_t ret = ws->wait(active, max_wait);
  if (ret == RETCODE_TIMEOUT) {
    if (verbose) {
      ACE_DEBUG((LM_DEBUG, "reader: Timedout\n"));
    }
    return false;
  } else if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: wait returned %d\n", ret));
    return false;
  }
  // data;
  SampleInfoSeq info;
  while ((ret = pdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_rc)) == RETCODE_OK) {
    for (unsigned int i = 0; i < data.length(); ++i) {
      ++counter;
      instances.insert(data[i].key);
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "reader: Counter: %u Instance: %d\n", counter, data[i].key));
      }
    }
  }
  if (ret != RETCODE_NO_DATA && ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: take_w_condition returned %d\n", ret));
    return false;
  }

  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);
  return true;
}


template<typename T>
void get_topic(T ts, const DomainParticipant_var dp, const char* topic_name, Topic_var& topic)
{
  ts->register_type(dp, "");
  CORBA::String_var type_name = ts->get_type_name();
  topic = dp->create_topic(topic_name, type_name,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  bool verbose = false;
  bool writer = false;
  bool reader = false;
  std::string type;

  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--writer")) {
      writer = true;
    } else if (arg == ACE_TEXT("--reader")) {
      reader = true;
    } else if (arg == ACE_TEXT("--type")) {
      if (i + 1 < argc) {
        type = argv[++i];
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid type argument"));
        return 1;
      }
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  bool failed = false;

  Topic_var topic;
  if (type == "Property_1") {
    Property_1TypeSupport_var ts = new Property_1TypeSupportImpl;
    get_topic(ts, dp, "MyTopic1", topic);
  } else if (type == "Property_2") {
    Property_2TypeSupport_var ts = new Property_2TypeSupportImpl;
    get_topic(ts, dp, "MyTopic2", topic);
  }

  if (writer) {
    Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
    DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
      DEFAULT_STATUS_MASK);

    if (type == "Property_1") {
      Property_1DataWriter_var pdw = Property_1DataWriter::_narrow(dw);

      Property_1 p;
      p.key = 1;
      p.value = 1;
      pdw->write(p, HANDLE_NIL);
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "writer: Property_1\n"));
      }
    } else if (type == "Property_2") {
      Property_2DataWriter_var pdw2 = Property_2DataWriter::_narrow(dw);

      Property_2 p2;
      p2.key = 1;
      p2.value = "Test";
      pdw2->write(p2, HANDLE_NIL);
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "writer: Property_2\n"));
      }
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
      failed = 1;
    }

    ACE_OS::sleep(ACE_Time_Value(0, 5000 * 1000));

  } else if (reader) {
    ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));

    Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                               DEFAULT_STATUS_MASK);
    DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    DataReader_var dr = sub->create_datareader(topic, dr_qos, 0,
      DEFAULT_STATUS_MASK);

    if (type == "Property_1") {
      Property_1DataReader_var pdr = Property_1DataReader::_narrow(dr);
      ::Property_1Seq data;
      failed = !read(dr, pdr, data, verbose);
    } else if (type == "Property_2") {
      Property_2DataReader_var pdr = Property_2DataReader::_narrow(dr);
      ::Property_2Seq data;
      failed = !read(dr, pdr, data, verbose);
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
      failed = 1;
    }

    if (failed) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %s\n", type.c_str()));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass either --writer or --reader\n"));
    failed = 1;
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
