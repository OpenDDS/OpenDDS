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
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "tao/ORB_Core.h"

#include "common.h"

OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;
static const ACE_TCHAR * writer_address_str = ACE_TEXT("localhost:0");
static int writer_address_given = 0;


static int init_writer_tranport ()
{
  int status = 0;

  if (using_udp)
    {
      writer_transport_impl =
          TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                      ACE_TEXT("SimpleUdp"),
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);
      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, ACE_TEXT("SimpleUdp"));

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
                                                      ACE_TEXT("SimpleTcp"),
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, ACE_TEXT("SimpleTcp"));

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


class ReactorCtrl : public ACE_Event_Handler
{
public:
  ReactorCtrl() : cond_(lock_) {}

  int handle_timeout (const ACE_Time_Value &tv,
                      const void *arg)
  {
    ACE_UNUSED_ARG(tv);
    ACE_UNUSED_ARG(arg);

    // it appears that you must have the lock before waiting or signaling on Win32
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      this->lock_,
                      -1);

    return cond_.wait();
  }

  void pause()
  {
    // get the reactor and set the timer.
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();

    ACE_Reactor* reactor ;
    reactor = orb->orb_core()->reactor();

    if (reactor->schedule_timer(this,
                                0,
                                ACE_Time_Value(0,1)) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: PauseReactor, ")
                 ACE_TEXT(" %p. \n"), ACE_TEXT("schedule_timer")));
    }
  }

  void resume()
  {
    // it appears that you must have the lock before waiting or signaling on Win32
    ACE_GUARD (ACE_Recursive_Thread_Mutex,
               guard,
               this->lock_);
    cond_.signal();
  }

private:
  ACE_Recursive_Thread_Mutex lock_;
  ACE_Condition<ACE_Recursive_Thread_Mutex> cond_;
} ;


/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -i num_ops_per_thread       defaults to 1
    //  -l num_unlively_periods     defaults to 10
    //  -w num_datawriters          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -p pub transport address    defaults to localhost:23456
    //  -z                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      num_unlively_periods = ACE_OS::atoi (currentArg);
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
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-p"))) != 0)
    {
      writer_address_str = currentArg;
      writer_address_given = 1;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-u"))) != 0)
    {
      using_udp = ACE_OS::atoi (currentArg);
      if (using_udp == 1)
      {
        ACE_DEBUG((LM_DEBUG, "Publisher Using UDP transport.\n"));
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


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      ::Xyz::FooTypeSupport_var fts(new ::Xyz::FooTypeSupportImpl);

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

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dw_qos.liveliness.lease_duration.nanosec = 0 ;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                  dw_qos,
                                  ::DDS::DataWriterListener::_nil());

      if (CORBA::is_nil (dw.in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1 ;
        }

      // ensure that the connection and association has been fully established
      ACE_OS::sleep(2);  //TBD remove this kludge when the transport is fixed.

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
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_LIB_TEXT("r"));
        } while (0 == readers_ready);

      ACE_OS::fclose(writers_ready);
      ACE_OS::fclose(readers_ready);

      ReactorCtrl rc ;

      // stop the Service_Participant reactor so LIVELINESS.kind=AUTOMATIC does not
      // send out an automatic liveliness control message when sleeping in the loop
      // below.
      rc.pause() ;

      Writer* writer = new Writer(dw.in (),
                                1,
                                num_ops_per_thread);

      for (int i = 0 ; i < num_unlively_periods ; i++)
        {
          writer->run_test (i);

          // 3 ensures that we will detect when an DataReader detects
          // liveliness lost on an already unliveliy DataReader.
          ACE_OS::sleep (3 * LEASE_DURATION_SEC);
        }
      writer->run_test (num_unlively_periods);

      rc.resume() ;

      bool writers_finished = false;

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T waiting for writers to finish\n") ));
      while ( !writers_finished )
        {
          ACE_OS::sleep(ACE_Time_Value(0,250000));
          writers_finished = true;
          writers_finished = writers_finished && writer->is_finished();
        }

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T Writers are finished\n") ));

      // Indicate that the publisher is done
      FILE* writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_LIB_TEXT("w"));
      if (writers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create publisher completed file\n")));
        }


      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T waiting for readers to finish\n") ));

      // Wait for the subscriber to finish.
      FILE* readers_completed = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_LIB_TEXT("r"));
        } while (0 == readers_completed);

      ACE_OS::fclose(writers_completed);
      ACE_OS::fclose(readers_completed);

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T Readers are finished\n") ));

      // Clean up publisher objects
      pub->delete_contained_entities() ;

      delete writer;

      dp->delete_publisher(pub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      // Moved TransportImpl reference release from just before exit from main
      // to here. This intended to fix the access violation in some optimize
      // build on linux during shutdown. I think TransportImpl object cleanup
      // may reference some resouces that already released.
      writer_transport_impl = 0;

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
