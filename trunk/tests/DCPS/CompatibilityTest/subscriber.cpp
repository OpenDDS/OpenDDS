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
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif

#include "ace/Arg_Shifter.h"

#include "common.h"

static int init_reader_tranport (OpenDDS::DCPS::TransportImpl_rch& reader_transport_impl)
{
  int status = 0;

  reader_transport_impl =
      TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                  "SimpleTcp",
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch reader_config
    = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleTcp");

  if (reader_transport_impl->configure(reader_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) init_reader_tranport: sub TCP ")
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
    //  -c expect compatibility     defaults to false
    //  -d durability kind          defaults to TRANSIENT_LOCAL_DURABILITY_QOS
    //  -k liveliness kind          defaults to AUTOMATIC_LIVELINESS_QOS
    //  -l lease duration           no default
    //  -r reliability kind         defaults to TRANSIENT_LOCAL_DURABILITY_QOS
    //  -x test duration in sec     defaults to 40
    //  -z                          verbose transport debug

    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-c")) != 0)
    {
      compatible = (std::string(currentArg) == "true") ? true : false;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0)
    {
      durability_kind_str = currentArg;
      durability_kind = ::get_durability_kind(durability_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-k")) != 0)
    {
      liveliness_kind_str = currentArg;
      liveliness_kind = ::get_liveliness_kind(liveliness_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-l")) != 0)
    {
      LEASE_DURATION_STR = currentArg;
      LEASE_DURATION = ::get_lease_duration(LEASE_DURATION_STR);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-r")) != 0)
    {
      reliability_kind_str = currentArg;
      reliability_kind = ::get_reliability_kind(reliability_kind_str);
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
      OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
      if (0 != ::init_reader_tranport(reader_transport_impl) )
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

      dr_qos.durability.kind = durability_kind;
      dr_qos.liveliness.kind = liveliness_kind;
      dr_qos.liveliness.lease_duration = LEASE_DURATION;
      dr_qos.reliability.kind = reliability_kind;

      ::DDS::DataReaderListener_var drl (new DataReaderListenerImpl);
      DataReaderListenerImpl* drl_servant =
        dynamic_cast<DataReaderListenerImpl*>(drl.in());

      ::DDS::DataReader_var dr(sub->create_datareader(description.in (),
                                                      dr_qos,
                                                      drl.in ()));

      ACE_OS::sleep(test_duration);

      // clean up subscriber objects


      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

      // there is an error if we matched when not compatible (or vice-versa)
      if (drl_servant->subscription_matched() != compatible)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) Expected subscription_matched to be %s, but it wasn't."
                             "durability_kind=%s,liveliness_kind=%s,liveliness_duration=%s,"
                             "reliability_kind=%s\n"),
                    (compatible) ? "true" : "false",
                    durability_kind_str.c_str(),
                    liveliness_kind_str.c_str(),
                    LEASE_DURATION_STR.c_str(),
                    reliability_kind_str.c_str()));
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
