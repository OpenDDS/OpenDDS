#include "Options.h"
#include "ace/Arg_Shifter.h"
#include <stdexcept>

Options::Options() :
        test_duration(20),
        test_duration_str("20"),
        LEASE_DURATION_STR("infinite"),
        reliability_kind(::DDS::BEST_EFFORT_RELIABILITY_QOS),
        reliability_kind_str("best_effort"),
        durability_kind(::DDS::TRANSIENT_LOCAL_DURABILITY_QOS),
        durability_kind_str("transient_local"),
        liveliness_kind(::DDS::AUTOMATIC_LIVELINESS_QOS),
        liveliness_kind_str("automatic"),
        compatible(false),
        entity_autoenable(true),
        entity_str("none"),
        collocation_str("none"),
        configuration_str("none")
{
  LEASE_DURATION.sec = ::DDS::DURATION_INFINITE_SEC;
  LEASE_DURATION.nanosec = ::DDS::DURATION_INFINITE_NSEC;
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
///  -x test duration in sec     defaults to 20

Options::Options(int argc, ACE_TCHAR *argv[]) :
        test_duration(20),
        test_duration_str("20"),
        reliability_kind(::DDS::BEST_EFFORT_RELIABILITY_QOS),
        reliability_kind_str("best_effort"),
        durability_kind(::DDS::TRANSIENT_LOCAL_DURABILITY_QOS),
        durability_kind_str("transient_local"),
        liveliness_kind(::DDS::AUTOMATIC_LIVELINESS_QOS),
        liveliness_kind_str("automatic"),
        compatible(false),
        entity_autoenable(true),
        entity_str("none"),
        collocation_str("none"),
        configuration_str("none")
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  // Swallow the executable name
  arg_shifter.consume_arg();

  LEASE_DURATION.sec = ::DDS::DURATION_INFINITE_SEC;
  LEASE_DURATION.nanosec = ::DDS::DURATION_INFINITE_NSEC;

  while (arg_shifter.is_anything_left())
    {
      const ACE_TCHAR *currentArg = 0;

      if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
        {
          this->protocol_str.push_back(ACE_TEXT_ALWAYS_CHAR(currentArg));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-f"))) != 0)
        {
          this->negotiated_str.push_back(ACE_TEXT_ALWAYS_CHAR(currentArg));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
        {
          this->entity_autoenable = this->get_entity_autoenable_kind(ACE_TEXT_ALWAYS_CHAR(currentArg));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-e"))) != 0)
        {
          this->entity_str = this->get_entity_kind(ACE_TEXT_ALWAYS_CHAR(currentArg));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0)
        {
          this->collocation_str = this->get_collocation_kind(ACE_TEXT_ALWAYS_CHAR(currentArg));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0)
        {
          this->configuration_str = ACE_TEXT_ALWAYS_CHAR(currentArg);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-c"))) != 0)
        {
          this->compatible = (ACE_TString(currentArg) == ACE_TEXT("true"));
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
        {
          this->durability_kind_str = ACE_TEXT_ALWAYS_CHAR(currentArg);
          this->durability_kind = this->get_durability_kind(this->durability_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0)
        {
          this->liveliness_kind_str = ACE_TEXT_ALWAYS_CHAR(currentArg);
          this->liveliness_kind = this->get_liveliness_kind(this->liveliness_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
        {
          this->LEASE_DURATION_STR = ACE_TEXT_ALWAYS_CHAR(currentArg);
          this->LEASE_DURATION = this->get_lease_duration(this->LEASE_DURATION_STR);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0)
        {
          this->reliability_kind_str = ACE_TEXT_ALWAYS_CHAR(currentArg);
          this->reliability_kind = this->get_reliability_kind(this->reliability_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-x"))) != 0)
        {
          this->test_duration = ACE_OS::atoi(currentArg);
          arg_shifter.consume_arg();
        }
      else
        {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) Ignoring command line argument: %C\n"), arg_shifter.get_current()));
          arg_shifter.ignore_arg();
        }
    }
}

Options::~Options() { }

const std::string&
Options::get_collocation_kind(const std::string& argument)
{
  if (argument == "none")
    {
      return argument;
    }
  else if (argument == "process")
    {
      return argument;
    }
  else if (argument == "participant")
    {
      return argument;
    }
  else if (argument == "pubsub")
    {
      return argument;
    }
  throw std::runtime_error("invalid string for get_collocation_kind");
}

const std::string&
Options::get_entity_kind(const std::string& argument)
{
  if (argument == "none")
    {
      return argument;
    }
  else if (argument == "rw")
    {
      return argument;
    }
  else if (argument == "pubsub")
    {
      return argument;
    }
  else if (argument == "participant")
    {
      return argument;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

bool
Options::get_entity_autoenable_kind(const std::string& argument)
{
  if (argument == "yes")
    {
      return true;
    }
  else if (argument == "true")
    {
      return true;
    }
  else if (argument == "no")
    {
      return false;
    }
  else if (argument == "false")
    {
      return false;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

::DDS::LivelinessQosPolicyKind
Options::get_liveliness_kind(const std::string& argument)
{
  if (argument == "automatic")
    {
      return ::DDS::AUTOMATIC_LIVELINESS_QOS;
    }
  else if (argument == "participant")
    {
      return ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    }
  else if (argument == "topic")
    {
      return ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    }
  throw std::runtime_error("invalid string for get_liveliness_kind");
}

::DDS::DurabilityQosPolicyKind
Options::get_durability_kind(const std::string& argument)
{
  if (argument == "volatile")
    {
      return ::DDS::VOLATILE_DURABILITY_QOS;
    }
  else if (argument == "transient_local")
    {
      return ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    }
  else if (argument == "transient")
    {
      return ::DDS::TRANSIENT_DURABILITY_QOS;
    }
  else if (argument == "persistent")
    {
      return ::DDS::PERSISTENT_DURABILITY_QOS;
    }

  std::string message("invalid string for get_durability_kind: ");
  throw std::runtime_error(message + argument.c_str());
}

::DDS::ReliabilityQosPolicyKind
Options::get_reliability_kind(const std::string& argument)
{
  if (argument == "best_effort")
    {
      return ::DDS::BEST_EFFORT_RELIABILITY_QOS;
    }
  else if (argument == "reliable")
    {
      return ::DDS::RELIABLE_RELIABILITY_QOS;
    }
  throw std::runtime_error("invalid string for get_reliability_kind");
}

::DDS::Duration_t
Options::get_lease_duration(const std::string& argument)
{
  ::DDS::Duration_t lease;
  if (argument == "infinite")
    {
      lease.sec = ::DDS::DURATION_INFINITE_SEC;
      lease.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    }
  else
    {
      lease.sec = ACE_OS::atoi(argument.c_str());
      lease.nanosec = 0;
    }
  return lease;
}
