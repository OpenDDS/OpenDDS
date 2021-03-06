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
#include "tests/Utils/StatusMatching.h"

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

    unsigned long binary_id = static_cast<unsigned long>(fromhex(writers_[thread_id], 2))
                            + (256 * static_cast<unsigned long>(fromhex(writers_[thread_id], 1)))
                            + (256 * 256 * static_cast<unsigned long>(fromhex(writers_[thread_id], 0)));
    char config_name_buffer[16];
    sprintf(config_name_buffer, "Config%lu", binary_id);
    OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config_name_buffer, publisher);

    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);
    qos.user_data.value.length(3);
    qos.user_data.value[0] = fromhex(writers_[thread_id], 0);
    qos.user_data.value[1] = fromhex(writers_[thread_id], 1);
    qos.user_data.value[2] = fromhex(writers_[thread_id], 2);

    if (reliable_) {
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      qos.reliability.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;
      qos.reliability.max_blocking_time.nanosec = DDS::DURATION_INFINITE_NSEC;
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
    Utils::wait_match(writer, total_readers_, Utils::GTE);

    // Write samples
    TestMsg message;
    message.src = thread_id;
    message.value = 1;
    for (int i = 0; i < MSGS_PER_WRITER; ++i) {
      DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
      ++message.value;

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: main() -")
                   ACE_TEXT(" write returned %d!\n"), error));
      }
    }

    DDS::Duration_t duration = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    writer->wait_for_acknowledgments(duration);

    // Block until Subscriber is gone
    Utils::wait_match(writer, 0);

    publisher->delete_datawriter(writer);

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
    std::vector<DDS::DataReader_var> datareaders;
    for (std::vector<std::string>::iterator pos = readers.begin(), limit = readers.end();
         pos != limit;
         ++pos) {
      pos->resize(6);
      DDS::DataReaderListener_var listener(new DataReaderListenerImpl(*pos, reliable, true, writers, total_writers, n_msgs, reader_done_callback, subscriber.in(), check_bits));

#ifndef DDS_HAS_MINIMUM_BIT
      DataReaderListenerImpl* listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (!listener_servant) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
      }

      DDS::Subscriber_var builtin = participant->get_builtin_subscriber();
      DDS::DataReader_var bitdr =
        builtin->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
      listener_servant->set_builtin_datareader(bitdr.in());
#endif /* DDS_HAS_MINIMUM_BIT */

      unsigned long binary_id = static_cast<unsigned long>(fromhex(*pos, 2))
                              + (256 * static_cast<unsigned long>(fromhex(*pos, 1)))
                              + (256 * 256 * static_cast<unsigned long>(fromhex(*pos, 0)));
      char config_name_buffer[16];
      sprintf(config_name_buffer, "Config%lu", binary_id);
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config_name_buffer, subscriber);

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

      datareaders.push_back(reader);

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

    if (!reliable)
      ACE_OS::sleep(10);
    else {
      ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
      while (readers_done != static_cast<int>(readers.size()))
        readers_done_cond.wait();
    }

    for (std::vector<DDS::DataReader_var>::iterator it = datareaders.begin(), limit = datareaders.end(); it != limit; ++it) {
      subscriber->delete_datareader(*it);
    }
    datareaders.clear();

    task.wait();

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

  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT("Unexpected Transport Exception!\n")),
                       -1);
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
