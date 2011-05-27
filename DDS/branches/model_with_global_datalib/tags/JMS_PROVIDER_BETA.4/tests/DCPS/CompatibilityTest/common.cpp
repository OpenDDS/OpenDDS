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


#include "dds/DdsDcpsInfrastructureC.h"
#include <stdexcept>
#include <string>

DDS::Duration_t LEASE_DURATION;

int test_duration = 40;
::DDS::ReliabilityQosPolicyKind reliability_kind = ::DDS::RELIABLE_RELIABILITY_QOS;
::DDS::DurabilityQosPolicyKind durability_kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
::DDS::LivelinessQosPolicyKind liveliness_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
bool compatible = false;

std::string LEASE_DURATION_STR;
std::string test_duration_str;
std::string reliability_kind_str;
std::string durability_kind_str;
std::string liveliness_kind_str;

::DDS::LivelinessQosPolicyKind get_liveliness_kind(const std::string& argument)
{
  if(argument == "automatic")
  {
    return ::DDS::AUTOMATIC_LIVELINESS_QOS;
  }
  else if(argument == "participant")
  {
    return ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  }
  else if(argument == "topic")
  {
    return ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  }
  throw std::runtime_error("invalid string for get_liveliness_kind");
}

::DDS::DurabilityQosPolicyKind get_durability_kind(const std::string& argument)
{
  if(argument == "volatile")
  {
    return ::DDS::VOLATILE_DURABILITY_QOS;
  }
  else if(argument == "transient_local")
  {
    return ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  }
  else if(argument == "transient")
  {
    return ::DDS::TRANSIENT_DURABILITY_QOS;
  }
  else if(argument == "persistent")
  {
    return ::DDS::PERSISTENT_DURABILITY_QOS;
  }
  throw std::runtime_error("invalid string for get_durability_kind");
}

::DDS::ReliabilityQosPolicyKind get_reliability_kind(const std::string& argument)
{
  if(argument == "best_effort")
  {
    return ::DDS::BEST_EFFORT_RELIABILITY_QOS;
  }
  else if(argument == "reliable")
  {
    return ::DDS::RELIABLE_RELIABILITY_QOS;
  }
  throw std::runtime_error("invalid string for get_reliability_kind");
}

DDS::Duration_t get_lease_duration(const std::string& argument)
{
  DDS::Duration_t lease;
  if(argument == "infinite")
  {
    lease.sec = ::DDS::DURATION_INFINITY_SEC;
    lease.nanosec = ::DDS::DURATION_INFINITY_NSEC;
  }
  else
  {
    lease.sec = ACE_OS::atoi (argument.c_str());
    lease.nanosec = 0;
  }
  return lease;
}
