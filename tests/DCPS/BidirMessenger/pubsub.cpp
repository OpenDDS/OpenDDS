/*
 *
 * Copyright 2009 Object Computing, Inc.
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
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include "tests/Utils/ExceptionStreams.h"


#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#endif

#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

#include <sstream>

namespace {

int num_processes = 2;
int num_topics = 200;
int num_samples_per_topic = 10;
int dont_verify_sample_count_sleep_sec = -1;

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  //
  // Command-line Options:
  //
  //    -w <number of topics>
  //    -s <samples per topic>
  //    -z <sec>  -- don't check the sample counts, just sleep this much
  //                 and exit
  //

  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("p:w:s:z:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'p':
      num_processes = ACE_OS::atoi(get_opts.opt_arg());
      std::cout << "num_processes = " << num_processes << std::endl;
      break;

    case 'w':
      num_topics = ACE_OS::atoi(get_opts.opt_arg());
      std::cout << "num_topics = " << num_topics << std::endl;
      break;

    case 's':
      num_samples_per_topic = ACE_OS::atoi(get_opts.opt_arg());
      std::cout << "num_samples_per_topic = " << num_samples_per_topic
                << std::endl;
      break;

    case 'z':
      dont_verify_sample_count_sleep_sec = ACE_OS::atoi(get_opts.opt_arg());
      std::cout << "Don't wait for all samples; sleep "
                << dont_verify_sample_count_sleep_sec
                << " seconds instead" << std::endl;
      break;

    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -n num_procs\n"), argv[0]),
                       -1);
    }
  }

  return 0;
}

} // namespace

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
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
      dpf->create_participant(411,
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
    DDS::Topic_var* topic = new DDS::Topic_var[num_topics];
    for (int i = 0; i < num_topics; ++i) {
      std::stringstream s;
      s << "Movie Discussion List " << i << std::ends;

      topic[i] = participant->create_topic(s.str().c_str(),
                                           CORBA::String_var(mts->get_type_name()),
                                           TOPIC_QOS_DEFAULT,
                                           DDS::TopicListener::_nil(),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic[i].in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         -1);
      }
    }

    //
    // Publisher, DataWriter
    //

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

    // Create DataWriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    DDS::DataWriter_var* dw = new DDS::DataWriter_var[num_topics];
    for (int i = 0; i < num_topics; ++i) {
      dw[i] = pub->create_datawriter(topic[i].in(),
                                     dw_qos,
                                     DDS::DataWriterListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw[i].in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         -1);
      }
    }

    //
    // Subscriber, DataReader
    //

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderListener_var* listener =
      new DDS::DataReaderListener_var[num_topics];
    DDS::DataReader_var* reader = new DDS::DataReader_var[num_topics];

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos (dr_qos);
    dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    for (int i = 0; i < num_topics; ++i) {
      listener[i] = new DataReaderListenerImpl;

      reader[i] =
        sub->create_datareader(topic[i].in(),
                               dr_qos,
                               listener[i].in(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(reader[i].in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
      }
    }

    // Block until all Subscribers are attached
    for (int i = 0; i < num_topics; ++i) {
      DDS::StatusCondition_var condition = reader[i]->get_statuscondition();
      condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition);
      DDS::Duration_t timeout =
        { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
      DDS::ConditionSeq conditions;
      DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

      do {
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: wait() failed!\n")), -1);
        }

        if (reader[i]->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")), -1);
        }
      } while (matches.current_count < num_processes);
    }

    int expected_num_reads = num_samples_per_topic * num_processes;

    // Have all subscribers; start writing threads
    Writer** writer = new Writer*[num_topics];
    for (int i = 0; i < num_topics; ++i) {
      writer[i] = new Writer(dw[i].in(),num_processes,num_samples_per_topic);
      writer[i]->start();
    }

    // Wait for all expected samples
    if (dont_verify_sample_count_sleep_sec < 0) {
      for (int i = 0; i < num_topics; ++i) {
        DataReaderListenerImpl* listener_impl =
          dynamic_cast<DataReaderListenerImpl*>(listener[i].in());

        while (listener_impl->num_reads() != expected_num_reads) {
          ACE_Time_Value small_time(0, 1000000);
          ACE_OS::sleep(small_time);
          std::cout << i << ") Pid " << ACE_OS::getpid() << " received "
                    << listener_impl->num_reads() << " of "
                    << expected_num_reads << std::endl;
        }
        std::cout << i << ") Pid " << ACE_OS::getpid() << " received "
                  << listener_impl->num_reads() << " of "
                  << expected_num_reads << std::endl;
      }
    } else {
      std::cout << "Sleeping " << dont_verify_sample_count_sleep_sec
                << std::endl;
      for (int i = 0; i < dont_verify_sample_count_sleep_sec; ++i) {
        std::cout << "." << std::flush;
        ACE_OS::sleep(1);
      }
      std::cout << std::endl;
    }

    for (int i = 0; i < num_topics; ++i) {
      writer[i]->end();
      delete writer[i];
    }

    delete [] topic;
    delete [] dw;
    delete [] listener;
    delete [] reader;
    delete [] writer;

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
