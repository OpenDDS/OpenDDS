/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "tests/DCPS/LargeSample/MessengerTypeSupportImpl.h"
#include "Options.h"
#include "Writer.h"

#include <sstream>
#include <iomanip>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  bool status = false;
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

    const Options options(argc, argv);
    // Create DomainParticipant
    typedef std::vector<DDS::DomainParticipant_var> Participants;
    Participants participants(options.num_pub_participants);
    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var mts =
      new Messenger::MessageTypeSupportImpl();
    CORBA::String_var type_name = mts->get_type_name();
    std::vector<WriterSample> writers;
    std::ostringstream pid;
    pid << mypid;
    WriterSample ws;
    ws.message.process_id = pid.str().c_str();
    ws.message.from       = "Comic Book Guy";
    ws.message.text       = "Worst. Movie. Ever.";
    ws.message.participant_id = 0;
    ws.message.sample_id  = 0;
    ws.message.data.length(options.sample_size);
    for (CORBA::ULong j = 0; j < ws.message.data.length(); ++j) {
      ws.message.data[j] = j % 256;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Created dpf for process=%C\n"), pid.str().c_str()));
    int part_num = 0;
    for (Participants::iterator part = participants.begin();
         part != participants.end();
         ++part, ++ws.message.participant_id, ++part_num) {
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

      if (generated_config) {
        std::stringstream domain_config_stream;
        std::string config_name = "domain_part_";
        domain_config_stream << config_name << part_num;
        OPENDDS_STRING config;
        config = domain_config_stream.str().c_str();
        TheTransportRegistry->bind_config(config, *part);
      }

      if (mts->register_type(part->in(), "") != DDS::RETCODE_OK) {
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

      // Create Publisher
      DDS::Publisher_var pub =
        (*part)->create_publisher(PUBLISHER_QOS_DEFAULT,
                                  DDS::PublisherListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         -1);
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
      qos.liveliness.lease_duration.sec = 5;
      qos.liveliness.lease_duration.nanosec = 0;
      qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

      ws.message.writer_id = 0;
      for (unsigned int writer = 0; writer < options.num_writers; ++writer, ++ws.message.writer_id) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Creating writer\n")));

        // Create DataWriter
        ws.writer =
          pub->create_datawriter(topic.in(),
                                 qos,
                                 DDS::DataWriterListener::_nil(),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(ws.writer.in())) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l: main()")
                            ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                           -1);
        }

        writers.push_back(ws);
      }
    }


    {
      Writer writer(options, writers);
      status = writer.write();
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Writers Done\n")));

    for (Participants::iterator part = participants.begin();
         part != participants.end();
         ++part, ++ws.message.participant_id) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Cleanup Participant\n")));
      // Clean-up!
      (*part)->delete_contained_entities();
      dpf->delete_participant(part->in());
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Publisher shutting down\n")));

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_DEBUG((LM_ERROR, "Transport exception caught in publisher main\n"));
    return -1;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) Publisher exiting\n")));
  return (status ? 0 : -1);
}
