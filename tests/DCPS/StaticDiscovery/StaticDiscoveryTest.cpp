#include "TestMsgTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DCPS/GuidConverter.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_unistd.h"

/*
  NOTE:  The messages may not be processed by the reader in this test.
  This behavior is not an error.
*/

const int DOMAIN_ID = 100, MSGS_PER_WRITER = 10;

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
  WriterTask (std::vector<std::string>& writers,
              DDS::DomainParticipant_var participant,
              DDS::Topic_var topic,
              const bool& reliable,
              int total_readers)
    : writers_(writers)
    , participant_(participant)
    , topic_(topic)
    , reliable_(reliable)
    , total_readers_(total_readers)
  { }

  int svc()
  {
    int thread_id = thread_counter_++;

    // Create Publisher
    DDS::Publisher_var publisher =
      participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_publisher failed!\n")),
                       -1);
    }

    writers_[thread_id].resize(6);

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Starting DataWriter %C\n", writers_[thread_id].c_str()));

    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);
    qos.user_data.value.length(3);
    qos.user_data.value[0] = fromhex(writers_[thread_id], 0);
    qos.user_data.value[1] = fromhex(writers_[thread_id], 1);
    qos.user_data.value[2] = fromhex(writers_[thread_id], 2);

    if (reliable_) {
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      qos.reliability.max_blocking_time.sec = 5;
      qos.reliability.max_blocking_time.nanosec = 0;
    } else {
      qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    }

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic_,
                                   qos,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_datawriter failed!\n")),
                       -1);
    }

    TestMsgDataWriter_var message_writer =
      TestMsgDataWriter::_narrow(writer);

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

      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C has %d of %d readers\n", writers_[thread_id].c_str(), matches.current_count, total_readers_));
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

    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is waiting for acknowledgements\n", writers_[thread_id].c_str()));
    DDS::Duration_t timeout = { 30, 0 };
    message_writer->wait_for_acknowledgments(timeout);
    // With static discovery, it's not an error for wait_for_acks to fail
    // since the peer process may have terminated before sending acks.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is done\n", writers_[thread_id].c_str()));

    return 0;
  }

private:
  std::vector<std::string>& writers_;
  static ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> thread_counter_;
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var topic_;
  const bool& reliable_;
  int total_readers_;
};

ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> WriterTask::thread_counter_;

ACE_Thread_Mutex readers_done_lock;
ACE_Condition_Thread_Mutex readers_done_cond(readers_done_lock);
int readers_done = 0;
bool built_in_read_errors = false;

void reader_done_callback(bool bit_read_errors)
{
  ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
  if (bit_read_errors) built_in_read_errors = true;
  ++readers_done;
  ACE_DEBUG((LM_INFO, "(%P|%t) Reader %d done\n", readers_done));
  readers_done_cond.signal();
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    std::string participant_id;
    std::vector<std::string> readers;
    std::vector<std::string> writers;
    bool reliable = false;
    int total_readers = 0, total_writers = 0;
    bool check_bits = false;

    {
      // New scope.
      ACE_Arg_Shifter shifter (argc, argv);
      while (shifter.is_anything_left ()) {
        const ACE_TCHAR* x;
        x = shifter.get_the_parameter (ACE_TEXT("-participant"));
        if (x != NULL) {
          participant_id = ACE_TEXT_ALWAYS_CHAR(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reader"));
        if (x != NULL) {
          readers.push_back(ACE_TEXT_ALWAYS_CHAR(x));
        }
        x = shifter.get_the_parameter (ACE_TEXT("-writer"));
        if (x != NULL) {
          writers.push_back(ACE_TEXT_ALWAYS_CHAR(x));
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reliable"));
        if (x != NULL) {
          reliable = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-total_readers"));
        if (x != NULL) {
          total_readers = ACE_OS::atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-total_writers"));
        if (x != NULL) {
          total_writers = ACE_OS::atoi(x);
        }

        shifter.consume_arg ();
      }
    }
    if ((size_t) total_writers > writers.size()) {
      check_bits = true;
    }
    participant_id.resize(12);

    // Create DomainParticipant
    DDS::DomainParticipantQos dp_qos;
    dpf->get_default_participant_qos(dp_qos);
    dp_qos.user_data.value.length(6);
    dp_qos.user_data.value[0] = fromhex(participant_id, 0);
    dp_qos.user_data.value[1] = fromhex(participant_id, 1);
    dp_qos.user_data.value[2] = fromhex(participant_id, 2);
    dp_qos.user_data.value[3] = fromhex(participant_id, 3);
    dp_qos.user_data.value[4] = fromhex(participant_id, 4);
    dp_qos.user_data.value[5] = fromhex(participant_id, 5);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(DOMAIN_ID,
                              dp_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport
    TestMsgTypeSupport_var ts =
      new TestMsgTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" register_type failed!\n")),
                       -1);
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("TheTopic",
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
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_subscriber failed!\n")), -1);
    }

    const int n_msgs = reliable ? MSGS_PER_WRITER * total_writers : 0;

    // Create DataReaders
    for (std::vector<std::string>::iterator pos = readers.begin(), limit = readers.end();
         pos != limit;
         ++pos) {
      pos->resize(6);
      DDS::DataReaderListener_var listener(new DataReaderListenerImpl(*pos, n_msgs, reader_done_callback, check_bits));

#ifndef DDS_HAS_MINIMUM_BIT
      DataReaderListenerImpl* listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());
      DDS::Subscriber_var builtin = participant->get_builtin_subscriber();
      DDS::DataReader_var bitdr =
        builtin->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
      listener_servant->set_builtin_datareader(bitdr.in());
#endif /* DDS_HAS_MINIMUM_BIT */

      DDS::DataReaderQos qos;
      subscriber->get_default_datareader_qos(qos);
      qos.user_data.value.length(3);
      qos.user_data.value[0] = fromhex(*pos, 0);
      qos.user_data.value[1] = fromhex(*pos, 1);
      qos.user_data.value[2] = fromhex(*pos, 2);
      qos.reliability.kind = reliable ? DDS::RELIABLE_RELIABILITY_QOS : DDS::BEST_EFFORT_RELIABILITY_QOS;

      DDS::DataReader_var reader =
        subscriber->create_datareader(topic,
                                      qos,
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
    }

    WriterTask task(writers, participant, topic, reliable, total_readers);
    task.activate(DEFAULT_FLAGS, static_cast<int>(writers.size()));
    task.wait();

    if (!reliable)
      ACE_OS::sleep(10);
    else {
      ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
      while (readers_done != static_cast<int>(readers.size()))
        readers_done_cond.wait();
    }

    if (built_in_read_errors) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" BIT read failed!\n")),
                       -1);
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
