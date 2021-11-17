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

#include <ace/Arg_Shifter.h>
#include <ace/Semaphore.h>
#include <ace/Thread_Semaphore.h>

#include <iostream>
#include <sstream>
namespace OpenDDS {
namespace DCPS {
long num_samples = 0;
long num_seconds = 0;
// parse the command line arguments
int parse_args (int& argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -samples num_samples       defaults to infinite
    //  -time num_seconds          defaults to infinite

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-samples"))) != 0) {
      num_samples = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-time"))) != 0) {
      num_seconds = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

class TestRecorderListener : public RecorderListener
{
public:
  explicit TestRecorderListener()
    : sem_(0)
    , sample_count_(0)
  {
  }

  virtual void on_sample_data_received(Recorder* rec,
                                       const RawDataSample& sample)
  {
    using namespace DCPS;
    XTypes::DynamicData dd = rec->get_dynamic_data(sample);
    String my_type = "";
    if (!print_dynamic_data(dd, my_type, "")){
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TestRecorderListener::on_sample_data_received: "
        "Failed to read dynamic data\n"));
    }
    std::cout << my_type;
    ++sample_count_;
  }

  virtual void on_recorder_matched(Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status )
  {
    if (status.current_count == 1 && DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestRecorderListener::on_recorder_matched:"
          " a writer connected to recorder\n")));
    } else if (status.current_count == 0 && status.total_count > 0) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestRecorderListener::on_recorder_matched:"
          " a writer disconnected with recorder\n")));
      }
      sem_.release();
    }
  }

  int wait(const ACE_Time_Value & tv)
  {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }

  long sample_count() const
  {
    return sample_count_;
  }
private:
  ACE_Thread_Semaphore sem_;
  long sample_count_;
};


int run_test(int argc, ACE_TCHAR *argv[]){
  int ret_val = 0;
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    parse_args(argc, argv);
    if (argc != 4) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: main():"
                   " dynamic-data-monitor was passed an incorrect number of arguments:"
                   " expected 3, received %d\n"
                   "Format should be ./dynamic-data-monitor [topic name] [type name] [domain id]\n", argc-1));
      }
      return 1;
    }
    const ACE_TCHAR* topic_name = argv[1];
    const ACE_TCHAR* type_name = argv[2];
    DDS::DomainId_t domainid = ACE_OS::atoi(argv[3]);
    Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(domainid,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    if (!participant) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant failed!\n"));
      }
      return 1;
    }
    using namespace DCPS;

    {
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       ACE_TEXT_ALWAYS_CHAR(topic_name),
                                       ACE_TEXT_ALWAYS_CHAR(type_name),
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       DEFAULT_STATUS_MASK);

      if (!topic) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic failed!\n"));
        }
        return 1;
      }

      RcHandle<TestRecorderListener> recorder_listener = make_rch<TestRecorderListener> ();

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();
      dr_qos.representation.value.length(1);
      dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
      // Create Recorder
      Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);

      if (!recorder.in()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_recorder failed!\n"));
        }
        return 1;
      }

      ACE_Time_Value tv;
      ACE_Time_Value start_tv = tv.now();

      while (1) {
        // TODO CLAYTON: Find a better waiting pattern
        sleep(1);
        if (num_seconds != 0 && (tv.now() - start_tv).sec() >= num_seconds) {
          break;
        }
        if (num_samples != 0 && recorder_listener->sample_count() >= num_samples) {
          break;
        }
      }
      service->delete_recorder(recorder);
    }
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
  if (ret_val == 1) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): failed to properly analyze sample!\n"));
    }
    return 1;
  }
  return ret_val;
}

} // namespace DCPS
} // namespace OpenDDS

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = OpenDDS::DCPS::run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
