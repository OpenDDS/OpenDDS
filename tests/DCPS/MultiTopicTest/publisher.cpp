// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
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
#include "tests/DCPS/MultiTopicTypes/Foo1DefTypeSupportImpl.h"
#include "tests/DCPS/MultiTopicTypes/Foo2DefTypeSupportImpl.h"
#include "tests/DCPS/MultiTopicTypes/Foo3DefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_time.h"

#include "common.h"

OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;
static const char * writer_address_str = "localhost:0";
static int writer_address_given = 0;

static int topics = 0 ;

static int init_writer_tranport ()
{
  int status = 0;

  if (using_udp)
    {
      writer_transport_impl =
          TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                      "SimpleUdp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleUdp");

      OpenDDS::DCPS::SimpleUdpConfiguration* writer_udp_config
        = static_cast <OpenDDS::DCPS::SimpleUdpConfiguration*> (writer_config.in ());

      if (!writer_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: pub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 12;
        }

      ACE_INET_Addr writer_address (writer_address_str);
      writer_udp_config->local_address_ = writer_address;
      writer_udp_config->local_address_str_ = writer_address_str;

      if (writer_transport_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }
  else
    {
      writer_transport_impl =
          TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                      "SimpleTcp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleTcp");

      OpenDDS::DCPS::SimpleTcpConfiguration* writer_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (writer_config.in ());

      if (writer_address_given)
        {
          ACE_INET_Addr writer_address (writer_address_str);
          writer_tcp_config->local_address_ = writer_address;
          writer_tcp_config->local_address_str_ = writer_address_str;
        }
        // else use default address - OS assigned.

      if (writer_transport_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub TCP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  return status;
}


/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -i num_ops_per_thread       defaults to 10
    //  -w num_datawriters          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -p pub transport address    defaults to localhost:23456
    //  -z                          verbose transport debug

    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0)
    {
      if (!ACE_OS::strcmp(currentArg, "all"))
        {
          topics = TOPIC_T1 | TOPIC_T2 | TOPIC_T3 ;
        }
      else
        {
          int t = ACE_OS::atoi (currentArg);
          arg_shifter.consume_arg ();

          switch(t)
          {
            case 1:
              topics |= TOPIC_T1 ;
              break ;

            case 2:
              topics |= TOPIC_T2 ;
              break ;

            case 3:
              topics |= TOPIC_T3 ;
              break ;

            default:
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Invalid topic id (must be 1, 2, or 3).\n")));
              return 1 ;
          }
        }
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-p")) != 0)
    {
      writer_address_str = currentArg;
      writer_address_given = 1;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-u")) != 0)
    {
      using_udp = ACE_OS::atoi (currentArg);
      if (using_udp == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Publisher Using UDP transport.\n"));
      }
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{

  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      if (!topics)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) Must run with one or more of the following: -t1 -t2 -t3\n")));
        return 1 ;
      }

      ::T1::Foo1TypeSupport_var fts1 ;
      ::T2::Foo2TypeSupport_var fts2 ;
      ::T3::Foo3TypeSupport_var fts3 ;

      if (topics & TOPIC_T1)
        {
          fts1 = new ::T1::Foo1TypeSupportImpl;
        }

      if (topics & TOPIC_T2)
        {
          fts2 = new ::T2::Foo2TypeSupportImpl();
        }

      if (topics & TOPIC_T3)
        {
          fts3 = new ::T3::Foo3TypeSupportImpl();
        }

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1 ;
      }

      if (topics & TOPIC_T1)
        {
          if (::DDS::RETCODE_OK != fts1->register_type(dp.in (), MY_TYPE1))
            {
              ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("Failed to register the Foo1TypeSupport.")));
              return 1;
            }
        }

      if (topics & TOPIC_T2)
        {
          if (::DDS::RETCODE_OK != fts2->register_type(dp.in (), MY_TYPE2))
            {
              ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("Failed to register the Foo2TypeSupport.")));
              return 1;
            }
        }

      if (topics & TOPIC_T3)
        {
          if (::DDS::RETCODE_OK != fts3->register_type(dp.in (), MY_TYPE3))
            {
              ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("Failed to register the Foo3TypeSupport.")));
              return 1;
            }
        }

      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic1 ;
      ::DDS::Topic_var topic2 ;
      ::DDS::Topic_var topic3 ;

      if (topics & TOPIC_T1)
        {
          topic1 = dp->create_topic (MY_TOPIC1,
                                     MY_TYPE1,
                                     topic_qos,
                                     ::DDS::TopicListener::_nil());

          if (CORBA::is_nil (topic1.in ()))
            {
              return 1 ;
            }
        }

      if (topics & TOPIC_T2)
        {
          topic2 = dp->create_topic (MY_TOPIC2,
                                     MY_TYPE2,
                                     topic_qos,
                                     ::DDS::TopicListener::_nil());

          if (CORBA::is_nil (topic2.in ()))
            {
              return 1 ;
            }
        }

      if (topics & TOPIC_T3)
        {
          topic3 = dp->create_topic (MY_TOPIC3,
                                     MY_TYPE3,
                                     topic_qos,
                                     ::DDS::TopicListener::_nil());

          if (CORBA::is_nil (topic3.in ()))
            {
              return 1 ;
            }
        }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Initialize the transport
      if (0 != ::init_writer_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) init_transport failed!\n")),
                           1);
      }

      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* pub_impl
        = dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub.in ());

      if (0 == pub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::OpenDDS::DCPS::PublisherImpl\n")),
                          1);
      }

      OpenDDS::DCPS::AttachStatus attach_status =
        pub_impl->attach_transport(writer_transport_impl.in());

      if (attach_status != OpenDDS::DCPS::ATTACH_OK)
        {
          // We failed to attach to the transport for some reason.
          ACE_TString status_str;

          switch (attach_status)
            {
              case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = "ATTACH_BAD_TRANSPORT";
                break;
              case OpenDDS::DCPS::ATTACH_ERROR:
                status_str = "ATTACH_ERROR";
                break;
              case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = "ATTACH_INCOMPATIBLE_QOS";
                break;
              default:
                status_str = "Unknown Status";
                break;
            }

          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to attach to the transport. ")
                            ACE_TEXT("AttachStatus == %s\n"),
                            status_str.c_str()),
                            1);
        }

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dw_qos.liveliness.lease_duration.nanosec = 0 ;

      ::DDS::DataWriter_var dw1 ;
      ::DDS::DataWriter_var dw2 ;
      ::DDS::DataWriter_var dw3 ;

      if (topics & TOPIC_T1)
        {
          dw1 = pub->create_datawriter(topic1.in (),
                                      dw_qos,
                                      ::DDS::DataWriterListener::_nil());

          if (CORBA::is_nil (dw1.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                       ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
              return 1 ;
            }
        }

      if (topics & TOPIC_T2)
        {
          dw2 = pub->create_datawriter(topic2.in (),
                                      dw_qos,
                                      ::DDS::DataWriterListener::_nil());

          if (CORBA::is_nil (dw2.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                       ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
              return 1 ;
            }
        }

      if (topics & TOPIC_T3)
        {
          dw3 = pub->create_datawriter(topic3.in (),
                                      dw_qos,
                                      ::DDS::DataWriterListener::_nil());

          if (CORBA::is_nil (dw3.in ()))
            {
              ACE_ERROR ((LM_ERROR,
                       ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
              return 1 ;
            }
        }
/*
      // Indicate that the publisher is ready
      FILE* writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_LIB_TEXT("w"));
      if (writers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create publisher ready file\n")));
        }

      // Wait for the subscriber to be ready.
      FILE* readers_ready = 0;
      do
        {
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_LIB_TEXT("r"));
        } while (0 == readers_ready);

      ACE_OS::fclose(writers_ready);
      ACE_OS::fclose(readers_ready);
*/
      int num_writers(0) ;

      if (topics & TOPIC_T1)
      {
        num_writers++ ;
      }
      if (topics & TOPIC_T2)
      {
        num_writers++ ;
      }
      if (topics & TOPIC_T3)
      {
        num_writers++ ;
      }

      Writer **writers = new Writer* [num_writers] ;

      int idx(0) ;

      if (topics & TOPIC_T1)
      {
        writers[idx++] = new Writer(dw1.in (),
                                    1,
                                    num_ops_per_thread);
      }

      if (topics & TOPIC_T2)
      {
        writers[idx++] = new Writer(dw2.in (),
                                    1,
                                    num_ops_per_thread);
      }

      if (topics & TOPIC_T3)
      {
        writers[idx++] = new Writer(dw3.in (),
                                    1,
                                    num_ops_per_thread);
      }

      ACE_OS::srand((unsigned) ACE_OS::time(NULL));

      for (int i = 0 ; i < num_writers ; i++)
      {
        writers[i]->start ();
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
                                    pub_finished_filename ;
          FILE* writers_completed =
                ACE_OS::fopen (t1_filename.c_str (), ACE_LIB_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      if (topics & TOPIC_T2)
        {
          // Indicate that the publisher is done
          ACE_TString t2_filename = ACE_TEXT(MY_TOPIC2) +
                                    pub_finished_filename ;
          FILE* writers_completed =
                ACE_OS::fopen (t2_filename.c_str (), ACE_LIB_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      if (topics & TOPIC_T3)
        {
          // Indicate that the publisher is done
          ACE_TString t3_filename = ACE_TEXT(MY_TOPIC3) +
                                    pub_finished_filename ;
          FILE* writers_completed =
                ACE_OS::fopen (t3_filename.c_str (), ACE_LIB_TEXT("w"));
          if (writers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create publisher completed file\n")));
            }
            ACE_OS::fclose(writers_completed);
        }

      // Wait for the subscriber to finish.
      if (topics & TOPIC_T1)
        {
          FILE* readers_completed = 0;
          ACE_TString t1_filename = ACE_TEXT(MY_TOPIC1) +
                                    sub_finished_filename ;
          do
            {
              ACE_Time_Value small(0,250000);
              ACE_OS::sleep (small);
              readers_completed =
                  ACE_OS::fopen (t1_filename.c_str (), ACE_LIB_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T2)
        {
          FILE* readers_completed = 0;
          ACE_TString t2_filename = ACE_TEXT(MY_TOPIC2) +
                                    sub_finished_filename ;
          do
            {
              ACE_Time_Value small(0,250000);
              ACE_OS::sleep (small);
              readers_completed =
                  ACE_OS::fopen (t2_filename.c_str (), ACE_LIB_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T3)
        {
          FILE* readers_completed = 0;
          ACE_TString t3_filename = ACE_TEXT(MY_TOPIC3) +
                                    sub_finished_filename ;
          do
            {
              ACE_Time_Value small(0,250000);
              ACE_OS::sleep (small);
              readers_completed =
                  ACE_OS::fopen (t3_filename.c_str (), ACE_LIB_TEXT("r"));
            } while (0 == readers_completed);

          ACE_OS::fclose(readers_completed);
        }

      // Clean up publisher objects
      pub->delete_contained_entities() ;

      for (int n = 0 ; n < num_writers ; n++)
        {
          delete writers[n] ;
        }
      delete [] writers;

      dp->delete_publisher(pub.in ());

      if (topics & TOPIC_T1)
        {
          dp->delete_topic(topic1.in ());
        }
      if (topics & TOPIC_T2)
        {
          dp->delete_topic(topic2.in ());
        }
      if (topics & TOPIC_T3)
        {
          dp->delete_topic(topic3.in ());
        }

      dp->delete_contained_entities();
      dpf->delete_participant(dp.in ());

      TheTransportFactory->release();
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
