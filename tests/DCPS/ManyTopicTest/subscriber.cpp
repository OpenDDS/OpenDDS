// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================


#include "../common/TestException.h"
#include "DataReaderListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/ManyTopicTypes/Foo1DefTypeSupportImpl.h"
#include "tests/DCPS/ManyTopicTypes/Foo2DefTypeSupportImpl.h"
#include "tests/DCPS/ManyTopicTypes/Foo3DefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

#include "common.h"

static int topics = 0;

static int num_ops_per_thread = 10;
static int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
static int history_depth = 100;
#endif

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -o synchronization directory
    //  -i num_ops_per_thread       defaults to 10
    //  -r num_datareaders          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -z                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0)
    {
      synch_dir = currentArg;
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
    {
      if (!ACE_OS::strcmp(currentArg, ACE_TEXT("all")))
      {
        topics = TOPIC_T1 | TOPIC_T2 | TOPIC_T3 | TOPIC_T4;
      }
      else
      {
        int t = ACE_OS::atoi(currentArg);
        arg_shifter.consume_arg();

        switch (t)
        {
        case 1: topics |= TOPIC_T1; break;
        case 2: topics |= TOPIC_T2; break;
        case 3: topics |= TOPIC_T3; break;
        case 4: topics |= TOPIC_T4; break;
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) Invalid topic id (must be 1-4).\n")));
          return 1;
        }
      }
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

// We don't really need the type-specific data reader here, but we'll
// return it anyway in order to demostrate the use of the nested typedefs.
template<typename MessageType> // TSI is some generated TypeSupportImpl
DDS::DataReader_var
create_reader(const DDS::Subscriber_var& sub, const char* topicName,
  const DDS::DataReaderQos& qos = DATAREADER_QOS_DEFAULT,
  const DDS::DataReaderListener_var& listener = 0,
  const DDS::StatusMask& mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK)
{
  const DDS::TypeSupport_var ts = new typename ::OpenDDS::DCPS::DDSTraits<MessageType>::TypeSupportTypeImpl();
  const DDS::DomainParticipant_var dp = sub->get_participant();
  const CORBA::String_var typeName = ts->get_type_name();
  (void) ts->register_type(dp, typeName); // may have been registered before

  const DDS::Topic_var topic =
    dp->create_topic(topicName, typeName, TOPIC_QOS_DEFAULT, 0, 0);
  if (!topic) return 0;

  const DDS::DataReader_var dr =
    sub->create_datareader(topic, qos, listener, mask);
  return OpenDDS::DCPS::DDSTraits<MessageType>::DataReaderType::_narrow(dr);
}


void print1(const T1::Foo1& foo, int i)
{
  ACE_OS::printf("foo1[%d]: c = %c, x = %f y = %f, key = %d\n",
                 i, foo.c, foo.x, foo.y, foo.key);
}

void print2(const T2::Foo2& foo, int i)
{
  ACE_OS::printf("foo2[%d]: text = %s, key = %d\n",
                 i, foo.text.in(), foo.key);
}

void print3(const T3::Foo3& foo, int i)
{
  ACE_OS::printf("foo3[%d]: c = %c,  s = %d, l = %d, text = %s, key = %d\n",
                 i, foo.c, foo.s, foo.l, foo.text.in(), foo.key);
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  int status = 0;

  try
  {
    ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

    ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    //      TheServiceParticipant->liveliness_factor(100);

    // let the Service_Participant (in above line) strip out -DCPSxxx parameters
    // and then get application specific parameters.
    parse_args(argc, argv);

    if (!topics)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) Must run with one or more of the following: -t1 -t2 -t3\n")));
      return 1;
    }

    ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dp.in()))
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) create_participant failed.\n")));
      return 1;
    }

    ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                              ::DDS::SubscriberListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                       1);
    }

    // Create the Datareaders
    ::DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);

#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
    dr_qos.history.depth = history_depth;
#endif
    dr_qos.resource_limits.max_samples_per_instance =
      max_samples_per_instance;

    dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
    dr_qos.liveliness.lease_duration.nanosec = 0;


    int drl1_num_samples = 0, drl2_num_samples = 0,
      drl3_num_samples = 0, drl4_num_samples = 0;

    ::DDS::DataReaderListener_var drl1 =
        new DataReaderListenerImpl<T1::Foo1>(num_ops_per_thread, drl1_num_samples, print1);
    ::DDS::DataReaderListener_var drl2 =
        new DataReaderListenerImpl<T2::Foo2>(num_ops_per_thread, drl2_num_samples, print2);
    ::DDS::DataReaderListener_var drl3 =
        new DataReaderListenerImpl<T3::Foo3>(num_ops_per_thread, drl3_num_samples, print3);
    ::DDS::DataReaderListener_var drl4 =
        new DataReaderListenerImpl<T3::Foo3>(num_ops_per_thread, drl4_num_samples, print3);

    ::DDS::DataReader_var dr1;
    ::DDS::DataReader_var dr2;
    ::DDS::DataReader_var dr3;
    ::DDS::DataReader_var dr4;

    if (topics & TOPIC_T1)
    {
      dr1 = create_reader<T1::Foo1>(sub, MY_TOPIC1, dr_qos, drl1);
    }

    if (topics & TOPIC_T2)
    {
      dr2 = create_reader<T2::Foo2>(sub, MY_TOPIC2, dr_qos, drl2);
    }

    if (topics & TOPIC_T3)
    {
      dr3 = create_reader<T3::Foo3>(sub, MY_TOPIC3, dr_qos, drl3);
    }

    if (topics & TOPIC_T4)
    {
      dr4 = create_reader<T3::Foo3>(sub, MY_TOPIC4, dr_qos, drl4);
    }

    /*
    // Indicate that the subscriber is ready
    FILE* readers_ready = ACE_OS::fopen(sub_ready_filename.c_str(), ACE_TEXT("w"));
    if (readers_ready == 0)
    {
    ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
    }

    // Wait for the publisher to be ready
    FILE* writers_ready = 0;
    do
    {
    ACE_Time_Value small(0,250000);
    ACE_OS::sleep(small);
    writers_ready = ACE_OS::fopen(pub_ready_filename.c_str(), ACE_TEXT("r"));
    } while (0 == writers_ready);

    ACE_OS::fclose(readers_ready);
    ACE_OS::fclose(writers_ready);
    */
    // Indicate that the subscriber is done
    if (topics & TOPIC_T1)
    {
      ACE_TString t1_fn = ACE_TEXT(MY_TOPIC1) + sub_finished_filename;
      FILE* readers_completed =
        ACE_OS::fopen((synch_dir + t1_fn).c_str(), ACE_TEXT("w"));
      if (readers_completed == 0)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
      }
      ACE_OS::fclose(readers_completed);
    }

    if (topics & TOPIC_T2)
    {
      ACE_TString t2_fn = ACE_TEXT(MY_TOPIC2) + sub_finished_filename;
      FILE* readers_completed =
        ACE_OS::fopen((synch_dir + t2_fn).c_str(), ACE_TEXT("w"));
      if (readers_completed == 0)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
      }
      ACE_OS::fclose(readers_completed);
    }

    if (topics & TOPIC_T3)
    {
      ACE_TString t3_fn = ACE_TEXT(MY_TOPIC3) + sub_finished_filename;
      FILE* readers_completed =
        ACE_OS::fopen((synch_dir + t3_fn).c_str(), ACE_TEXT("w"));
      if (readers_completed == 0)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
      }
      ACE_OS::fclose(readers_completed);
    }

    // Wait for the publisher(s) to finish
    if (topics & TOPIC_T1)
    {
      FILE* writers_completed = 0;
      ACE_TString t1_fn = ACE_TEXT(MY_TOPIC1) + pub_finished_filename;
      do
      {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
        writers_completed =
          ACE_OS::fopen((synch_dir + t1_fn).c_str(), ACE_TEXT("r"));
      } while (0 == writers_completed);

      ACE_OS::fclose(writers_completed);
    }

    if (topics & TOPIC_T2)
    {
      FILE* writers_completed = 0;
      ACE_TString t2_fn = ACE_TEXT(MY_TOPIC2) + pub_finished_filename;
      do
      {
        ACE_Time_Value small_time(0,250000);
        ACE_OS::sleep(small_time);
        writers_completed =
          ACE_OS::fopen((synch_dir + t2_fn).c_str(), ACE_TEXT("r"));
      } while (0 == writers_completed);

      ACE_OS::fclose(writers_completed);
    }

    if (topics & TOPIC_T3)
    {
      FILE* writers_completed = 0;
      ACE_TString t3_fn = ACE_TEXT(MY_TOPIC3) + pub_finished_filename;
      do
      {
        ACE_Time_Value small_time(0,250000);
        ACE_OS::sleep(small_time);
        writers_completed =
          ACE_OS::fopen((synch_dir + t3_fn).c_str(), ACE_TEXT("r"));
      } while (0 == writers_completed);

      ACE_OS::fclose(writers_completed);
    }

    if (topics & TOPIC_T1)
    {
      ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC1,
                     drl1_num_samples);
      if (drl1_num_samples != num_ops_per_thread)
      {
        ACE_OS::fprintf(stderr,
                        "%s: Expected %d samples, got %d samples.\n",
                        MY_TOPIC1,
                        num_ops_per_thread, drl1_num_samples);
        return 1;
      }
    }
    if (topics & TOPIC_T2)
    {
      ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC2,
                     drl2_num_samples);
      if (drl2_num_samples != num_ops_per_thread)
      {
        ACE_OS::fprintf(stderr,
                        "%s: Expected %d samples, got %d samples.\n",
                        MY_TOPIC2,
                        num_ops_per_thread, drl2_num_samples);
        return 1;
      }
    }
    if (topics & TOPIC_T3)
    {
      ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC3,
                     drl3_num_samples);
      if (drl3_num_samples != num_ops_per_thread)
      {
        ACE_OS::fprintf(stderr,
                        "%s: Expected %d samples, got %d samples.\n",
                        MY_TOPIC3,
                        num_ops_per_thread, drl3_num_samples);
        return 1;
      }
    }
    if (topics & TOPIC_T4)
    {
      ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC4,
                     drl4_num_samples);
      if (drl4_num_samples != num_ops_per_thread)
      {
        ACE_OS::fprintf(stderr,
                        "%s: Expected %d samples, got %d samples.\n",
                        MY_TOPIC4,
                        num_ops_per_thread, drl4_num_samples);
        return 1;
      }
    }

    // clean up subscriber objects

    dp->delete_contained_entities();

    dpf->delete_participant(dp.in ());

    TheServiceParticipant->shutdown ();
  }
  catch (const TestException&)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
    return 1;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in main.cpp:");
    return 1;
  }

  return status;
}
