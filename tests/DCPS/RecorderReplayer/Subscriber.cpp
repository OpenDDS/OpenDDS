/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

 #include <dds/DdsDcpsInfrastructureC.h>
 #include <dds/DCPS/Marked_Default_Qos.h>
 #include <dds/DCPS/Service_Participant.h>
 #include <dds/DCPS/SubscriberImpl.h>
 #include <dds/DCPS/WaitSet.h>

 #include "dds/DCPS/StaticIncludes.h"
 #if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
 #include <dds/DCPS/transport/udp/Udp.h>
 #include <dds/DCPS/transport/multicast/Multicast.h>
 #include <dds/DCPS/RTPS/RtpsDiscovery.h>
 #include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
 #include <dds/DCPS/transport/shmem/Shmem.h>
 #endif

 #include <dds/DCPS/transport/framework/TransportRegistry.h>
 #include <dds/DCPS/transport/framework/TransportConfig.h>
 #include <dds/DCPS/transport/framework/TransportInst.h>

 #include "DataReaderListenerImpl.h"
 #include "MessengerTypeSupportImpl.h"
 #include "Args.h"

 bool
 make_dr_reliable()
 {
   OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
   return gc->instances_[0]->name() == "the_rtps_transport";
 }

 int
 ACE_TMAIN(int argc, ACE_TCHAR *argv[])
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
                        ACE_TEXT(" create_participant failed!\n")), -1);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Start subscriber\n"));

    {
      // Register Type (Messenger::Message)
      Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl;

      if (ts->register_type(participant, "Messenger") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" register_type failed!\n")), -1);
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
                          ACE_TEXT(" create_topic failed!\n")), -1);
      }

      // setup partition
      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);
      DDS::StringSeq my_partition;
      my_partition.length(1);
      my_partition[0] = "Two";
      sub_qos.partition.name = my_partition;

      // Create Subscriber
      DDS::Subscriber_var subscriber =
        participant->create_subscriber(sub_qos,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!subscriber) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_subscriber failed!\n")), -1);
      }

      // Create DataReader
      DDS::DataReaderListener_var listener(new DataReaderListenerImpl);

      DDS::DataReaderQos dr_qos;
      subscriber->get_default_datareader_qos(dr_qos);
      if (make_dr_reliable()) {
        dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      DDS::DataReader_var reader =
        subscriber->create_datareader(topic,
                               dr_qos,
                               listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!reader) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_datareader failed!\n")), -1);
      }

      Messenger::MessageDataReader_var reader_i =
        Messenger::MessageDataReader::_narrow(reader);

      if (!reader_i) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
      }

      // Block until Publisher completes
      DDS::StatusCondition_var condition = reader->get_statuscondition();
      condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition);

      while (true) {
        DDS::SubscriptionMatchedStatus matches;
        if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" get_subscription_matched_status failed!\n")), -1);
        }

        if (matches.current_count == 0 && matches.total_count > 0) {
          break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = { 60, 0 };
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" wait failed!\n")), -1);
        }
      }

      ws->detach_condition(condition);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Stop subscriber\n"));

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Subscriber exiting\n"));

  return 0;
}
