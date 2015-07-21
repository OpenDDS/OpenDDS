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

#include "ace/Arg_Shifter.h"

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
    return c - 'a';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A';
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

    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);
    qos.user_data.value.length(3);
    qos.user_data.value[0] = fromhex(writers_[thread_id], 0);
    qos.user_data.value[1] = fromhex(writers_[thread_id], 1);
    qos.user_data.value[2] = fromhex(writers_[thread_id], 2);
    if (reliable_) {
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      qos.reliability.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;
      qos.resource_limits.max_instances = 10;
      qos.history.depth = 10;
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

    DDS::Duration_t timeout = { 30, 0 };
    message_writer->wait_for_acknowledgments(timeout);
    // With static discovery, it's not an error for wait_for_acks to fail
    // since the peer process may have terminated before sending acks.

//     try
//       {
//         ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    -> PARTICIPANT STARTED\n")));

//         DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
//         DDS::DomainParticipant_var participant;
//         DDS::Publisher_var publisher;
//         DDS::DataWriter_var writer;
//         TestMsgDataWriter_var writer_i;
//         DDS::StatusCondition_var cond;
//         DDS::WaitSet_var ws = new DDS::WaitSet;

//         // Create Participant
//         participant =
//           dpf->create_participant(42,
//                                   PARTICIPANT_QOS_DEFAULT,
//                                   DDS::DomainParticipantListener::_nil(),
//                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

// #ifdef OPENDDS_SAFETY_PROFILE
//         // RTPS cannot be shared
//         char config_name[64], inst_name[64];
//         ACE_OS::snprintf(config_name, 64, "cfg_%d", thread_index);
//         ACE_OS::snprintf(inst_name, 64, "rtps_%d", thread_index);

//         ACE_DEBUG((LM_INFO,
//                    "(%P|%t)    -> PARTICIPANT creating transport config %C\n",
//                    config_name));
//         OpenDDS::DCPS::TransportConfig_rch config =
//           TheTransportRegistry->create_config(config_name);
//         OpenDDS::DCPS::TransportInst_rch inst =
//           TheTransportRegistry->create_inst(inst_name, "rtps_udp");
//         config->instances_.push_back(inst);
//         TheTransportRegistry->bind_config(config_name, participant);
// #endif


//         if (CORBA::is_nil(participant.in()))
//           ACE_ERROR_RETURN((LM_ERROR,
//                             ACE_TEXT("%N:%l: svc()")
//                             ACE_TEXT(" create_participant failed!\n")), 1);

//         {
//           // Create Publisher
//           publisher =
//             participant->create_publisher(PUBLISHER_QOS_DEFAULT,
//                                           DDS::PublisherListener::_nil(),
//                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

//           if (CORBA::is_nil(publisher.in()))
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("%N:%l: svc()")
//                               ACE_TEXT(" create_publisher failed!\n")), 1);


//           // Register Type (FooType)
//           TestMsgTypeSupport_var ts = new TestMsgTypeSupportImpl;
//           if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("%N:%l: svc()")
//                               ACE_TEXT(" register_type failed!\n")), 1);

//           // Create Topic (TestMsgTopic)
//           DDS::Topic_var topic =
//             participant->create_topic("TestMsgTopic",
//                                       ts->get_type_name(),
//                                       TOPIC_QOS_DEFAULT,
//                                       DDS::TopicListener::_nil(),
//                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

//           if (CORBA::is_nil(topic.in()))
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("%N:%l: svc()")
//                               ACE_TEXT(" create_topic failed!\n")), 1);

//           // Create DataWriter
//           DDS::DataWriterQos writer_qos;
//           publisher->get_default_datawriter_qos(writer_qos);

//           writer =
//             publisher->create_datawriter(topic.in(),
//                                          writer_qos,
//                                          DDS::DataWriterListener::_nil(),
//                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

//           if (CORBA::is_nil(writer.in()))
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("%N:%l: svc()")
//                               ACE_TEXT(" create_datawriter failed!\n")), 1);

//           writer_i = TestMsgDataWriter::_narrow(writer);
//           if (CORBA::is_nil(writer_i))
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("%N:%l: svc()")
//                               ACE_TEXT(" _narrow failed!\n")), 1);

//           // Block until Subscriber is available
//           cond = writer->get_statuscondition();
//           cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

//           ws->attach_condition(cond);

//           DDS::Duration_t timeout =
//             { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

//           DDS::ConditionSeq conditions;
//           DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};
//           do
//             {
//               if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
//                 ACE_ERROR_RETURN((LM_ERROR,
//                                   ACE_TEXT("%N:%l: svc()")
//                                   ACE_TEXT(" wait failed!\n")), 1);

//               if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
//                 {
//                   ACE_ERROR ((LM_ERROR,
//                               "(%P|%t) ERROR: failed to get publication matched status\n"));
//                   ACE_OS::exit (1);
//                 }
//             }
//           while (matches.current_count < 1);

//           ws->detach_condition(cond);

//           DDS::Duration_t interval = { 30, 0};
//           if( DDS::RETCODE_OK != writer->wait_for_acknowledgments( interval)) {
//             ACE_ERROR_RETURN((LM_ERROR,
//                               ACE_TEXT("(%P:%t) ERROR: svc() - ")
//                               ACE_TEXT("timed out waiting for acks!\n")
//                               ), 1);
//           }
//         }

//         // Clean-up!
//         ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER PARTICIPANT DEL CONT ENTITIES\n")));
//         participant->delete_contained_entities();
//         ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER DELETE PARTICIPANT\n")));
//         dpf->delete_participant(participant.in());
//         ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)       <- PUBLISHER PARTICIPANT VARS GOING OUT OF SCOPE\n")));
//       }
//     catch (const CORBA::Exception& e)
//       {
//         e._tao_print_exception("caught in svc()");
//         return 1;
//       }

//     ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- PARTICIPANT FINISHED\n")));

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

    std::string participant_id;
    std::vector<std::string> readers;
    std::vector<std::string> writers;
    bool reliable = false;
    int total_readers = 0, total_writers = 0;

    {
      // New scope.
      ACE_Arg_Shifter shifter (argc, argv);
      while (shifter.is_anything_left ()) {
        const ACE_TCHAR* x;
        x = shifter.get_the_parameter (ACE_TEXT("-participant"));
        if (x != NULL) {
          participant_id = x;
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reader"));
        if (x != NULL) {
          readers.push_back(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-writer"));
        if (x != NULL) {
          writers.push_back(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-reliable"));
        if (x != NULL) {
          reliable = atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-total_readers"));
        if (x != NULL) {
          total_readers = atoi(x);
        }
        x = shifter.get_the_parameter (ACE_TEXT("-total_writers"));
        if (x != NULL) {
          total_writers = atoi(x);
        }

        shifter.consume_arg ();
      }
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

      DDS::DataReaderListener_var listener(new DataReaderListenerImpl(n_msgs, reader_done_callback));

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
    task.activate(DEFAULT_FLAGS, writers.size());
    task.wait();

    if (!reliable)
      ACE_OS::sleep(10);
    else {
      ACE_Guard<ACE_Thread_Mutex> g(readers_done_lock);
      while (readers_done != static_cast<int>(readers.size()))
        readers_done_cond.wait();
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
