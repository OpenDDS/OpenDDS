// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "../common/TestException.h"
#include "DataReaderListener1.h"
#include "DataReaderListener2.h"
#include "DataReaderListener3.h"
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

#include "ace/Arg_Shifter.h"

#include "common.h"

OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
static const ACE_TCHAR * reader_address_str = ACE_TEXT("localhost:0");
static int reader_address_given = 0;

static int topics = 0 ;

static int init_reader_tranport ()
{
  int status = 0;

  if (using_udp)
    {
      reader_transport_impl =
          TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      ACE_TEXT("udp"),
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, ACE_TEXT("udp"));

      OpenDDS::DCPS::UdpConfiguration* reader_udp_config
        = static_cast <OpenDDS::DCPS::UdpConfiguration*> (reader_config.in ());

      if (!reader_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 11;
        }

      ACE_INET_Addr reader_address (reader_address_str);
      reader_udp_config->local_address_ = reader_address;

      if (reader_transport_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }
  else
    {
      reader_transport_impl =
          TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      ACE_TEXT("tcp"),
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, ACE_TEXT("tcp"));

      OpenDDS::DCPS::TcpConfiguration* reader_tcp_config
        = static_cast <OpenDDS::DCPS::TcpConfiguration*> (reader_config.in ());

      if (reader_address_given)
        {
          ACE_INET_Addr reader_address (reader_address_str);
          reader_tcp_config->local_address_ = reader_address;
          reader_tcp_config->local_address_str_ = reader_address_str;
        }
        // else use default address - OS assigned.

      if (reader_transport_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub TCP ")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  return status;
}


/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -i num_ops_per_thread       defaults to 10
    //  -r num_datareaders          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -s sub transport address    defaults to localhost:23456
    //  -z                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
    {
      if (!ACE_OS::strcmp(currentArg, ACE_TEXT("all")))
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
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0)
    {
      reader_address_str = currentArg;
      reader_address_given = 1;
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
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
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


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

//      TheServiceParticipant->liveliness_factor(100) ;

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
          fts2 = new ::T2::Foo2TypeSupportImpl;
        }

      if (topics & TOPIC_T3)
        {
          fts3 = new ::T3::Foo3TypeSupportImpl;
        }

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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
                                     ::DDS::TopicListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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
                                     ::DDS::TopicListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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
                                     ::DDS::TopicListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

          if (CORBA::is_nil (topic3.in ()))
            {
              return 1 ;
            }
        }

      ::DDS::TopicDescription_var description1 ;
      ::DDS::TopicDescription_var description2 ;
      ::DDS::TopicDescription_var description3 ;

      if (topics & TOPIC_T1)
        {
          description1 = dp->lookup_topicdescription(MY_TOPIC1);
          if (CORBA::is_nil (description1.in ()))
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                      ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                      1);
            }
        }

      if (topics & TOPIC_T2)
        {
          description2 = dp->lookup_topicdescription(MY_TOPIC2);
          if (CORBA::is_nil (description2.in ()))
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                      ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                      1);
            }
        }

      if (topics & TOPIC_T3)
        {
          description3 = dp->lookup_topicdescription(MY_TOPIC3);
          if (CORBA::is_nil (description3.in ()))
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                      ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                      1);
            }
        }

      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Initialize the transport
      if (0 != ::init_reader_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) init_transport failed!\n")),
                           1);
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl
        = dynamic_cast<OpenDDS::DCPS::SubscriberImpl*> (sub.in ());

      if (0 == sub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::OpenDDS::DCPS::SubscriberImpl\n")),
                          1);
      }

      OpenDDS::DCPS::AttachStatus attach_status =
        sub_impl->attach_transport(reader_transport_impl.in());

      if (attach_status != OpenDDS::DCPS::ATTACH_OK)
        {
          // We failed to attach to the transport for some reason.
          ACE_TString status_str;

          switch (attach_status)
            {
              case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = ACE_TEXT("ATTACH_BAD_TRANSPORT");
                break;
              case OpenDDS::DCPS::ATTACH_ERROR:
                status_str = ACE_TEXT("ATTACH_ERROR");
                break;
              case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = ACE_TEXT("ATTACH_INCOMPATIBLE_QOS");
                break;
              default:
                status_str = ACE_TEXT("Unknown Status");
                break;
            }

          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to attach to the transport. ")
                            ACE_TEXT("AttachStatus == %s\n"),
                            status_str.c_str()),
                            1);
        }


      // Create the Datareaders
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dr_qos.liveliness.lease_duration.nanosec = 0 ;


      ::DDS::DataReader_var dr1 ;
      ::DDS::DataReader_var dr2 ;
      ::DDS::DataReader_var dr3 ;

      ::DDS::DataReaderListener_var drl1 (new DataReaderListenerImpl1);
      ::DDS::DataReaderListener_var drl2 (new DataReaderListenerImpl2);
      ::DDS::DataReaderListener_var drl3 (new DataReaderListenerImpl3);
      if (topics & TOPIC_T1)
        {
          dr1 = sub->create_datareader(description1.in (),
                                  dr_qos,
                                  drl1.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        }

      if (topics & TOPIC_T2)
        {
          dr2 = sub->create_datareader(description2.in (),
                                  dr_qos,
                                  drl2.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        }

      if (topics & TOPIC_T3)
        {
          dr3 = sub->create_datareader(description3.in (),
                                  dr_qos,
                                  drl3.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        }
/*
      // Indicate that the subscriber is ready
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
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_TEXT("r"));
        } while (0 == writers_ready);

      ACE_OS::fclose(readers_ready);
      ACE_OS::fclose(writers_ready);
*/
      // Indicate that the subscriber is done
      if (topics & TOPIC_T1)
        {
          ACE_TString t1_fn = ACE_TEXT(MY_TOPIC1) + sub_finished_filename ;
          FILE* readers_completed =
              ACE_OS::fopen (t1_fn.c_str (), ACE_TEXT("w"));
          if (readers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
            }
          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T2)
        {
          ACE_TString t2_fn = ACE_TEXT(MY_TOPIC2) + sub_finished_filename ;
          FILE* readers_completed =
              ACE_OS::fopen (t2_fn.c_str (), ACE_TEXT("w"));
          if (readers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
            }
          ACE_OS::fclose(readers_completed);
        }

      if (topics & TOPIC_T3)
        {
          ACE_TString t3_fn = ACE_TEXT(MY_TOPIC3) + sub_finished_filename ;
          FILE* readers_completed =
              ACE_OS::fopen (t3_fn.c_str (), ACE_TEXT("w"));
          if (readers_completed == 0)
            {
              ACE_ERROR ((LM_ERROR,
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
              ACE_OS::sleep (small_time);
              writers_completed =
                  ACE_OS::fopen (t1_fn.c_str (), ACE_TEXT("r"));
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
              ACE_OS::sleep (small_time);
              writers_completed =
                  ACE_OS::fopen (t2_fn.c_str (), ACE_TEXT("r"));
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
              ACE_OS::sleep (small_time);
              writers_completed =
                  ACE_OS::fopen (t3_fn.c_str (), ACE_TEXT("r"));
            } while (0 == writers_completed);

          ACE_OS::fclose(writers_completed);
        }

      if (topics & TOPIC_T1)
        {
          DataReaderListenerImpl1* drl_servant1 =
            dynamic_cast<DataReaderListenerImpl1*>(drl1.in());
          ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC1,
                        drl_servant1->num_samples()) ;
          if (drl_servant1->num_samples() != num_ops_per_thread)
            {
              ACE_OS::fprintf(stderr,
                              "%s: Expected %d samples, got %d samples.\n",
                              MY_TOPIC1,
                              num_ops_per_thread, drl_servant1->num_samples());
              return 1;
            }
        }
      if (topics & TOPIC_T2)
        {
          DataReaderListenerImpl2* drl_servant2 =
            dynamic_cast<DataReaderListenerImpl2*>(drl2.in());
          ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC2,
                        drl_servant2->num_samples()) ;
          if (drl_servant2->num_samples() != num_ops_per_thread)
            {
              ACE_OS::fprintf(stderr,
                              "%s: Expected %d samples, got %d samples.\n",
                              MY_TOPIC2,
                              num_ops_per_thread, drl_servant2->num_samples());
              return 1;
            }
        }
      if (topics & TOPIC_T3)
        {
          DataReaderListenerImpl3* drl_servant3 =
            dynamic_cast<DataReaderListenerImpl3*>(drl3.in());
          ACE_OS::printf("\n*** %s received %d samples.\n", MY_TOPIC3,
                        drl_servant3->num_samples()) ;
          if (drl_servant3->num_samples() != num_ops_per_thread)
            {
              ACE_OS::fprintf(stderr,
                              "%s: Expected %d samples, got %d samples.\n",
                              MY_TOPIC3,
                              num_ops_per_thread, drl_servant3->num_samples());
              return 1;
            }
        }

      // clean up subscriber objects

      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in ());

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
