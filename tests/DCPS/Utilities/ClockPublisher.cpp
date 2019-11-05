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

#include "dds/DCPS/StaticIncludes.h"

#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/transport/udp/Udp.h>
#  include <dds/DCPS/transport/multicast/Multicast.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/shmem/Shmem.h>
# endif
# include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "UtilitiesTypeSupportImpl.h"

#include <iostream>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant = dpf->create_participant(Utilities::DOMAIN,
                                                                     part_qos,
                                                                     DDS::DomainParticipantListener::_nil(),
                                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport (Messenger::Message)
    Utilities::TimeTypeSupport_var mts =
      new Utilities::TimeTypeSupportImpl();

    if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       -1);
    }

    // Create Topic
    CORBA::String_var type_name = mts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Time",
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
    DDS::PublisherQos publisher_qos;
    participant->get_default_publisher_qos(publisher_qos);

    DDS::Publisher_var pub =
      participant->create_publisher(publisher_qos,
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

    // Write samples
    Utilities::TimeDataWriter_var message_dw
      = Utilities::TimeDataWriter::_narrow(dw.in());

    if (CORBA::is_nil(message_dw.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    srand(time(0));

    Utilities::Time message;
    message.instance(rand());
    for (;;) {
      message.seconds(time(0));
      std::cout << "instance = " << message.instance() << " seconds = " << message.seconds() << std::endl;
      DDS::ReturnCode_t error = message_dw->write(message, DDS::HANDLE_NIL);
      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));
      }
      ACE_OS::sleep(1);
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
