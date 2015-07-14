// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
 *
 *
 */
// ============================================================================


#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/SString.h"
#include <stdexcept>
#include <string>

DDS::Duration_t LEASE_DURATION;

int test_duration = 40;
::DDS::ReliabilityQosPolicyKind reliability_kind = ::DDS::RELIABLE_RELIABILITY_QOS;
::DDS::DurabilityQosPolicyKind durability_kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
::DDS::LivelinessQosPolicyKind liveliness_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
bool compatible = false;

ACE_TString LEASE_DURATION_STR;
std::string test_duration_str;
ACE_TString reliability_kind_str;
ACE_TString durability_kind_str;
ACE_TString liveliness_kind_str;

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
  throw std::runtime_error("invalid string for get_durability_kind");
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

DDS::Duration_t get_lease_duration(const ACE_TString& argument)
{
  DDS::Duration_t lease;
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
