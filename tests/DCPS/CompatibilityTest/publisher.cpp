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
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "DataWriterListenerImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Reactor.h"

#include "common.h"


/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
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

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-c"))) != 0)
    {
      compatible = (ACE_TString(currentArg) == ACE_TEXT ("true"));
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-d"))) != 0)
    {
      durability_kind_str = currentArg;
      durability_kind = ::get_durability_kind(durability_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-k"))) != 0)
    {
      liveliness_kind_str = currentArg;
      liveliness_kind = ::get_liveliness_kind(liveliness_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-l"))) != 0)
    {
      LEASE_DURATION_STR = currentArg;
      LEASE_DURATION = ::get_lease_duration(LEASE_DURATION_STR);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-r"))) != 0)
    {
      reliability_kind_str = currentArg;
      reliability_kind = ::get_reliability_kind(reliability_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-x"))) != 0)
    {
      test_duration = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT ("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
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
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
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
                          ::DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_topic failed!\n")),
                           1);
      }

      OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.durability.kind = durability_kind;
      dw_qos.liveliness.kind = liveliness_kind;
      dw_qos.liveliness.lease_duration = LEASE_DURATION;
      dw_qos.reliability.kind = reliability_kind;

      ::DDS::DataWriterListener_var dwl (new DataWriterListenerImpl);

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                  dw_qos,
                                  dwl.in(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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
      dp->delete_contained_entities();
      dpf->delete_participant(dp.in ());
      TheServiceParticipant->shutdown();

      {
        DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*>(dwl.ptr());
        // check to see if the publisher worked
        if(dwl_servant->publication_matched() != compatible)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it wasn't. ")
                      ACE_TEXT("durability_kind=%s,liveliness_kind=%s,liveliness_duration=%s,")
                      ACE_TEXT("reliability_kind=%s\n"),
                      (compatible) ? "true" : "false",
                      durability_kind_str.c_str(),
                      liveliness_kind_str.c_str(),
                      LEASE_DURATION_STR.c_str(),
                      reliability_kind_str.c_str()));
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
