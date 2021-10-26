/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "InternalThreadStatusListenerImpl.h"
#include "MessengerTypeSupportImpl.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif
#include <dds/DCPS/DCPS_Utils.h>

#include <ace/Get_Opt.h>

#include <iostream>

const DDS::Duration_t max_wait_time = {10, 0};

bool wait_for_samples(DDS::DataReader_var reader, bool disposed) {
  DDS::ReadCondition_var read_condition = reader->create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
    disposed ? DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE : DDS::ALIVE_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(read_condition);
  DDS::ConditionSeq active;
  DDS::ReturnCode_t rc = ws->wait(active, max_wait_time);
  ws->detach_condition(read_condition);
  reader->delete_readcondition(read_condition);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l wait_for_samples() ERROR: ")
      ACE_TEXT("wait failed: %C\n"),
      OpenDDS::DCPS::retcode_to_string(rc)));
    return false;
  }
  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;

  try {
    std::cout << "Starting publisher" << std::endl;

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

    // Create Publisher
    DDS::PublisherQos publisher_qos;
    participant->get_default_publisher_qos(publisher_qos);
    publisher_qos.partition.name.length(1);
    publisher_qos.partition.name[0] = "OCI";

    DDS::Publisher_var pub =
      participant->create_publisher(publisher_qos,
                                    DDS::PublisherListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    std::cout << "Reliable DataWriter" << std::endl;
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

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
                       EXIT_FAILURE);
    }

    Messenger::MessageDataWriter_var message_writer =
      Messenger::MessageDataWriter::_narrow(dw);

    // Get the Built-In Subscriber for Built-In Topics
    DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();

    DDS::DataReader_var thread_reader =
      bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
    if (!thread_reader) {
      std::cerr << "Could not get " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC
                << " DataReader." << std::endl;
      return EXIT_FAILURE;
    }

    DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();
    InternalThreadStatusListenerImpl* listener = new InternalThreadStatusListenerImpl("Publisher", dcs);
    DDS::DataReaderListener_var listener_var(listener);

    const DDS::ReturnCode_t retcode =
      thread_reader->set_listener(listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      std::cerr << "set_listener for " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC << " failed." << std::endl;
      return EXIT_FAILURE;
    }

    // wait for subscriber
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter is waiting for subscriber\n"));
    Utils::wait_match(dw, 1);

    // write samples
    Messenger::Message message;
    message.from = "Publisher";
    message.subject = "Test Message";
    message.subject_id = 1;
    message.text = "Testing...";
    message.count = 0;

    for (int i = 0; i < 10; ++i) {
      DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
      ++message.count;

      std::cout << "Publisher sending message " << i << std::endl;

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: main() -")
                   ACE_TEXT(" write returned %d!\n"), error));
      }
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter is waiting for acknowledgments\n"));
    message_writer->wait_for_acknowledgments(max_wait_time);
    // With static discovery, it's not an error for wait_for_acks to fail
    // since the peer process may have terminated before sending acks.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter is done\n"));

    // wait for internal thread status reports
    ACE_OS::sleep(3);

    int count = listener->get_count();
    if (count < 3 ) {
      // in 3 seconds with multiple threads, there should be more than 3 status reports.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: Publisher did not receive expected internal thread status reports.")));
      return EXIT_FAILURE;
    } else {
      std::cout << "Publisher received " << count << " internal thread status messages." << std::endl;
    }

    // Create 2nd participant to witness disposes of the 1st participant's threads.
    DDS::DomainParticipant_var part2 = dpf->create_participant(
      42, part_qos, DDS::DomainParticipantListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!part2) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant (2nd) failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::Subscriber_var bit_subscriber2 = part2->get_builtin_subscriber();
    DDS::DataReader_var thread_reader2 =
      bit_subscriber2->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
    if (!thread_reader2) {
      std::cerr << "ERROR: Could not get 2nd " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC
                << " DataReader." << std::endl;
      return EXIT_FAILURE;
    }

    InternalThreadStatusListenerImpl* listener2 =
      new InternalThreadStatusListenerImpl("Publisher part2", dcs);
    DDS::DataReaderListener_var listener2_var(listener2);
    if (thread_reader2->set_listener(listener2, OpenDDS::DCPS::DEFAULT_STATUS_MASK) != DDS::RETCODE_OK) {
      std::cerr << "ERROR: set_listener for " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC
        << " failed." << std::endl;
      return EXIT_FAILURE;
    }

    // Wait for valid thread status to come into the 2nd thread reader
    if (!wait_for_samples(thread_reader2, false)) {
      ACE_ERROR((LM_ERROR, "ERROR: wait for part2 thread status failed\n"));
      return EXIT_FAILURE;
    }
    if (listener->disposes() > 0 || listener2->disposes() > 0) {
      ACE_ERROR((LM_ERROR, "ERROR: thread listener has disposes before participant deletes\n"));
      return EXIT_FAILURE;
    }

    // Clean-up first participant
    std::cerr << "publisher deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "publisher deleting participant" << std::endl;
    dpf->delete_participant(participant.in());



    // Wait for disposes to come in to 2nd thread reader
    if (!wait_for_samples(thread_reader2, true)) {
      ACE_ERROR((LM_ERROR, "ERROR: wait for part2 thread status disposes failed\n"));
      status = EXIT_FAILURE;
    }

    dcs->wait_for("DRIVER", listener2->id(), DISPOSE_RECEIVED);

    // Finish cleaning up
    dpf->delete_participant(part2);
    std::cerr << "publisher shutdown" << std::endl;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
