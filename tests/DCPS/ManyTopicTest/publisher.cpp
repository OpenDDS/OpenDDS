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
#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/PublisherImpl.h"
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
#include "ace/OS_NS_time.h"
#include "ace/Atomic_Op_T.h"
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
    //  -w num_datawriters          defaults to 1
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

template<typename MessageType>
typename OpenDDS::DCPS::DDSTraits<MessageType>::DataWriterType::_var_type
create_writer(const DDS::Publisher_var& pub, const char* topicName,
  const DDS::DataWriterQos& qos = DATAWRITER_QOS_DEFAULT,
  const DDS::DataWriterListener_var& listener = 0,
  const DDS::StatusMask& mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK)
{
  const DDS::TypeSupport_var ts = new typename ::OpenDDS::DCPS::DDSTraits<MessageType>::TypeSupportTypeImpl();
  const DDS::DomainParticipant_var dp = pub->get_participant();
  const CORBA::String_var typeName = ts->get_type_name();
  (void) ts->register_type(dp, typeName); // may have been registered before

  const DDS::Topic_var topic =
    dp->create_topic(topicName, typeName, TOPIC_QOS_DEFAULT, 0, 0);
  if (!topic) return 0;

  const DDS::DataWriter_var dw =
    pub->create_datawriter(topic, qos, listener, mask);
  return OpenDDS::DCPS::DDSTraits<MessageType>::DataWriterType::_narrow(dw);
}

ACE_Atomic_Op<ACE_SYNCH_MUTEX, CORBA::Long> key(0);

void t1_init(T1::Foo1& foo, int)
{
  foo.x = -1;
  foo.y = -1;
  foo.key = ++key;
}

void t1_next(T1::Foo1& foo, int i)
{
  foo.x = (float)i;
  foo.c = 'A' + (i % 26);
}

void t2_init(T2::Foo2& foo, int)
{
  foo.key = ++key;
}

void t2_next(T2::Foo2& foo, int i)
{
  static char buff[20];
  ACE_OS::snprintf(buff, sizeof buff, "message %d", i + 1);
  foo.text = (const char*) buff;
}

void t3_init(T3::Foo3& foo, int)
{
  foo.key = ++key;
}

void t3_next(T3::Foo3& foo, int i)
{
  static char buff[20];
  ACE_OS::snprintf(buff, sizeof buff, "message %d", i + 1);
  foo.c = 'A' + (i % 26);
  foo.text = (const char*) buff;
  foo.s = i + 1;
  foo.l = i * 100;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

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

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(pub.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos(dw_qos);

#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      dw_qos.history.depth = history_depth;
#endif
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance;

      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw_qos.liveliness.lease_duration.nanosec = 0;

      T1::Foo1DataWriter_var dw1;
      T2::Foo2DataWriter_var dw2;
      T3::Foo3DataWriter_var dw3;
      T3::Foo3DataWriter_var dw4;

      if (topics & TOPIC_T1)
      {
        dw1 = create_writer<T1::Foo1>(pub, MY_TOPIC1, dw_qos);

        if (CORBA::is_nil(dw1.in()))
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1;
        }
      }

      if (topics & TOPIC_T2)
      {
        dw2 = create_writer<T2::Foo2>(pub, MY_TOPIC2, dw_qos);

        if (CORBA::is_nil(dw2.in()))
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1;
        }
      }

      if (topics & TOPIC_T3)
      {
        dw3 = create_writer<T3::Foo3>(pub, MY_TOPIC3, dw_qos);

        if (CORBA::is_nil(dw3.in()))
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1;
        }
      }

      if (topics & TOPIC_T4)
      {
        dw4 = create_writer<T3::Foo3>(pub, MY_TOPIC4, dw_qos);

        if (CORBA::is_nil(dw4.in()))
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1;
        }
      }
/*
      // Indicate that the publisher is ready
      FILE* writers_ready = ACE_OS::fopen(pub_ready_filename.c_str(), ACE_TEXT("w"));
      if (writers_ready == 0)
        {
          ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ready file\n")));
        }

      // Wait for the subscriber to be ready.
      FILE* readers_ready = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep(small_time);
          readers_ready = ACE_OS::fopen(sub_ready_filename.c_str(), ACE_TEXT("r"));
        } while (0 == readers_ready);

      ACE_OS::fclose(writers_ready);
      ACE_OS::fclose(readers_ready);
*/
      int num_writers(0);

      if (topics & TOPIC_T1)
      {
        num_writers++;
      }
      if (topics & TOPIC_T2)
      {
        num_writers++;
      }
      if (topics & TOPIC_T3)
      {
        num_writers++;
      }
      if (topics & TOPIC_T4)
      {
        num_writers++;
      }

      Writer **writers = new Writer* [num_writers];

      int idx(0);

      if (topics & TOPIC_T1)
      {
        TypedWriter<T1::Foo1>* tw =
          new TypedWriter<T1::Foo1>(dw1, 1, num_ops_per_thread);
        tw->init_instance_handler(t1_init);
        tw->next_sample_handler(t1_next);
        writers[idx++] = tw;
      }

      if (topics & TOPIC_T2)
      {
        TypedWriter<T2::Foo2>* tw =
          new TypedWriter<T2::Foo2>(dw2, 1, num_ops_per_thread);
        tw->init_instance_handler(t2_init);
        tw->next_sample_handler(t2_next);
        writers[idx++] = tw;
      }

      if (topics & TOPIC_T3)
      {
        TypedWriter<T3::Foo3>* tw =
          new TypedWriter<T3::Foo3>(dw3, 1, num_ops_per_thread);
        tw->init_instance_handler(t3_init);
        tw->next_sample_handler(t3_next);
        writers[idx++] = tw;
      }

      if (topics & TOPIC_T4)
      {
        TypedWriter<T3::Foo3>* tw =
          new TypedWriter<T3::Foo3>(dw4, 1, num_ops_per_thread);
        tw->init_instance_handler(t3_init);
        tw->next_sample_handler(t3_next);
        writers[idx++] = tw;
      }

      ACE_OS::srand((unsigned) ACE_OS::time(NULL));

      for (int i = 0; i < num_writers; i++)
      {
        writers[i]->start();
      }

      bool writers_finished = false;
      while ( !writers_finished )
        {
          writers_finished = true;
          for (int m = 0; m < num_writers; m++)
            {
              writers_finished =
                  writers_finished && writers[m]->is_finished();
            }
        }

      if (topics & TOPIC_T1)
        {
          // Indicate that the publisher is done
          ACE_TString t1_filename = ACE_TEXT(MY_TOPIC1) +
                                    pub_finished_filename;
          FILE* writers_completed =
            ACE_OS::fopen((synch_dir + t1_filename).c_str(), ACE_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      if (topics & TOPIC_T2)
        {
          // Indicate that the publisher is done
          ACE_TString t2_filename = ACE_TEXT(MY_TOPIC2) +
                                    pub_finished_filename;
          FILE* writers_completed =
            ACE_OS::fopen((synch_dir + t2_filename).c_str(), ACE_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      if (topics & TOPIC_T3)
        {
          // Indicate that the publisher is done
          ACE_TString t3_filename = ACE_TEXT(MY_TOPIC3) +
                                    pub_finished_filename;
          FILE* writers_completed =
            ACE_OS::fopen((synch_dir + t3_filename).c_str(), ACE_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      // Wait for the subscriber to finish.
      if (topics & TOPIC_T1)
        {
          FILE* readers_completed = 0;
          ACE_TString t1_filename = ACE_TEXT(MY_TOPIC1) +
                                    sub_finished_filename;
          do
            {
              ACE_Time_Value small_time(0,250000);
              ACE_OS::sleep(small_time);
              readers_completed =
                ACE_OS::fopen((synch_dir + t1_filename).c_str(), ACE_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T2)
        {
          FILE* readers_completed = 0;
          ACE_TString t2_filename = ACE_TEXT(MY_TOPIC2) +
                                    sub_finished_filename;
          do
            {
              ACE_Time_Value small_time(0,250000);
              ACE_OS::sleep(small_time);
              readers_completed =
                ACE_OS::fopen((synch_dir + t2_filename).c_str(), ACE_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T3)
        {
          FILE* readers_completed = 0;
          ACE_TString t3_filename = ACE_TEXT(MY_TOPIC3) +
                                    sub_finished_filename;
          do
            {
              ACE_Time_Value small_time(0,250000);
              ACE_OS::sleep(small_time);
              readers_completed =
                ACE_OS::fopen((synch_dir + t3_filename).c_str(), ACE_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      // Clean up publisher objects
      pub->delete_contained_entities();

      for (int n = 0; n < num_writers; n++)
        {
          delete writers[n];
        }
      delete [] writers;

      dp->delete_contained_entities();
      dpf->delete_participant(dp.in());

      TheServiceParticipant->shutdown();

    }
  catch (const TestException&)
    {
      ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception("Exception caught in main.cpp:");
      return 1;
    }

  return status;
}
