#include "DelayedDurableTypeSupportImpl.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/ace_wchar.h>

#include <fstream>
#include <set>

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

const unsigned large_sample_seq_size =
 static_cast<unsigned>(
    OpenDDS::DCPS::TransportSendStrategy::UDP_MAX_MESSAGE_SIZE);

int scale(int value, bool large_samples)
{
  return value * (large_samples ? 1 : 10);
}

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
  bool large_samples = false;
  bool has_early_reader = false;
  int argv_index_report_file = 0;

  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--writer")) {
      writer = true;
    } else if (arg == ACE_TEXT("--reader")) {
      reader = true;
    } else if (arg == ACE_TEXT("--large-samples")) {
      large_samples = true;
    } else if (arg == ACE_TEXT("--has-early-reader")) {
      has_early_reader = true;
    } else if (arg == ACE_TEXT("--report-last-value")) {
      argv_index_report_file = ++i;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  bool failed = false;
  const int num_instances = has_early_reader ? 1 : scale(100, large_samples);
  const char* const name = writer ? "writer" : "reader";
  ACE_DEBUG((LM_DEBUG, "(%P|%t) %C starting at %T\n", name));

  if (writer) {
    Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
    DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
                                               DEFAULT_STATUS_MASK);
    PropertyDataWriter_var pdw = PropertyDataWriter::_narrow(dw);

    if (has_early_reader) {
      Utils::wait_match(dw, 1);
    }

    Property p;
    if (large_samples) {
      p.extra.length(large_sample_seq_size);
      for (unsigned i = 0; i < large_sample_seq_size; ++i) {
        p.extra[i] = i % 256;
      }
    } else {
      p.extra.length(0);
    }

    for (int i = 1; i <= num_instances; ++i) {
      p.key = i;
      p.value = i;
      pdw->write(p, HANDLE_NIL);
    }

    for (int c = 1; c < 75; ++c) {
      for (int i = 1; i <= num_instances; i += scale(5, large_samples)) {
        p.key = i;
        p.value = i + c;
        pdw->write(p, HANDLE_NIL);
      }
      if (!has_early_reader) {
        ACE_OS::sleep(ACE_Time_Value(0, 500 * 1000)); // 1/2 sec
      }
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "writer: Count: %d\n", c));
      }
    }

    DDS::Duration_t duration = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    pdw->wait_for_acknowledgments(duration);

    if (has_early_reader) {
      Utils::wait_match(dw, 2);
    }

  } else if (reader) {
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
    const unsigned minimum_sample_count = has_early_reader ? 1 : large_samples ? 95 : 981;
    std::set<int> instances;
    int last_value = 0;
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
          last_value = data[i].value;
          if (verbose) {
            ACE_DEBUG((LM_DEBUG, "reader: Counter: %u Instance: %d Value: %d\n", counter, data[i].key, data[i].value));
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
    for (int i = 1; i <= num_instances; ++i) {
      if (instances.count(i) == 0) {
        ACE_ERROR((LM_ERROR, "ERROR: Reader Missing Instance %d\n", i));
        failed = true;
      }
    }
    ws->detach_condition(dr_rc);
    dr->delete_readcondition(dr_rc);

    if (argv_index_report_file) {
      const OpenDDS::DCPS::String filename = ACE_TEXT_ALWAYS_CHAR(argv[argv_index_report_file]);
      std::ofstream file(filename.c_str());
      file << last_value;
    }

    if (has_early_reader) {
      Utils::wait_match(dr, 0);
    }

  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass either --writer or --reader\n"));
    return 1;
  }
  ACE_DEBUG((LM_DEBUG, "(%P|%t) %C ending at %T\n", name));

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
