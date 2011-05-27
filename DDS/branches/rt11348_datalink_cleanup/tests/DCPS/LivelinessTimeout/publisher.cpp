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
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "DataWriterListenerImpl.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "tao/ORB_Core.h"

#include "common.h"

static const char * writer_address_str = "";
static int writer_address_given = 0;

static int init_writer_tranport (OpenDDS::DCPS::TransportImpl_rch& writer_transport_impl)
{
  int status = 0;

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
    //  -l lease duration     defaults to 10
    //  -x test duration in sec     defaults to 40
    //  -z                          verbose transport debug

    const char *currentArg = 0;

    if  ((currentArg = arg_shifter.get_the_parameter("-l")) != 0)
    {
      LEASE_DURATION_SEC = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-x")) != 0)
    {
      test_duration = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
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
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

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

      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          TOPIC_QOS_DEFAULT,
                          ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ())) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_topic failed!\n")),
                           1);
      }

      OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

      // Initialize the transport
      if (0 != ::init_writer_tranport(writer_transport_impl) )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) init_transport failed!\n")),
                           1);
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

      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw_qos.liveliness.lease_duration.nanosec = 0;

      ::DDS::DataWriterListener_var dwl (new DataWriterListenerImpl);

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                  dw_qos,
                                  dwl.in());

      if (CORBA::is_nil (dw.in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1 ;
        }


//      Writer writer(dw.in ());

//      ACE_Time_Value duration(10);
//      writer.run_test(duration);
      ACE_OS::sleep(test_duration);

      // Clean up publisher objects
      pub->delete_contained_entities() ;

      dp->delete_publisher(pub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

      // check to see if the publisher worked
      {
        DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*>(dwl.in());
        if(!dwl_servant->valid())
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) publisher didn't connect with subscriber. test_duration=%d\n"),
                     test_duration));
          return 1 ;
        }
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
