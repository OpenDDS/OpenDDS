/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Args.h"

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
 try {
     // Initialize DomainParticipantFactory
     DDS::DomainParticipantFactory_var dpf =
       TheParticipantFactoryWithArgs(argc, argv);

     int error;
     if ((error = parse_args(argc, argv)) != 0) {
       return error;
     }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Start publisher\n"));

    {
      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl;

      if (ts->register_type(participant, "Messenger") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" register_type failed!\n")),
                         -1);
      }

      // Create Topic (Movie Discussion List)
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  "Messenger",
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                         -1);
      }

      // setup partition
      DDS::PublisherQos pub_qos;
      participant->get_default_publisher_qos(pub_qos);

      DDS::StringSeq my_partition;
      my_partition.length(1);
      my_partition[0] = "One";
      pub_qos.partition.name = my_partition;

      // Create Publisher
      DDS::Publisher_var publisher =
        participant->create_publisher(pub_qos,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!publisher) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_publisher failed!\n")),
                         -1);
      }

      // Create DataWriter
      DDS::DataWriter_var writer =
        publisher->create_datawriter(topic,
                                     DATAWRITER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!writer) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_datawriter failed!\n")),
                         -1);
      }

      Messenger::MessageDataWriter_var message_writer =
        Messenger::MessageDataWriter::_narrow(writer);

      if (!message_writer) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
      }

      // Block until Subscriber is available
      DDS::StatusCondition_var condition = writer->get_statuscondition();
      condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition);

      while (true) {
        DDS::PublicationMatchedStatus matches;
        if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" get_publication_matched_status failed!\n")),
                           -1);
        }

        if (matches.current_count >= 1) {
          break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = { 60, 0 };
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" wait failed!\n")),
                           -1);
        }
      }

      ws->detach_condition(condition);

      // Write samples
      Messenger::Message message;
      message.subject_id = 99;

      message.from       = "Comic Book Guy";
      message.subject    = "Review";
      message.text       = "Worst. Movie. Ever.";
      message.count      = 0;

      for (int i = 0; i < 10; ++i) {
        DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
        ++message.count;
        ++message.subject_id;

        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: %N:%l: main() -")
                     ACE_TEXT(" write returned %d!\n"), error));
        }
      }
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Stop publisher\n"));

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Publisher exiting\n"));

  return 0;
}
