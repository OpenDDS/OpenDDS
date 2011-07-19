// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "common.h"
#include "../common/TestSupport.h"

#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "ace/SString.h"
#include "ace/Arg_Shifter.h"

#include <stdexcept>
#include <string>




DDS::Duration_t LEASE_DURATION;

int test_duration = 40;
::DDS::ReliabilityQosPolicyKind reliability_kind = ::DDS::RELIABLE_RELIABILITY_QOS;
::DDS::DurabilityQosPolicyKind durability_kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
::DDS::LivelinessQosPolicyKind liveliness_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
bool compatible = false;

ACE_TString LEASE_DURATION_STR;
ACE_TString entity_str;
ACE_TString collocation_str;
ACE_TString source_str;
std::string configuration_str;
std::string test_duration_str;
ACE_TString reliability_kind_str;
ACE_TString durability_kind_str;
ACE_TString liveliness_kind_str;
std::string protocol_str;

const ACE_TString& get_collocation_kind(const ACE_TString& argument)
{
  if(argument == ACE_TEXT("none"))
  {
	    return argument;
  }
  else if(argument == ACE_TEXT("memory"))
  {
	    return argument;
  }
  else if(argument == ACE_TEXT("host"))
  {
	    return argument;
  }
  else if(argument == ACE_TEXT("network"))
  {
	    return argument;
  }
  throw std::runtime_error("invalid string for get_collocation_kind");
}

const ACE_TString& get_entity_kind(const ACE_TString& argument)
{
  if(argument == ACE_TEXT("none"))
  {
	    return argument;
  }
  else if(argument == ACE_TEXT("pubsub"))
  {
	    return argument;
  }
  else if(argument == ACE_TEXT("participant"))
  {
	    return argument;
  }
  throw std::runtime_error("invalid string for get_entity_kind");
}

::DDS::LivelinessQosPolicyKind get_liveliness_kind(const ACE_TString& argument)
{
  if(argument == ACE_TEXT("automatic"))
  {
    return ::DDS::AUTOMATIC_LIVELINESS_QOS;
  }
  else if(argument == ACE_TEXT("participant"))
  {
    return ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  }
  else if(argument == ACE_TEXT("topic"))
  {
    return ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  }
  throw std::runtime_error("invalid string for get_liveliness_kind");
}

::DDS::DurabilityQosPolicyKind get_durability_kind(const ACE_TString& argument)
{
  if(argument == ACE_TEXT("volatile"))
  {
    return ::DDS::VOLATILE_DURABILITY_QOS;
  }
  else if(argument == ACE_TEXT("transient_local"))
  {
    return ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  }
  else if(argument == ACE_TEXT("transient"))
  {
    return ::DDS::TRANSIENT_DURABILITY_QOS;
  }
  else if(argument == ACE_TEXT("persistent"))
  {
    return ::DDS::PERSISTENT_DURABILITY_QOS;
  }

  std::string message("invalid string for get_durability_kind: ");
  throw std::runtime_error(message + argument.c_str());
}

::DDS::ReliabilityQosPolicyKind get_reliability_kind(const ACE_TString& argument)
{
  if(argument == ACE_TEXT("best_effort"))
  {
    return ::DDS::BEST_EFFORT_RELIABILITY_QOS;
  }
  else if(argument == ACE_TEXT("reliable"))
  {
    return ::DDS::RELIABLE_RELIABILITY_QOS;
  }
  throw std::runtime_error("invalid string for get_reliability_kind");
}

::DDS::Duration_t get_lease_duration(const ACE_TString& argument)
{
  ::DDS::Duration_t lease;
  if(argument == ACE_TEXT("infinite"))
  {
    lease.sec = ::DDS::DURATION_INFINITE_SEC;
    lease.nanosec = ::DDS::DURATION_INFINITE_NSEC;
  }
  else
  {
    lease.sec = ACE_OS::atoi (argument.c_str());
    lease.nanosec = 0;
  }
  return lease;
}

/// parse the command line arguments
/// Options:
///  -e entity                   defaults to none
///  -a collocation              defaults to none
///  -s transport configuration  defaults to none
///  -p effective transport protocol     defaults to none

///  -c expect compatibility     defaults to false
///  -t effective transport protocol     defaults to none
///  -d durability kind          defaults to TRANSIENT_LOCAL_DURABILITY_QOS
///  -k liveliness kind          defaults to AUTOMATIC_LIVELINESS_QOS
///  -r reliability kind         defaults to BEST_EFFORT_RELIABILITY_QOS
///  -l lease duration           no default
///  -x test duration in sec     defaults to 40

int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  // Swallow the executable name
  arg_shifter.consume_arg ();

  while (arg_shifter.is_anything_left ())
  {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-t"))) != 0)
    {
      protocol_str =  currentArg;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-e"))) != 0)
    {
      entity_str =  currentArg;
      ::get_entity_kind(entity_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-a"))) != 0)
	{
	  collocation_str =  currentArg;
      ::get_collocation_kind(collocation_str);
	  arg_shifter.consume_arg ();
	}
	else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-s"))) != 0)
    {
      configuration_str = currentArg;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT ("-c"))) != 0)
    {
      compatible = (ACE_TString(currentArg) == ACE_TEXT("true"));
      arg_shifter.consume_arg ();
    }
    else if (0 == arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-d")))
    {

      durability_kind_str = arg_shifter.get_the_parameter(ACE_TEXT("-d"));
      durability_kind = ::get_durability_kind(durability_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0)
    {
      liveliness_kind_str = currentArg;
      liveliness_kind = ::get_liveliness_kind(liveliness_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      LEASE_DURATION_STR = currentArg;
      LEASE_DURATION = ::get_lease_duration(LEASE_DURATION_STR);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0)
    {
      reliability_kind_str = currentArg;
      reliability_kind = ::get_reliability_kind(reliability_kind_str);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-x"))) != 0)
    {
      test_duration = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else
    {
	  ACE_ERROR ((LM_WARNING,
				  ACE_TEXT ("(%P|%t) Ignoring command line argument: %C\n"), arg_shifter.get_current()));
      arg_shifter.ignore_arg ();
    }
  }

  // Indicates successful parsing of the command line
  return 0;
}


DDS::DomainParticipant_ptr
create_configured_participant (DDS::DomainParticipantFactory_ptr dpf)
{
  DDS::DomainParticipant_var dp =
          dpf->create_participant (MY_DOMAIN,
                                   PARTICIPANT_QOS_DEFAULT,
                                   DDS::DomainParticipantListener::_nil (),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  TEST_CHECK (!configuration_str.empty ());
  if (configuration_str != "none" && entity_str == "participant")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, dp.in ());
    }


  Xyz::FooTypeSupport_var fts (new Xyz::FooTypeSupportImpl);
  TEST_CHECK (DDS::RETCODE_OK == fts->register_type (dp.in (), MY_TYPE));

  return dp._retn ();

}
DDS::Topic_ptr
create_configured_topic (DDS::DomainParticipant_ptr dp)
{
  // When collocation doesn't matter we choose a topic name that will not match
  // the publisher's topic name
  std::string topicname ((collocation_str == "none") ? MY_OTHER_TOPIC : MY_SAME_TOPIC);


  DDS::TopicQos topic_qos;
  dp->get_default_topic_qos (topic_qos);

  return dp->create_topic (topicname.c_str (),
                           MY_TYPE,
                           TOPIC_QOS_DEFAULT,
                           DDS::TopicListener::_nil (),
                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

