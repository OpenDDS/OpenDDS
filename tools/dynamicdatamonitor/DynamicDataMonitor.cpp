/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DCPS/Recorder.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/DynamicData.h>
#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include <ace/Semaphore.h>
#include <ace/Thread_Semaphore.h>

#include <iostream>
#include <sstream>

class TestRecorderListener : public OpenDDS::DCPS::RecorderListener
{
public:
  explicit TestRecorderListener()
    : sem_(0)
  {
  }

  virtual void on_sample_data_received(OpenDDS::DCPS::Recorder* rec,
                                       const OpenDDS::DCPS::RawDataSample& sample)
  {
    using namespace OpenDDS::DCPS;
    OpenDDS::XTypes::DynamicData dd = rec->get_dynamic_data(sample);
    OpenDDS::DCPS::String my_type = "";
    if (!print_dynamic_data(dd, dd.get_type(), my_type, "")){
      ACE_ERROR((LM_ERROR, "Error: (%P|%t) on_sample_data_received Failed to read dynamic data\n"));
    }
    std::cout << my_type;
  }

  virtual void on_recorder_matched(OpenDDS::DCPS::Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status )
  {
    if (status.current_count == 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("TestRecorderListener -- a writer connect to recorder\n")));
    }
    else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("TestRecorderListener -- writer disconnect with recorder\n")));
      sem_.release();
    }
  }

  int wait(const ACE_Time_Value & tv) {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }
private:
  ACE_Thread_Semaphore sem_;
};


int run_test(int argc, ACE_TCHAR *argv[]){
  int ret_val = 0;
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    if (argc != 4) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" dynamic-data-monitor was passed an incorrect number of arguments: ")
                        ACE_TEXT("expected 4, received %d\n")
                        ACE_TEXT("Format should be ./dynamic-data-monitor [topic name] [type name] [domain id] [-DCPSConfigFile *.ini]\n"), argc),
                        1);
    }
    OpenDDS::DCPS::String topic_name = argv[1];
    OpenDDS::DCPS::String type_name = argv[2];
    DDS::DomainId_t domainid = ACE_OS::atoi(argv[3]);
    OpenDDS::DCPS::Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(domainid,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                        1);
    }
    using namespace OpenDDS::DCPS;

    {
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       topic_name.c_str(),
                                       type_name.c_str(),
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                          1);
      }

      // ACE_Time_Value wait_time(60, 0);

      RcHandle<TestRecorderListener> recorder_listener = make_rch<TestRecorderListener> ();

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();
      dr_qos.representation.value.length(1);
      dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
      // Create Recorder
      OpenDDS::DCPS::Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);

      if (!recorder.in()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_recorder failed!\n")),
                          1);
      }

      int i;
      while (1) {
        ++i;
      }
      // wait until the writer disconnnects
      // if (recorder_listener->wait(wait_time) == -1) {
      //   ACE_ERROR_RETURN((LM_ERROR,
      //                     ACE_TEXT("ERROR: %N:%l: main() -")
      //                     ACE_TEXT(" recorder timeout!\n")),
      //                     1);
      // }
      // ret_val = recorder_listener->ret_val_;
      // service->delete_recorder(recorder);
    }
    // participant->delete_contained_entities();
    // dpf->delete_participant(participant);
    // TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
  if (ret_val == 1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" failed to properly analyze sample!\n")),
                      1);
  }
  return ret_val;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
