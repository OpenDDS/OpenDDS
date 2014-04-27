/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/udp/UdpInst_rch.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "DataReaderListener.h"
#include "Options.h"
#include "tests/DCPS/LargeSample/MessengerTypeSupportImpl.h"
#include <cstdlib>

namespace {

} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  bool ok = true;
  try {
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

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Created dpf\n")));

    for (Participants::iterator part = participants.begin();
         part != participants.end();
         ++part) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Creating participant\n")));

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
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Creating reader\n")));

        // Create DataReader
        listener_servants.push_back(new DataReaderListenerImpl(options));
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

    const ACE_Time_Value sleep_delay(options.delay_ms / 1000,
                                     (options.delay_ms % 1000) * 1000);
    unsigned int delay = 0;
    // writer uses above delay before sending every sample, so take the number of samples
    // altogether sent (per process) and then double it to ensure we give a chance for all
    // the data to be received.
    const unsigned int MAX_DELAY = options.num_pub_participants * options.num_writers *
      options.num_samples * options.delay_ms * 2;
    while (delay < MAX_DELAY) {
      bool complete = true;
      for (ListenerServants::const_iterator listener = listener_servants.begin();
           listener != listener_servants.end();
           ++listener) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) listener done=%d\n"), (*listener)->done()));
        if (!(*listener)->done()) {
          complete = false;
        }
      }

      if (complete)
        break;

      delay += options.delay_ms;
      ACE_OS::sleep(sleep_delay);
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Listeners done %d\n"), delay));

    if (delay == MAX_DELAY) {
      if (options.reliable) {
        for (ListenerServants::const_iterator listener = listener_servants.begin();
             listener != listener_servants.end();
             ++listener) {
          (*listener)->report_errors();
        }
        ok = false;
      }
      else {
        std::cout << "WARNING: un-reliable data loss\n";
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
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
