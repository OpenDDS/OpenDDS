// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================


#include "Writer.h"
#include "tests/DCPS/common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/PublisherImpl.h"
#include "DataReaderListener1.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "Foo1DefTypeSupportImpl.h"
#include "Foo4DefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"


#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_time.h"

#include "common.h"

static int topics = 0;
static int subscribe_topic = 0;

static int num_ops_per_thread = 10;
static int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    // options:
    //  -T synch directory
    //  -i num_ops_per_thread       defaults to 10
    //  -w num_datawriters          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -z                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-T"))) != 0) {
      synch_dir = currentArg;
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0) {
      num_ops_per_thread = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0) {
      max_samples_per_instance = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-p"))) != 0) {
      int t = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

      switch (t) {
      case 1: topics |= TOPIC_T1; break;
      case 2: topics |= TOPIC_T2; break;
      case 3: topics |= TOPIC_T3; break;
      case 4: topics |= TOPIC_T4; break;
      case 5: topics |= TOPIC_T5; break;
      default:
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) Invalid topic id (must be 1-5).\n")));
        return 1;
      }

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0) {
      int t = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

      switch (t) {
      case 6: subscribe_topic |= TOPIC_T6; break;
      case 7: subscribe_topic |= TOPIC_T7; break;
      default:
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) Invalid topic id (must be 6-7).\n")));
        return 1;
      }

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0) {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();

    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try {
    ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

    ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    // let the Service_Participant (in above line) strip out -DCPSxxx parameters
    // and then get application specific parameters.
    parse_args(argc, argv);

    if (!topics) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) Must run with one or more of the following: -p1 -p2 -p3 -p4 -p5 -s6 -s7\n")));
      return 1;
    }

    ::T1::Foo1TypeSupport_var fts1;
    ::T4::Foo4TypeSupport_var fts4;

    if (topics & (TOPIC_T1 | TOPIC_T3 | TOPIC_T4| TOPIC_T5)) {
      fts1 = new ::T1::Foo1TypeSupportImpl;
    }

    if (topics & TOPIC_T2) {
      fts4 = new ::T4::Foo4TypeSupportImpl;
    }

    ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dp)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1;
    }

    if (topics & (TOPIC_T1 | TOPIC_T3 | TOPIC_T4 | TOPIC_T5)) {
      if (::DDS::RETCODE_OK != fts1->register_type(dp, MY_TYPE1)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("Failed to register the Foo1TypeSupport.")));
        return 1;
      }
    }

    if (topics & TOPIC_T2) {
      if (::DDS::RETCODE_OK != fts4->register_type(dp, MY_TYPE4)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("Failed to register the Foo4TypeSupport.")));
        return 1;
      }
    }

    ::DDS::TopicQos topic_qos;
    dp->get_default_topic_qos(topic_qos);

    topic_qos.resource_limits.max_samples_per_instance =
      max_samples_per_instance;

    ::DDS::Topic_var topic1;
    ::DDS::Topic_var topic2;
    ::DDS::Topic_var topic3;
    ::DDS::Topic_var topic4;
    ::DDS::Topic_var topic5;
    ::DDS::Topic_var topic6;
    ::DDS::Topic_var topic7;

    if (topics & TOPIC_T1) {
      topic1 = dp->create_topic(MY_TOPIC1, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic1)) {
        return 1;
      }
    }

    if (topics & TOPIC_T2) {
      topic2 = dp->create_topic(MY_TOPIC2, MY_TYPE4, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic2)) {
        return 1;
      }
    }

    if (topics & TOPIC_T3) {
      topic3 = dp->create_topic(MY_TOPIC3, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic3)) {
        return 1;
      }
    }

    if (topics & TOPIC_T4) {
      topic4 = dp->create_topic(MY_TOPIC4, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic4)) {
        return 1;
      }
    }

    if (topics & TOPIC_T5) {
      topic5 = dp->create_topic(MY_TOPIC5, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic5)) {
        return 1;
      }
    }

    if (subscribe_topic & TOPIC_T6) {
      topic6 = dp->create_topic(MY_TOPIC6, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic6)) {
        return 1;
      }
    }

    if (subscribe_topic & TOPIC_T7) {
      topic7 = dp->create_topic(MY_TOPIC7, MY_TYPE1, TOPIC_QOS_DEFAULT,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic7)) {
        return 1;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    // Create the subscriber
    /////////////////////////////////////////////////////////////////////////
    ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                              ::DDS::SubscriberListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                       1);
    }

    // Create the Datareader
    ::DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);

    dr_qos.resource_limits.max_samples_per_instance =
      max_samples_per_instance;

    dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
    dr_qos.liveliness.lease_duration.nanosec = 0;

    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    ::DDS::DataReader_var dr;
    ::DDS::DataReaderListener_var drl =
        new DataReaderListenerImpl1(num_ops_per_thread);

    if (subscribe_topic & TOPIC_T6) {
      dr = sub->create_datareader(topic6, dr_qos, drl,
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    } else if (subscribe_topic & TOPIC_T7) {
      dr = sub->create_datareader(topic7, dr_qos, drl,
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    }

    /////////////////////////////////////////////////////////////////////////
    // Create the publisher
    /////////////////////////////////////////////////////////////////////////
    ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                       1);
    }

    // Create the datawriters
    ::DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);

    dw_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;

    dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
    dw_qos.liveliness.lease_duration.nanosec = 0;

    ::DDS::DataWriter_var dw1;
    ::DDS::DataWriter_var dw2;
    ::DDS::DataWriter_var dw3;
    ::DDS::DataWriter_var dw4;
    ::DDS::DataWriter_var dw5;

    int num_writers(0);

    if (topics & TOPIC_T1) {
      dw1 = pub->create_datawriter(topic1, dw_qos,
                                   ::DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw1)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1;
      }
      ++num_writers;
    }

    if (topics & TOPIC_T2) {
      dw2 = pub->create_datawriter(topic2, dw_qos,
                                   ::DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw2)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1;
      }
      ++num_writers;
    }

    if (topics & TOPIC_T3) {
      dw3 = pub->create_datawriter(topic3, dw_qos,
                                   ::DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw3)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1;
      }
      ++num_writers;
    }

    if (topics & TOPIC_T4) {
      dw4 = pub->create_datawriter(topic4, dw_qos,
                                   ::DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw4)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1;
      }
      ++num_writers;
    }

    if (topics & TOPIC_T5) {
      dw5 = pub->create_datawriter(topic5, dw_qos,
                                   ::DDS::DataWriterListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw5)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1;
      }
      ++num_writers;
    }

    Writer** writers = new Writer*[num_writers];
    int idx(0);

    if (topics & TOPIC_T1) {
      writers[idx++] = new Writer(dw1, 1, num_ops_per_thread);
    }

    if (topics & TOPIC_T2) {
      writers[idx++] = new Writer(dw2, 1, num_ops_per_thread);
    }

    if (topics & TOPIC_T3) {
      writers[idx++] = new Writer(dw3, 1, num_ops_per_thread);
    }

    if (topics & TOPIC_T4) {
      writers[idx++] = new Writer(dw4, 1, num_ops_per_thread);
    }

    if (topics & TOPIC_T5) {
      writers[idx++] = new Writer(dw5, 1, num_ops_per_thread);
    }

    for (int i = 0; i < num_writers; ++i) {
      writers[i]->start();
    }

    bool writers_finished = false;
    while (!writers_finished) {
      writers_finished = true;

      for (int m = 0; m < num_writers; ++m) {
        writers_finished = writers_finished && writers[m]->is_finished();
      }

      if (!writers_finished) ACE_OS::sleep(small_time);
    }

    if (topics & TOPIC_T1) {
      ACE_TString t1_filename = ACE_TEXT(MY_TOPIC1) + pub_finished_filename;
      FILE* writers_completed =
        ACE_OS::fopen((synch_dir + t1_filename).c_str(), ACE_TEXT("w"));
      if (writers_completed == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ")
                   ACE_TEXT("completed file\n")));
        status = 1;
      } else {
        ACE_OS::fclose(writers_completed);
      }
    }

    if (topics & TOPIC_T2) {
      ACE_TString t2_filename = ACE_TEXT(MY_TOPIC2) + pub_finished_filename;
      FILE* writers_completed =
        ACE_OS::fopen((synch_dir + t2_filename).c_str(), ACE_TEXT("w"));
      if (writers_completed == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ")
                   ACE_TEXT("completed file\n")));
        status = 1;
      } else {
        ACE_OS::fclose(writers_completed);
      }
    }

    if (topics & TOPIC_T3) {
      ACE_TString t3_filename = ACE_TEXT(MY_TOPIC3) + pub_finished_filename;
      FILE* writers_completed =
        ACE_OS::fopen((synch_dir + t3_filename).c_str(), ACE_TEXT("w"));
      if (writers_completed == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ")
                   ACE_TEXT("completed file\n")));
        status = 1;
      } else {
        ACE_OS::fclose(writers_completed);
      }
    }

    if (topics & TOPIC_T4) {
      ACE_TString t4_filename = ACE_TEXT(MY_TOPIC4) + pub_finished_filename;
      FILE* writers_completed =
        ACE_OS::fopen((synch_dir + t4_filename).c_str(), ACE_TEXT("w"));
      if (writers_completed == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ")
                   ACE_TEXT("completed file\n")));
        status = 1;
      } else {
        ACE_OS::fclose(writers_completed);
      }
    }

    if (topics & TOPIC_T5) {
      ACE_TString t5_filename = ACE_TEXT(MY_TOPIC5) + pub_finished_filename;
      FILE* writers_completed =
        ACE_OS::fopen((synch_dir + t5_filename).c_str(), ACE_TEXT("w"));
      if (writers_completed == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ")
                   ACE_TEXT("completed file\n")));
        status = 1;
      } else {
        ACE_OS::fclose(writers_completed);
      }
    }

    if (topics & TOPIC_T1) {
      wait_for_file(ACE_TEXT(MY_TOPIC1), sub_finished_filename);
    }

    if (topics & TOPIC_T2) {
      wait_for_file(ACE_TEXT(MY_TOPIC2), sub_finished_filename);
    }

    if (topics & TOPIC_T3) {
      wait_for_file(ACE_TEXT(MY_TOPIC3), sub_finished_filename);
    }

    if (topics & TOPIC_T4) {
      wait_for_file(ACE_TEXT(MY_TOPIC4), sub_finished_filename);
    }

    if (topics & TOPIC_T5) {
      wait_for_file(ACE_TEXT(MY_TOPIC5), sub_finished_filename);
    }

    if (subscribe_topic & TOPIC_T6) {
      const DataReaderListenerImpl* const drl_servant =
        dynamic_cast<const DataReaderListenerImpl*>(drl.in());
      if (!check_listener(drl_servant, num_ops_per_thread, ACE_TEXT(MY_TOPIC6)))
        status = 1;

    } else if (subscribe_topic & TOPIC_T7) {
      const DataReaderListenerImpl* const drl_servant =
        dynamic_cast<const DataReaderListenerImpl*>(drl.in());
      if (!check_listener(drl_servant, num_ops_per_thread, ACE_TEXT(MY_TOPIC7)))
        status = 1;
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) delete contained entities on pub\n"));
    pub->delete_contained_entities();

    for (int n = 0; n < num_writers; ++n) {
      delete writers[n];
    }
    delete [] writers;

    dp->delete_contained_entities();
    dpf->delete_participant(dp);

    TheServiceParticipant->shutdown();

  } catch (const TestException&) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TestException caught in main.cpp.\n")));
    return 1;

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  }

  return status;
}
