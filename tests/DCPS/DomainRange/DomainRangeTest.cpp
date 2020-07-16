#include "MessengerTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "tests/Utils/StatusMatching.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"

/*
  NOTE: The messages may not be processed by the reader in this test.
  This behavior is not an error.
*/

const int /*DOMAIN_ID = 100,*/ MSGS_PER_WRITER = 10;
const int TOTAL_WRITERS = 1, TOTAL_READERS = 1;

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class WriterTask : public ACE_Task_Base {
public:
  WriterTask (const std::string& writer_id,
              DDS::DataWriter_var writer,
              int total_readers)
    : writer_id_(writer_id)
    , writer_(writer)
    , total_readers_(total_readers)
  { }

  int svc()
  {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) WriterTask::svc - starting for writer_id: %C\n", writer_id_.c_str()));

    Messenger::MessageDataWriter_var message_writer =
      Messenger::MessageDataWriter::_narrow(writer_);

    if (!message_writer) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    // Block until Subscriber is available
    Utils::wait_match(writer_, total_readers_);

    // Write samples
    Messenger::Message message;
    message.from = writer_id_.c_str();
    message.subject = "Test Message";
    message.subject_id = 1;
    message.text = "Testing...";
    message.count = 0;
    for (int i = 0; i < MSGS_PER_WRITER; ++i) {
      DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
      ++message.count;

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: main() -")
                   ACE_TEXT(" write returned %d!\n"), error));
      }
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is waiting for acknowledgments\n", writer_id_.c_str()));
    DDS::Duration_t timeout = { 30, 0 };
    message_writer->wait_for_acknowledgments(timeout);
    // With static discovery, it's not an error for wait_for_acks to fail
    // since the peer process may have terminated before sending acks.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is done\n", writer_id_.c_str()));

    return 0;
  }

private:
  std::string writer_id_;
  DDS::DataWriter_var writer_;
  int total_readers_;
};

ACE_Thread_Mutex readers_done_lock;
ACE_Condition_Thread_Mutex readers_done_cond(readers_done_lock);
int readers_done = 0;

void reader_done_callback()
{
  ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
  ++readers_done;
  readers_done_cond.signal();
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::vector<DDS::DomainId_t> domains;

  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    const std::string reader_id = "Reader";
    const std::string writer_id = "Writer";

    int num_participants = 1;

    {
      // New scope.
      ACE_Arg_Shifter shifter (argc, argv);
      while (shifter.is_anything_left ()) {
        const ACE_TCHAR* x;
        x = shifter.get_the_parameter (ACE_TEXT("-domain"));
        if (x != NULL) {
          domains.push_back(ACE_OS::atoi(x));
          ACE_DEBUG((LM_DEBUG, "(%P|%t) main() - domain: %d added to test\n", ACE_OS::atoi(x)));
        }
        x = shifter.get_the_parameter (ACE_TEXT("-participants"));
        if (x != NULL) {
          int p = ACE_OS::atoi(x);
          if (p == 2) {
            num_participants = p;
          }
          ACE_DEBUG((LM_DEBUG, "(%P|%t) main() - using %d participants in each domain\n", p));
        }
        shifter.consume_arg ();
      }
    }

    for (std::vector<DDS::DomainId_t>::const_iterator it = domains.begin(); it != domains.end(); ++it)
    {
      DDS::DomainId_t domain = *it;

      // Create DomainParticipant
      DDS::DomainParticipantQos dr_dp_qos;
      dpf->get_default_participant_qos(dr_dp_qos);

      DDS::DomainParticipant_var sub_participant =
        dpf->create_participant(domain,
                                dr_dp_qos,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!sub_participant) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_participant failed!\n")),
                        -1);
      }

      // Register TypeSupport
      Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl();

      if (ts->register_type(sub_participant, "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" register_type failed!\n")),
                        -1);
      }

      // Create Topic
      CORBA::String_var type_name = ts->get_type_name();
      DDS::Topic_var topic =
          sub_participant->create_topic("TheTopic",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                        -1);
      }

      // Create Subscriber
      DDS::Subscriber_var subscriber =
          sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!subscriber) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_subscriber failed!\n")), -1);
      }

      DDS::DomainParticipant_var pub_participant;

      if (num_participants == 1) {
        // writer uses same participant
        pub_participant = sub_participant;
      } else {
        DDS::DomainParticipantQos dw_dp_qos;
        dpf->get_default_participant_qos(dw_dp_qos);

        pub_participant =
          dpf->create_participant(domain,
                                  dw_dp_qos,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!pub_participant) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_participant failed!\n")),
                          -1);
        }
      }

      // Register TypeSupport
      Messenger::MessageTypeSupport_var dw_ts =
        new Messenger::MessageTypeSupportImpl();

      if (dw_ts->register_type(pub_participant, "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() dw -")
                          ACE_TEXT(" register_type failed!\n")),
                        -1);
      }

      // Create Topic
      CORBA::String_var dw_type_name = dw_ts->get_type_name();
      DDS::Topic_var dw_topic =
          pub_participant->create_topic("TheTopic",
                                  dw_type_name,
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!dw_topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                        -1);
      }

      // Create Publisher
      DDS::Publisher_var publisher =
          pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!publisher) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_publisher failed!\n")),
                        -1);
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) Starting DataWriter %C\n", writer_id.c_str()));

      DDS::DataWriterQos dw_qos;
      publisher->get_default_datawriter_qos(dw_qos);

      // reliability settings
      dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time.sec     = 1;
      dw_qos.reliability.max_blocking_time.nanosec = 0;

      // Create DataWriter
      DDS::DataWriter_var writer =
        publisher->create_datawriter(dw_topic,
                                     dw_qos,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!writer) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_datawriter failed!\n")),
                        -1);
      }

      // Create DataReaders
      DDS::DataReaderListener_var listener(new DataReaderListenerImpl(reader_id, MSGS_PER_WRITER, reader_done_callback));

      DDS::DataReaderQos dr_qos;
      subscriber->get_default_datareader_qos(dr_qos);
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

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

      ACE_DEBUG((LM_DEBUG, "(%P|%t) Spawning writer task\n", writer_id.c_str()));
      WriterTask task(writer_id, writer, TOTAL_READERS);
      task.activate(DEFAULT_FLAGS, TOTAL_WRITERS);
      task.wait();

      ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
      while (readers_done != TOTAL_READERS)
        readers_done_cond.wait();

      DDS::Duration_t timeout = { 3, 0 };
      writer->wait_for_acknowledgments(timeout);

      // Clean-up!
      sub_participant->delete_contained_entities();
      dpf->delete_participant(sub_participant);

      if (num_participants == 2) {
        pub_participant->delete_contained_entities();
        dpf->delete_participant(pub_participant);
      }

      // reset for next domain
      readers_done = 0;

    }

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
