#include "DelayedDurableTypeSupportImpl.h"

#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/WaitSet.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

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

  const bool verbose = argc > 2 && ACE_TString(argv[2]) == ACE_TEXT("-verbose");

  if (argc > 1 && ACE_TString(argv[1]) == ACE_TEXT("-writer")) {
    Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
    DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
                                               DEFAULT_STATUS_MASK);
    PropertyDataWriter_var pdw = PropertyDataWriter::_narrow(dw);
    for (int i = 1; i <= 1000; ++i) {
      Property p = {i, i};
      pdw->write(p, HANDLE_NIL);
    }

    for (int c = 1; c < 75; ++c) {
      for (int i = 1; i <= 1000; i += 50) {
        Property p = {i, i + c};
        pdw->write(p, HANDLE_NIL);
      }
      ACE_OS::sleep(ACE_Time_Value(0, 500 * 1000)); // 1/2 sec
      if (verbose) {
        ACE_DEBUG((LM_DEBUG, "Count: %d\n", c));
      }
    }

  } else if (argc > 1 && ACE_TString(argv[1]) == ACE_TEXT("-reader")) {
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
    int counter = 0;
    ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));
    bool done = false;
    while (!done) {
      ConditionSeq active;
      Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
      ReturnCode_t ret = ws->wait(active, infinite);
      if (ret != RETCODE_OK) {
        return 1;
      }
      while (!done) {
        ::PropertySeq data;
        SampleInfoSeq info;
        ret = pdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_rc);
        if (ret == RETCODE_NO_DATA) {
          break;
        }
        if (ret != RETCODE_OK) {
          return 1;
        }
        for (unsigned int i = 0; i < data.length(); ++i) {
          ++counter;
          if (verbose) {
            ACE_DEBUG((LM_DEBUG, "Counter: %d Instance: %d\n", counter, data[i].key));
          }

          if (counter == 981) {
            ACE_DEBUG((LM_DEBUG, "Counter 981 at %T\n"));
            done = true;
          }
        }
      }
    }
    ws->detach_condition(dr_rc);
    dr->delete_readcondition(dr_rc);
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return 0;
}
