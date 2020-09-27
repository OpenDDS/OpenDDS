#include "XTypesTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <set>

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

const unsigned large_sample_seq_size =
 static_cast<unsigned>(
    OpenDDS::DCPS::TransportSendStrategy::UDP_MAX_MESSAGE_SIZE);

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  PropertyTypeSupport_var ts = new PropertyTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic", type_name,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  bool verbose = false;
  bool writer = false;
  bool reader = false;
  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--writer")) {
      writer = true;
    } else if (arg == ACE_TEXT("--reader")) {
      reader = true;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  bool failed = false;

  if (writer) {
    Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
    DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
                                               DEFAULT_STATUS_MASK);
    PropertyDataWriter_var pdw = PropertyDataWriter::_narrow(dw);

    Property p;
    p.extra.length(0);

    for (int i = 1; i <= 1000; ++i) {
      p.key = i;
      p.value = i;
      pdw->write(p, HANDLE_NIL);
    }

    for (int c = 1; c < 75; ++c) {
      for (int i = 1; i <= 1000; i += 50) {
        p.key = i;
        p.value = i + c;
        pdw->write(p, HANDLE_NIL);
      }
      ACE_OS::sleep(ACE_Time_Value(0, 500 * 1000)); // 1/2 sec
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "writer: Count: %d\n", c));
      }
    }

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
    PropertyDataReader_var pdr = PropertyDataReader::_narrow(dr);
    ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
                                                       ANY_VIEW_STATE,
                                                       ALIVE_INSTANCE_STATE);
    WaitSet_var ws = new WaitSet;
    ws->attach_condition(dr_rc);
    unsigned counter = 0;
    const unsigned minimum_sample_count = 981;
    std::set<int> instances;
    while (true) {
      ConditionSeq active;
      const Duration_t max_wait = {10, 0};
      ReturnCode_t ret = ws->wait(active, max_wait);
      if (ret == RETCODE_TIMEOUT) {
        if (verbose) {
          ACE_DEBUG((LM_DEBUG, "reader: Timedout\n"));
        }
        break;
      } else if (ret != RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "ERROR: Reader: wait returned %d\n", ret));
        failed = true;
        break;
      }
      ::PropertySeq data;
      SampleInfoSeq info;
      while ((ret = pdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_rc)) == RETCODE_OK) {
        for (unsigned int i = 0; i < data.length(); ++i) {
          ++counter;
          instances.insert(data[i].key);
          if (verbose) {
            ACE_DEBUG((LM_DEBUG, "reader: Counter: %u Instance: %d\n", counter, data[i].key));
          }

          if (counter == minimum_sample_count) {
            ACE_DEBUG((LM_DEBUG, "reader: Counter reached %u at %T\n", minimum_sample_count));
          }
        }
      }
      if (ret != RETCODE_NO_DATA && ret != RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "ERROR: Reader: take_w_condition returned %d\n", ret));
        failed = true;
        break;
      }
    }
    if (counter < minimum_sample_count) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader failed to get %d samples, got only %d\n",
        minimum_sample_count, counter));
      failed = true;
    }
    for (int i = 1; i <= 1000; ++i) {
      if (instances.count(i) == 0) {
        ACE_ERROR((LM_ERROR, "ERROR: Reader Missing Instance %d\n", i));
        failed = true;
      }
    }
    ws->detach_condition(dr_rc);
    dr->delete_readcondition(dr_rc);
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass either --writer or --reader\n"));
    return 1;
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
