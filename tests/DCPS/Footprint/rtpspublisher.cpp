/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "Args.h"

bool dw_reliable() {
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {

    std::cout << "Starting publisher" << std::endl;
    {
      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      std::cout << "Starting publisher with " << argc << " args" << std::endl;
      int error;
      if ((error = parse_args(argc, argv)) != 0) {
        return error;
      }

      // Create DomainParticipant
      participant = dpf->create_participant(4,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_participant failed!\n")),
                         -1);
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                         -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         -1);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
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
      if (dw_reliable()) {
        std::cout << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               DDS::DataWriterListener::_nil(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         -1);
      }

      // Start writing threads
      std::cout << "Creating Writer" << std::endl;
      Writer* writer = new Writer(dw.in());
      std::cout << "Starting Writer" << std::endl;
      writer->start();

      while (!writer->is_finished()) {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "Writer finished " << std::endl;
      writer->end();

      if (wait_for_acks) {
        std::cout << "Writer wait for ACKS" << std::endl;

        DDS::Duration_t timeout =
          { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
        dw->wait_for_acknowledgments(timeout);
      } else {
        // let any missed multicast/rtps messages get re-delivered
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "deleting DW" << std::endl;
      delete writer;
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
