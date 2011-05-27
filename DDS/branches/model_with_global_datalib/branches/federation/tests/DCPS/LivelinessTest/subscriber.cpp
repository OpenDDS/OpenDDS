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
#include "DataReaderListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h"
#endif

#include "ace/Arg_Shifter.h"

#include "common.h"

OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
static const char * reader_address_str = "localhost:0";
static int reader_address_given = 0;

static int init_reader_tranport ()
{
  int status = 0;

  if (using_udp)
    {
      reader_transport_impl =
          TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      "SimpleUdp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleUdp");

      OpenDDS::DCPS::SimpleUdpConfiguration* reader_udp_config
        = static_cast <OpenDDS::DCPS::SimpleUdpConfiguration*> (reader_config.in ());

      if (!reader_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 11;
        }


      ACE_INET_Addr reader_address (reader_address_str);
      reader_udp_config->local_address_ = reader_address;
      reader_udp_config->local_address_str_ = reader_address_str;


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
                                                      "SimpleTcp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleTcp");

      OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

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
int parse_args (int argc, char *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -t use_take?1:0             defaults to 0
    //  -i num_ops_per_thread       defaults to 1
    //  -l num_unlively_periods     defaults to 10
    //  -r num_datareaders          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -s sub transport address    defaults to localhost:23456
    //  -z                          verbose transport debug

    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-l")) != 0)
    {
      num_unlively_periods = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0)
    {
      use_take = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-s")) != 0)
    {
      reader_address_str = currentArg;
      reader_address_given = 1;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-u")) != 0)
    {
      using_udp = ACE_OS::atoi (currentArg);
      if (using_udp == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Subscriber Using UDP transport.\n"));
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
      ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

//      TheServiceParticipant->liveliness_factor(100) ;

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      ::Xyz::FooTypeSupport_var fts (new ::Xyz::FooTypeSupportImpl);

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

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to register the FooTypeSupport.")));
          return 1;
        }


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC);
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                           1);
      }



      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil());
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


      // Create the Datareaders
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dr_qos.liveliness.lease_duration.nanosec = 0 ;

      ::DDS::DataReaderListener_var drl (new DataReaderListenerImpl);
      DataReaderListenerImpl* drl_servant =
        dynamic_cast<DataReaderListenerImpl*>(drl.in());

      ::DDS::DataReader_var dr ;

      dr = sub->create_datareader(description.in (),
                                  dr_qos,
                                  drl.in ());

      // Indicate that the subscriber is ready
      FILE* readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_LIB_TEXT("w"));
      if (readers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create subscriber completed file\n")));
        }

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T waiting for publisher to be ready\n") ));

      // Wait for the publisher to be ready
      FILE* writers_ready = 0;
      do
        {
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_LIB_TEXT("r"));
        } while (0 == writers_ready);

      ACE_OS::fclose(readers_ready);
      ACE_OS::fclose(writers_ready);

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T Publisher is ready\n") ));

      // Indicate that the subscriber is done
      // (((it is done when ever the publisher is done)))
      FILE* readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_LIB_TEXT("w"));
      if (readers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create subscriber completed file\n")));
        }

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T waiting for publisher to finish\n") ));
      // Wait for the publisher to finish
      FILE* writers_completed = 0;
      do
        {
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_LIB_TEXT("r"));
        } while (0 == writers_completed);

      ACE_OS::fclose(readers_completed);
      ACE_OS::fclose(writers_completed);

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T publisher is finish - cleanup subscriber\n") ));

      // clean up subscriber objects

      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      // Moved TransportImpl reference release from just before exit from main
      // to here. This intended to fix the access violation in some optimize
      // build on linux during shutdown. I assume TransportImpl object cleanup
      // may reference some resouces that already released.
      reader_transport_impl = 0;

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

      ACE_OS::fprintf (stderr, "**********\n") ;
      ACE_OS::fprintf (stderr, "drl_servant->liveliness_changed_count() = %d\n",
                     drl_servant->liveliness_changed_count()) ;
      ACE_OS::fprintf (stderr, "drl_servant->no_writers_generation_count() = %d\n",
                     drl_servant->no_writers_generation_count()) ;
      ACE_OS::fprintf (stderr, "********** use_take=%d\n", use_take) ;
      
      if ((drl_servant->liveliness_changed_count() < 2 + 2 * num_unlively_periods) ||
          (drl_servant->verify_last_liveliness_status () == false) ||
          (drl_servant->no_writers_generation_count() != (use_take==1 ? 0 : num_unlively_periods) ))
      {
        // if use take then the instance had "no samples" when it got NO_WRITERS and
        // hence the instance state terminated and then started again so
        // no_writers_generation_count should = 0.
        ACE_ERROR ((LM_ERROR,
           ACE_TEXT("(%P|%t) Test failed. \n")));
          return 1;
      }

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
