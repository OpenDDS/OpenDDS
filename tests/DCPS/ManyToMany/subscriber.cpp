/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include "dds/DCPS/transport/framework/TransportInst_rch.h"
#if defined (sun)
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/udp/UdpInst_rch.h"
#endif

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "DataReaderListener.h"
#include "Options.h"
#include "tests/DCPS/LargeSample/MessengerTypeSupportImpl.h"
#include <cstdlib>
#include <sstream>
#include <iomanip>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  bool ok = true;
  bool generated_config = false;
  int mypid = ACE_OS::getpid();
  try {
    //Look to see if the config file (.ini) was generated
    //for rtps participant processing
    for(int i = 0; i < argc; ++i) {
      if(ACE_OS::strstr(argv[i], ACE_TEXT("generated"))) {
        generated_config = true;
      } else if (0 == ACE_OS::strcmp(ACE_TEXT("-p"), argv[i]) && i < argc - 1) {
        mypid = ACE_OS::atoi(argv[i + 1]);
      }
    }
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // handle test performance issue on one platform
#if defined (sun)
    const char* udpTransName = "udp";
    OpenDDS::DCPS::TransportInst_rch inst = OpenDDS::DCPS::TransportRegistry::instance()->get_inst(udpTransName);
    if (inst != 0) {
      OpenDDS::DCPS::UdpInst_rch udp_inst = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::UdpInst>(inst);
      if (udp_inst == 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: retrieving transport config for: %C failed!\n"),
                          udpTransName), -1);
      }
      udp_inst->rcv_buffer_size_ = 0x40000;
    }
#endif

    const Options options(argc, argv);
    // Create DomainParticipant
    typedef std::vector<DDS::DomainParticipant_var> Participants;
    Participants participants(options.num_sub_participants);
    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
      new Messenger::MessageTypeSupportImpl();
    CORBA::String_var type_name = ts->get_type_name();
    typedef std::vector<DataReaderListenerImpl*> ListenerServants;
    ListenerServants listener_servants;
    std::vector<DDS::DataReaderListener_var> listeners;
    std::stringstream ss;
    ss << std::setw(5) << mypid;

    const std::string pid = ss.str();

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Created dpf\n")));

    unsigned int part_index = 0;
    for (Participants::iterator part = participants.begin();
         part != participants.end();
         ++part, ++part_index) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Creating participant\n")));

      *part =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(part->in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_participant() failed!\n")), -1);
      }

      if (generated_config) {
        std::stringstream domain_config_stream;
        std::string config_name = "domain_part_";
        domain_config_stream << config_name << part_index;
        OPENDDS_STRING config;
        config = domain_config_stream.str().c_str();
        TheTransportRegistry->bind_config(config, *part);
      }

      if (ts->register_type(part->in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
      }

      // Create Topic (Movie Discussion List)
      DDS::Topic_var topic =
        (*part)->create_topic("Movie Discussion List",
                              type_name.in(),
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
      }

      // Create Subscriber
      DDS::Subscriber_var sub =
        (*part)->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(sub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_subscriber() failed!\n")), -1);
      }

      DDS::DataReaderQos qos;
      sub->get_default_datareader_qos(qos);
      qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
      qos.liveliness.lease_duration.sec = 10;
      qos.liveliness.lease_duration.nanosec = 0;
      qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

      if (options.reliable) {
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      for (unsigned int reader = 0; reader < options.num_readers; ++reader) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Creating reader\n")));

        // Create DataReader
        listener_servants.push_back(new DataReaderListenerImpl(options, pid, part_index, reader));
        listeners.push_back(DDS::DataReaderListener_var(listener_servants.back()));

        DDS::DataReader_var data_reader =
          sub->create_datareader(topic.in(),
                                 qos,
                                 listeners.back().in(),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(data_reader.in())) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
        }
      }
    }

    const unsigned int sleep_delay_msec = 500;
    unsigned int delay = 0;
    while (delay < options.total_duration_msec) {
      bool complete = true;
      for (ListenerServants::const_iterator listener = listener_servants.begin();
           listener != listener_servants.end();
           ++listener) {
        if (!(*listener)->done()) {
          complete = false;
        }
      }

      if (complete)
        break;

      delay += sleep_delay_msec;
      ACE_OS::sleep(ACE_Time_Value(0, sleep_delay_msec * 1000));
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Listeners done (ran for %d msec)\n"), delay));

    if (delay >= options.total_duration_msec) {
      for (ListenerServants::const_iterator listener = listener_servants.begin();
           listener != listener_servants.end();
           ++listener) {
        (*listener)->report_errors();
      }

      if (options.reliable) {
        ok = false;
      }
    }

    // Clean-up!
    for (Participants::iterator part = participants.begin();
         part != participants.end();
         ++part) {
      (*part)->delete_contained_entities();
      dpf->delete_participant(*part);
    }

    TheServiceParticipant->shutdown();
    ACE_Thread_Manager::instance()->wait();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_DEBUG((LM_ERROR, "Transport exception caught in subscriber main\n"));
    return -1;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Subscriber exiting\n")));
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
