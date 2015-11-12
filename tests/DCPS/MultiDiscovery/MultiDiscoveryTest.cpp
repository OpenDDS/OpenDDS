#include "TestMsgTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DCPS/GuidConverter.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"

/*
  NOTE:  The messages may not be processed by the reader in this test.
  This behavior is not an error.
*/

const int /*DOMAIN_ID = 100,*/ MSGS_PER_WRITER = 10;
const int TOTAL_WRITERS = 1, TOTAL_READERS = 1;

unsigned char hextobyte(unsigned char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + c - 'a';
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + c - 'A';
  }
  return c;
}

unsigned char
fromhex(const std::string& x,
        size_t idx)
{
  return (hextobyte(x[idx * 2]) << 4) | (hextobyte(x[idx * 2 + 1]));
}

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class WriterTask : public ACE_Task_Base {
public:
  WriterTask (std::string& writer_id,
              DDS::DataWriter_var writer,
              int total_readers)
    : writer_id_(writer_id)
    , writer_(writer)
    , total_readers_(total_readers)
  { }

  int svc()
  {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) WriterTask::svc - starting for writer_id: %C\n", writer_id_.c_str()));

    TestMsgDataWriter_var message_writer =
      TestMsgDataWriter::_narrow(writer_);

    if (!message_writer) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer_->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true) {
      DDS::PublicationMatchedStatus matches;
      if (writer_->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_publication_matched_status failed!\n")),
                         -1);
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C has %d of %d readers\n", writer_id_.c_str(), matches.current_count, total_readers_));
      if (matches.current_count >= total_readers_) {
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
    TestMsg message;
    message.from = writer_id_.c_str();
    message.value = 0;
    for (int i = 0; i < MSGS_PER_WRITER; ++i) {
      DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
      ++message.value;

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
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    std::string dw_participant_id;
    std::string dr_participant_id;
    std::string reader_id;
    std::string writer_id;
    bool reliable = false,
         dw_static_disc = false,
         dr_static_disc = false,
         origin = false;
    int domain_id_writer = 0, domain_id_reader = 0;

    {
      // New scope.
      ACE_Arg_Shifter shifter (argc, argv);
      while (shifter.is_anything_left ()) {
        const ACE_TCHAR* x;
        x = shifter.get_the_parameter (ACE_TEXT("-dw_participant"));
        if (x != NULL) {
          dw_participant_id = ACE_TEXT_ALWAYS_CHAR(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-dr_participant"));
        if (x != NULL) {
          dr_participant_id = ACE_TEXT_ALWAYS_CHAR(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reader"));
        if (x != NULL) {
          reader_id = ACE_TEXT_ALWAYS_CHAR(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-writer"));
        if (x != NULL) {
          writer_id = ACE_TEXT_ALWAYS_CHAR(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-origin"));
        if (x != NULL) {
          origin = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reliable"));
        if (x != NULL) {
          reliable = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-dw_static_disc"));
        if (x != NULL) {
          dw_static_disc = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-dr_static_disc"));
        if (x != NULL) {
          dr_static_disc = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-wdomain"));
        if (x != NULL) {
          domain_id_writer = ACE_OS::atoi(x);
          ACE_DEBUG((LM_DEBUG, "(%P|%t) main() - writer domain: %d\n", domain_id_writer));
        }
        x = shifter.get_the_parameter (ACE_TEXT("-rdomain"));
        if (x != NULL) {
          domain_id_reader = ACE_OS::atoi(x);
          ACE_DEBUG((LM_DEBUG, "(%P|%t) main() - reader domain: %d\n", domain_id_reader));

        }

        shifter.consume_arg ();
      }
    }

    // Create DomainParticipant
    DDS::DomainParticipantQos dr_dp_qos;
    dpf->get_default_participant_qos(dr_dp_qos);

    if (dr_static_disc) {
      dr_participant_id.resize(12);

      dr_dp_qos.user_data.value.length(6);
      dr_dp_qos.user_data.value[0] = fromhex(dr_participant_id, 0);
      dr_dp_qos.user_data.value[1] = fromhex(dr_participant_id, 1);
      dr_dp_qos.user_data.value[2] = fromhex(dr_participant_id, 2);
      dr_dp_qos.user_data.value[3] = fromhex(dr_participant_id, 3);
      dr_dp_qos.user_data.value[4] = fromhex(dr_participant_id, 4);
      dr_dp_qos.user_data.value[5] = fromhex(dr_participant_id, 5);
    }

    DDS::DomainParticipant_var participant_reading =
      dpf->create_participant(domain_id_reader,
                              dr_dp_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant_reading) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport
    TestMsgTypeSupport_var ts =
      new TestMsgTypeSupportImpl;

    if (ts->register_type(participant_reading, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" register_type failed!\n")),
                       -1);
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant_reading->create_topic("TheTopic",
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
        participant_reading->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_subscriber failed!\n")), -1);
    }

    // Setup Writer Side
    bool cleanup_participant = false;
    DDS::DomainParticipant_var pub_side_participant;
    if (domain_id_writer != domain_id_reader) {
      cleanup_participant = true;
      // Create DomainParticipant
      DDS::DomainParticipantQos dw_dp_qos;
      dpf->get_default_participant_qos(dw_dp_qos);

      if (dw_static_disc) {
        dw_participant_id.resize(12);

        dw_dp_qos.user_data.value.length(6);
        dw_dp_qos.user_data.value[0] = fromhex(dw_participant_id, 0);
        dw_dp_qos.user_data.value[1] = fromhex(dw_participant_id, 1);
        dw_dp_qos.user_data.value[2] = fromhex(dw_participant_id, 2);
        dw_dp_qos.user_data.value[3] = fromhex(dw_participant_id, 3);
        dw_dp_qos.user_data.value[4] = fromhex(dw_participant_id, 4);
        dw_dp_qos.user_data.value[5] = fromhex(dw_participant_id, 5);
      }
      pub_side_participant =
        dpf->create_participant(domain_id_writer,
                                dw_dp_qos,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!pub_side_participant) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_participant failed!\n")),
                         -1);
      }

    } else {
      pub_side_participant = participant_reading;
    }

    // Register TypeSupport
    TestMsgTypeSupport_var dw_ts =
      new TestMsgTypeSupportImpl;

    if (dw_ts->register_type(pub_side_participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() dw -")
                        ACE_TEXT(" register_type failed!\n")),
                       -1);
    }

    // Create Topic
    CORBA::String_var dw_type_name = dw_ts->get_type_name();
    DDS::Topic_var dw_topic =
        pub_side_participant->create_topic("TheTopic",
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
        pub_side_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_publisher failed!\n")),
                       -1);
    }
    writer_id.resize(6);

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Starting DataWriter %C\n", writer_id.c_str()));

    DDS::DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    if (dw_static_disc) {
      dw_qos.user_data.value.length(3);
      dw_qos.user_data.value[0] = fromhex(writer_id, 0);
      dw_qos.user_data.value[1] = fromhex(writer_id, 1);
      dw_qos.user_data.value[2] = fromhex(writer_id, 2);
    }
    if (reliable) {
      dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time.sec     = 1;
      dw_qos.reliability.max_blocking_time.nanosec = 0;
      // dw_qos.resource_limits.max_instances = 10;
      // dw_qos.history.depth = 10;
    } else {
      dw_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    }

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
    // Finish Writer Side Setup

    const int n_msgs = reliable ? MSGS_PER_WRITER * TOTAL_WRITERS : 0;

    // Create DataReaders
    reader_id.resize(6);
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl(reader_id, n_msgs, reader_done_callback, origin, writer, writer_id, TOTAL_READERS));

    DDS::DataReaderQos dr_qos;
    subscriber->get_default_datareader_qos(dr_qos);
    if (dr_static_disc) {
      dr_qos.user_data.value.length(3);
      dr_qos.user_data.value[0] = fromhex(reader_id, 0);
      dr_qos.user_data.value[1] = fromhex(reader_id, 1);
      dr_qos.user_data.value[2] = fromhex(reader_id, 2);
      dr_qos.reliability.kind = reliable ? DDS::RELIABLE_RELIABILITY_QOS : DDS::BEST_EFFORT_RELIABILITY_QOS;
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

    TestMsgDataReader_var reader_i =
      TestMsgDataReader::_narrow(reader);

    if (!reader_i) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }
    if (origin) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is the 'origin' so spawn writer task\n", writer_id.c_str()));
      WriterTask task(writer_id, writer, TOTAL_READERS);
      task.activate(DEFAULT_FLAGS, TOTAL_WRITERS);
      task.wait();
    }


    if (!reliable)
      ACE_OS::sleep(10);
    else {
      ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
      while (readers_done != TOTAL_READERS)
        readers_done_cond.wait();
      // Sleep allows an ACKNACK to be generated.
      ACE_OS::sleep(3);
    }
    if (cleanup_participant) {
      // Clean-up! (Writer has own participant it needs to clean up before finishing)
      // otherwise using same participant as subscriber, so let sub cleanup
      pub_side_participant->delete_contained_entities();
      dpf->delete_participant(pub_side_participant);
    }
    // Clean-up!
    participant_reading->delete_contained_entities();
    dpf->delete_participant(participant_reading);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
