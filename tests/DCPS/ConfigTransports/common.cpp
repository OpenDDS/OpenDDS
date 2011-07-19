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
std::string entity_str;
bool entity_autoenable = true;
ACE_TString collocation_str;
ACE_TString source_str;
std::string configuration_str (ACE_TEXT ("none"));
std::string test_duration_str;
ACE_TString reliability_kind_str;
ACE_TString durability_kind_str;
ACE_TString liveliness_kind_str;
std::vector<std::string> protocol_str;

const ACE_TString&
get_collocation_kind (const ACE_TString& argument)
{
  if (argument == ACE_TEXT ("none"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("memory"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("host"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("network"))
    {
      return argument;
    }
  throw std::runtime_error ("invalid string for get_collocation_kind");
}

const std::string&
get_entity_kind (const std::string& argument)
{
  if (argument == ACE_TEXT ("none"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("rw"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("pubsub"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT ("participant"))
    {
      return argument;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

bool
get_entity_autoenable_kind (const std::string& argument)
{
      ACE_ERROR ((LM_WARNING,
                      ACE_TEXT ("(%P|%t) autoenable: %C\n"), argument.c_str()));
  if (argument == ACE_TEXT ("yes"))
    {
      return true;
    }
  else if (argument == ACE_TEXT ("true"))
    {
      return true;
    }
  else if (argument == ACE_TEXT ("no"))
    {
      return false;
    }
  else if (argument == ACE_TEXT ("false"))
    {
      return false;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

::DDS::LivelinessQosPolicyKind
get_liveliness_kind (const ACE_TString& argument)
{
  if (argument == ACE_TEXT ("automatic"))
    {
      return ::DDS::AUTOMATIC_LIVELINESS_QOS;
    }
  else if (argument == ACE_TEXT ("participant"))
    {
      return ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    }
  else if (argument == ACE_TEXT ("topic"))
    {
      return ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    }
  throw std::runtime_error ("invalid string for get_liveliness_kind");
}

::DDS::DurabilityQosPolicyKind
get_durability_kind (const ACE_TString& argument)
{
  if (argument == ACE_TEXT ("volatile"))
    {
      return ::DDS::VOLATILE_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT ("transient_local"))
    {
      return ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT ("transient"))
    {
      return ::DDS::TRANSIENT_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT ("persistent"))
    {
      return ::DDS::PERSISTENT_DURABILITY_QOS;
    }

  std::string message ("invalid string for get_durability_kind: ");
  throw std::runtime_error (message + argument.c_str ());
}

::DDS::ReliabilityQosPolicyKind
get_reliability_kind (const ACE_TString& argument)
{
  if (argument == ACE_TEXT ("best_effort"))
    {
      return ::DDS::BEST_EFFORT_RELIABILITY_QOS;
    }
  else if (argument == ACE_TEXT ("reliable"))
    {
      return ::DDS::RELIABLE_RELIABILITY_QOS;
    }
  throw std::runtime_error ("invalid string for get_reliability_kind");
}

::DDS::Duration_t
get_lease_duration (const ACE_TString& argument)
{
  ::DDS::Duration_t lease;
  if (argument == ACE_TEXT ("infinite"))
    {
      lease.sec = ::DDS::DURATION_INFINITE_SEC;
      lease.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    }
  else
    {
      lease.sec = ACE_OS::atoi (argument.c_str ());
      lease.nanosec = 0;
    }
  return lease;
}

/// parse the command line arguments
/// Options:
///  -e entity                   defaults to none. may be used multiple times
///  -a collocation              defaults to none
///  -s transport configuration  defaults to none
///  -p effective transport protocol     defaults to none

///  -c expect compatibility     defaults to false
///  -t effective transport protocol     defaults to none. may be used multiple times
///  -d durability kind          defaults to TRANSIENT_LOCAL_DURABILITY_QOS
///  -k liveliness kind          defaults to AUTOMATIC_LIVELINESS_QOS
///  -r reliability kind         defaults to BEST_EFFORT_RELIABILITY_QOS
///  -l lease duration           no default
///  -x test duration in sec     defaults to 40

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask (ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask (mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter (argc, argv);

  // Swallow the executable name
  arg_shifter.consume_arg ();

  while (arg_shifter.is_anything_left ())
    {
      const ACE_TCHAR *currentArg = 0;

      if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-t"))) != 0)
        {
          protocol_str.push_back (currentArg);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-n"))) != 0)
        {
          entity_autoenable = ::get_entity_autoenable_kind (currentArg);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-e"))) != 0)
        {
          entity_str = ::get_entity_kind (currentArg);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-a"))) != 0)
        {
          collocation_str = currentArg;
          ::get_collocation_kind (collocation_str);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-s"))) != 0)
        {
          configuration_str = currentArg;
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-c"))) != 0)
        {
          compatible = (ACE_TString (currentArg) == ACE_TEXT ("true"));
          arg_shifter.consume_arg ();
        }
      else if (0 == arg_shifter.cur_arg_strncasecmp (ACE_TEXT ("-d")))
        {

          durability_kind_str = arg_shifter.get_the_parameter (ACE_TEXT ("-d"));
          durability_kind = ::get_durability_kind (durability_kind_str);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-k"))) != 0)
        {
          liveliness_kind_str = currentArg;
          liveliness_kind = ::get_liveliness_kind (liveliness_kind_str);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-l"))) != 0)
        {
          LEASE_DURATION_STR = currentArg;
          LEASE_DURATION = ::get_lease_duration (LEASE_DURATION_STR);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-r"))) != 0)
        {
          reliability_kind_str = currentArg;
          reliability_kind = ::get_reliability_kind (reliability_kind_str);
          arg_shifter.consume_arg ();
        }
      else if ((currentArg = arg_shifter.get_the_parameter (ACE_TEXT ("-x"))) != 0)
        {
          test_duration = ACE_OS::atoi (currentArg);
          arg_shifter.consume_arg ();
        }
      else
        {
          ACE_ERROR ((LM_WARNING,
                      ACE_TEXT ("(%P|%t) Ignoring command line argument: %C\n"), arg_shifter.get_current ()));
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

DDS::Publisher_ptr
create_configured_publisher (DDS::DomainParticipant_ptr dp)
{

  DDS::PublisherQos pub_qos;
  dp->get_default_publisher_qos (pub_qos);
  if (entity_str == "rw")
    {
      pub_qos.entity_factory.autoenable_created_entities = entity_autoenable;
    }

  // Create the publisher
  DDS::Publisher_var pub = dp->create_publisher (pub_qos,
                                                 DDS::PublisherListener::_nil (),
                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (!CORBA::is_nil (pub.in ()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  TEST_CHECK (!configuration_str.empty ());
  if (configuration_str != "none" && entity_str == "pubsub")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, pub.in ());
    }

  return pub._retn ();
}

DDS::Subscriber_ptr
create_configured_subscriber (DDS::DomainParticipant_ptr dp)
{

  DDS::SubscriberQos sub_qos;
  dp->get_default_subscriber_qos (sub_qos);
  if (entity_str == "rw")
    {
      sub_qos.entity_factory.autoenable_created_entities = entity_autoenable;
    }

  // Create the subscriber
  DDS::Subscriber_var sub =
          dp->create_subscriber (sub_qos,
                                 DDS::SubscriberListener::_nil (),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (!CORBA::is_nil (sub.in ()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  if (configuration_str != "none" && entity_str == "pubsub")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, sub.in ());
    }

  return sub._retn ();
}

DDS::DataWriter_ptr
create_configured_writer (DDS::Publisher_ptr pub, DDS::Topic_ptr topic, DDS::DataWriterListener_ptr dwl)
{
  // Create the data writer
  DDS::DataWriterQos dw_qos;
  pub->get_default_datawriter_qos (dw_qos);

  dw_qos.durability.kind = durability_kind;
  dw_qos.liveliness.kind = liveliness_kind;
  dw_qos.liveliness.lease_duration = LEASE_DURATION;
  dw_qos.reliability.kind = reliability_kind;

  DDS::DataWriter_var dw = pub->create_datawriter (topic,
                                                   dw_qos,
                                                   dwl,
                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Initialize the transport configuration for the appropriate entity
  TEST_CHECK (!configuration_str.empty ());

  if (configuration_str != "none" && entity_str == "rw")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str,
                                                                  dw.in ());

      if (!entity_autoenable)
        {
          TEST_CHECK (DDS::RETCODE_OK == dw->enable ());
        }

    }

  return dw._retn ();
}

DDS::DataReader_ptr
create_configured_reader (DDS::Subscriber_ptr sub, DDS::Topic_ptr topic, DDS::DataReaderListener_ptr drl)
{
  // Create the data readers
  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos (dr_qos);

  dr_qos.durability.kind = durability_kind;
  dr_qos.liveliness.kind = liveliness_kind;
  dr_qos.liveliness.lease_duration = LEASE_DURATION;
  dr_qos.reliability.kind = reliability_kind;

  DDS::TopicDescription_var description =
          sub->get_participant ()->lookup_topicdescription (topic->get_name ());
  TEST_CHECK (!CORBA::is_nil (description.in ()));

  DDS::DataReader_var rd (sub->create_datareader (description.in (),
                                                  dr_qos,
                                                  drl,
                                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK));

  // Initialize the transport configuration for the appropriate entity
  TEST_CHECK (!configuration_str.empty ());
  if (configuration_str != "none" && entity_str == "rw")
    {

      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str,
                                                                  rd.in ());
      if (!entity_autoenable)
        {
          TEST_CHECK (DDS::RETCODE_OK == rd->enable ());
        }
    }

  return rd._retn ();
}

bool
assert_subscription_matched (DDS::DataReaderListener_ptr drl)
{
  // Assert if pub/sub made a match ...

  DataReaderListenerImpl* drl_servant =
          dynamic_cast<DataReaderListenerImpl*> (drl);

  // there is an error if we matched when not compatible (or vice-versa)
  return !compatible || drl_servant->subscription_matched ();
}

bool
assert_publication_matched (DDS::DataWriterListener_ptr dwl)
{
  // Assert if pub/sub made a match ...
  DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*> (dwl);

  // check to see if the publisher worked
  return !compatible || dwl_servant->publication_matched ();

}
