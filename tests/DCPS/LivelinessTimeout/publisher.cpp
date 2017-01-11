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

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"

#include "common.h"

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -l lease duration     defaults to 10
    //  -x test duration in sec     defaults to 40
    //  -z                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if  ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      LEASE_DURATION_SEC = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-x"))) != 0)
    {
      test_duration = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
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

      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
      dw_qos.liveliness.lease_duration.nanosec = 0;

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
      pub->delete_contained_entities() ;

      dp->delete_publisher(pub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

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
