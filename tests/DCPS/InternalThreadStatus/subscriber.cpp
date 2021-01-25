/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessengerTypeSupportImpl.h"
#include "InternalThreadStatusListenerImpl.h"
#include "DataReaderListenerImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include <ace/Get_Opt.h>

#include <iostream>

ACE_Thread_Mutex reader_done_lock;
ACE_Condition_Thread_Mutex reader_done_cond(reader_done_lock);

void reader_done_callback()
{
  ACE_Guard<ACE_Thread_Mutex> g(reader_done_lock);
  reader_done_cond.signal();
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;

  try {
    std::cout << "Starting subscriber" << std::endl;

    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant = dpf->create_participant(42,
                                                                     part_qos,
                                                                     DDS::DomainParticipantListener::_nil(),
                                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       EXIT_FAILURE);
    }

    // Register TypeSupport (Messenger::Message)
    Messenger::MessageTypeSupport_var mts =
      new Messenger::MessageTypeSupportImpl();

    if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       EXIT_FAILURE);
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
                       EXIT_FAILURE);
    }

    // Create Subscriber
    DDS::SubscriberQos subscriber_qos;
    participant->get_default_subscriber_qos(subscriber_qos);
    subscriber_qos.partition.name.length(1);
    subscriber_qos.partition.name[0] = "OCI";

    DDS::Subscriber_var sub =
      participant->create_subscriber(subscriber_qos,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_subscriber failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::DataReaderQos qos;
    sub->get_default_datareader_qos(qos);
    std::cout << "Reliable DataReader" << std::endl;
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    // Create DataReaders
    DDS::DataReaderListener_var dr_listener(new DataReaderListenerImpl("Subscriber", 10, reader_done_callback));

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
      sub->create_datareader(topic,
                             dr_qos,
                             dr_listener,
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

    // wait for message reader
    ACE_Guard<ACE_Thread_Mutex> g(reader_done_lock);
    reader_done_cond.wait();

    // give publsher time to receive acknowledgements
    ACE_OS::sleep(3);

    // Clean-up!
    std::cerr << "subscriber deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "subscriber deleting participant" << std::endl;
    dpf->delete_participant(participant.in());
    std::cerr << "subscriber shutdown" << std::endl;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
