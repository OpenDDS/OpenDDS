// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================

#ifdef ACE_LYNXOS_MAJOR
#define VMOS_DEV
#endif

#include "DataReaderListener.h"
#include "TestException.h"
// Include the Udp.h to make sure Initializer is created before the Service
// Configurator open service configure file.
#ifndef OPENDDS_SAFETY_PROFILE
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#include "dds/DCPS/transport/shmem/Shmem.h"
#endif
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"

#include "ace/Arg_Shifter.h"
#include "ace/Default_Constants.h"

#include "common.h"
#include <sstream>
#include <cstdio>


/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -r num_datareaders          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -u using_udp                defaults to 0 - using TCP
    //  -c using_multicast          defaults to 0 - using TCP
    //  -p using rtps transport flag     defaults to 0 - using TCP
    //  -s using shared memory flag      defaults to 0 - using TCP
    //  -m num_instances_per_writer defaults to 1
    //  -i num_samples_per_instance defaults to 1
    //  -w num_datawriters          defaults to 1
    //  -z length of float sequence in data type   defaults to 10
    //  -y read operation interval                 defaults to 0
    //  -k data type has no key flag               defaults to 0 - has key
    //  -f mixed transport test flag               defaults to 0 - single transport test
    //  -o directory of synch files used to coordinate publisher and subscriber
    //                              defaults to current directory.
    //  -v                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0)
    {
      num_datareaders = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-u"))) != 0)
    {
      using_udp = ACE_OS::atoi (currentArg);
      if (using_udp == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Subscriber Using UDP transport.\n"));
      }
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-c"))) != 0)
    {
      using_multicast = ACE_OS::atoi (currentArg);
      if (using_multicast == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Subscriber Using MULTICAST transport.\n"));
      }
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-p"))) != 0)
    {
      using_rtps_transport = ACE_OS::atoi(currentArg);
      if (using_rtps_transport == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Subscriber Using RTPS transport.\n"));
      }
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0)
    {
      using_shmem = ACE_OS::atoi(currentArg);
      if (using_shmem == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Subscriber Using Shmem transport.\n"));
      }
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0)
    {
      num_instances_per_writer = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-w"))) != 0)
    {
      num_datawriters = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-z"))) != 0)
    {
      sequence_length = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0)
    {
      no_key = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-y"))) != 0)
    {
      op_interval_ms = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-f"))) != 0)
    {
      mixed_trans = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0)
    {
      synch_file_dir = currentArg;
      pub_ready_filename = synch_file_dir + pub_ready_filename;
      pub_finished_filename = synch_file_dir + pub_finished_filename;
      sub_ready_filename = synch_file_dir + sub_ready_filename;
      sub_finished_filename = synch_file_dir + sub_finished_filename;

      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-v")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }

  if ((using_udp != 0 || mixed_trans != 0) && using_multicast != 0)
  {
    using_multicast = 0;
    ACE_DEBUG((LM_DEBUG, "Subscriber NOT using MULTICAST transport.\n"));
  }

  // Indicates successful parsing of the command line
  return 0;
}


::DDS::Subscriber_ptr
create_subscriber (::DDS::DomainParticipant_ptr participant,
                   int                          attach_to_udp,
                   int                          attach_to_multicast,
                   int                          attach_to_rtps,
                   int                          attach_to_shmem)
{

  // Create the subscriber
  ::DDS::Subscriber_var sub;

  try
    {
      sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to create_subscriber.")));
          return ::DDS::Subscriber::_nil ();
        }

      // Attach the subscriber to the transport.
      if (attach_to_udp)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to udp \n")));
          TheTransportRegistry->bind_config("udp", sub.in());
        }
      else if (attach_to_multicast)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to multicast \n")));
          TheTransportRegistry->bind_config("multicast", sub.in());
        }
      else if (attach_to_rtps)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to RTPS\n")));
          TheTransportRegistry->bind_config("rtps", sub);
        }
      else if (attach_to_shmem)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to shmem\n")));
          TheTransportRegistry->bind_config("shmem", sub);
        }
      else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to tcp \n")));
          TheTransportRegistry->bind_config("tcp", sub.in());
        }

    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in create_subscriber. ")));
      return ::DDS::Subscriber::_nil ();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in create_subscriber.");
      return ::DDS::Subscriber::_nil ();
    }

  return sub._retn ();
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  ::DDS::DomainParticipantFactory_var dpf;
  ::DDS::DomainParticipant_var participant;

  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

      dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      results.init ();

      participant =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ()))
        {
          ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_participant failed.\n")));
          return 1 ;
        }

      if (no_key)
        {
          ::Xyz::FooNoKeyTypeSupportImpl* nokey_fts_servant
            = new ::Xyz::FooNoKeyTypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = nokey_fts_servant;

          if (::DDS::RETCODE_OK != nokey_fts_servant->register_type(participant.in (), MY_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("Failed to register the FooTypeSupport.")));
              throw TestException ();
            }
        }
      else
        {
          ::Xyz::FooTypeSupportImpl* fts_servant
            = new ::Xyz::FooTypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = fts_servant;

          if (::DDS::RETCODE_OK != fts_servant->register_type(participant.in (), MY_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("Failed to register the FooNoTypeTypeSupport.")));
              throw TestException ();
            }
        }

      if (mixed_trans)
        {
          ::Xyz::FooTypeSupportImpl* fts_servant
            = new ::Xyz::FooTypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = fts_servant;

          if (::DDS::RETCODE_OK != fts_servant->register_type(participant.in (), MY_TYPE_FOR_UDP))
            {
              ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t) register_type failed.\n")));
              throw TestException ();
            }
        }

      ::DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);

      ::DDS::Topic_var topic
        = participant->create_topic(MY_TOPIC,
                                    MY_TYPE,
                                    topic_qos,
                                    ::DDS::TopicListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to create_topic.")));
          throw TestException ();
        }

      ::DDS::TopicDescription_var description
        = participant->lookup_topicdescription(MY_TOPIC);
      if (CORBA::is_nil (description.in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to lookup_topicdescription.")));
          throw TestException ();
        }

      ::DDS::Topic_var topic1;
      ::DDS::TopicDescription_var description1;

      if (mixed_trans)
        {
          topic1 = participant->create_topic (MY_TOPIC_FOR_UDP,
                                              MY_TYPE_FOR_UDP,
                                              topic_qos,
                                              ::DDS::TopicListener::_nil(),
                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (topic1.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) create_topic failed.\n")));
              throw TestException ();
            }

          description1
            = participant->lookup_topicdescription(MY_TOPIC_FOR_UDP);
          if (CORBA::is_nil (description1.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("Failed to lookup_topicdescription.")));
              throw TestException ();
            }
        }

      int attach_to_udp = using_udp;
      int attach_to_multicast = using_multicast;

      // Create the subscriber and attach to the corresponding
      // transport.
      DDS::Subscriber_var sub =
        create_subscriber(participant, attach_to_udp, attach_to_multicast,
                          using_rtps_transport, using_shmem);
      if (CORBA::is_nil (sub.in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to create_subscriber.")));
          throw TestException ();
        }

      ::DDS::Subscriber_var sub1;
      if (mixed_trans)
        {
          // Create the subscriber with a different transport from previous
          // subscriber.
          sub1 = create_subscriber(participant, !attach_to_udp,
                                   attach_to_multicast, false /*rtps*/, false);
          if (CORBA::is_nil (sub1.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("Failed to create_subscriber.")));
              throw TestException ();
            }
        }

      // Create the Datareaders
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      // Make it KEEP_ALL history so we can verify the received
      // data without dropping.
      dr_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dr_qos.reliability.kind =
        (using_udp || mixed_trans)
        ? ::DDS::BEST_EFFORT_RELIABILITY_QOS
        : ::DDS::RELIABLE_RELIABILITY_QOS;
      dr_qos.resource_limits.max_samples_per_instance =
        max_samples_per_instance ;
      // The history depth is only used for KEEP_LAST.
      //dr_qos.history.depth = history_depth  ;

      // activate the listener
      ::DDS::DataReaderListener_var listener (new DataReaderListenerImpl);

      if (CORBA::is_nil (listener.in ()))
        {
          ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) listener is nil.")));
          throw TestException ();
        }

      ::DDS::DataReader_var * drs = new ::DDS::DataReader_var[num_datareaders];

      // Create one datareader or multiple datareaders belonging to the same
      // subscriber.
      for (int i = 0; i < num_datareaders; i ++)
        {
          int attach_to_udp = using_udp;
          ::DDS::Subscriber_var the_sub = sub;
          ::DDS::TopicDescription_var the_description = description;
          // The first datawriter would be using a different transport
          // from others for the diff trans test case.
          if (mixed_trans && i == 0)
            {
              attach_to_udp = ! attach_to_udp;
              the_sub = sub1;
              the_description = description1;
            }

          // create the datareader.
          drs[i] = the_sub->create_datareader(the_description.in (),
                                              dr_qos,
                                              listener.in (),
                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

          if (CORBA::is_nil (drs[i].in ()))
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                              ACE_TEXT("(%P|%t) create_datareader failed.\n")),
                              1);
            }
        }

      // Indicate that the subscriber is ready
      ACE_DEBUG((LM_INFO, "(%P|%t) subscriber signaling ready\n"));
      FILE* readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_TEXT("w"));
      if (readers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to be ready
      FILE* writers_ready = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          ACE_DEBUG((LM_INFO, "(%P|%t) subscriber checking for pub ready\n"));
          writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_TEXT("r"));
        } while (0 == writers_ready);

      ACE_OS::fclose(readers_ready);
      ACE_OS::fclose(writers_ready);

      int num_associations = mixed_trans ? num_datawriters :
                                           num_datareaders * num_datawriters;
      int expected = num_associations *
                     num_instances_per_writer * num_samples_per_instance;

      FILE* writers_completed = 0;
      int timeout_writes = 0;

      while ( num_reads < expected)
        {
          // Get the number of the timed out writes from publisher so we
          // can re-calculate the number of expected messages. Otherwise,
          // the blocking timeout test will never exit from this loop.
          if (writers_completed == 0)
            {
              ACE_DEBUG((LM_INFO, "(%P|%t) subscriber checking for pub finished\n"));
              writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
              if (writers_completed != 0)
                {
                  //writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
                  if (std::fscanf (writers_completed, "%d\n", &timeout_writes) != 1) {
                    //if fscanf return 0 or EOF(-1), failed to read a matching line format to populate in timeout_writes
                    ACE_DEBUG ((LM_DEBUG,
                                ACE_TEXT("(%P|%t) Warning: subscriber could not read timeout_writes\n")));
                  } else if (timeout_writes) {
                    expected -= timeout_writes;
                    ACE_DEBUG((LM_DEBUG,
                             ACE_TEXT ("(%P|%t) %d timed out writes, adjusted we expected to %d\n"),
                             timeout_writes, expected));
                  }
                  // After we got the number of timed out writes, we should speed the
                  // receiving.
                  op_interval_ms = 0;
                }

                ACE_DEBUG((LM_DEBUG,
                           ACE_TEXT ("(%P|%t) received %d of expected %d\n"),
                           num_reads.value(), expected));

            }
          ACE_OS::sleep (1);
        }

      // Indicate that the subscriber is done
      ACE_DEBUG((LM_INFO, "(%P|%t) subscriber signaling finished\n"));
      FILE* readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_TEXT("w"));
      if (readers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to finish
      while (writers_completed == 0)
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          ACE_DEBUG((LM_INFO, "(%P|%t) subscriber checking for pub finished\n"));
          writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
        }

      ACE_OS::fclose(readers_completed);
      ACE_OS::fclose(writers_completed);

      if (results.test_passed (expected) == false)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) Verify received samples - not passed \n")));
          status = 1;
        }

      delete [] drs;
    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main (). ")));
      status = 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main ():");
      status = 1;
    }
  catch (const OpenDDS::DCPS::Transport::MiscProblem &)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Transport::MiscProblem exception caught during processing.\n")
      ));
      status = 1;
    }

    ACE_DEBUG((LM_INFO, "(%P|%t) subscriber starting shutdown\n"));
  try
    {
      if (! CORBA::is_nil (participant.in ()))
        {
          participant->delete_contained_entities();
        }
      if (! CORBA::is_nil (dpf.in ()))
        {
          dpf->delete_participant(participant.in ());
          participant = 0;
          dpf = 0;
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in cleanup.");
      status = 1;
    }

  ACE_DEBUG((LM_INFO, "(%P|%t) subscriber shutdown of ServiceParticipant\n"));
  TheServiceParticipant->shutdown ();
  ACE_DEBUG((LM_INFO, "(%P|%t) subscriber exiting with status %d\n", status));
  return status;
}
