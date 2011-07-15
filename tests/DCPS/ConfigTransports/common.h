// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "dds/DCPS/transport/tcp/TcpInst.h"

#include "ace/SString.h"

#define MY_DOMAIN 411
#define MY_SAME_TOPIC  "foo"
#define MY_OTHER_TOPIC  "bar"
#define MY_TYPE "foo"


// These files need to be unlinked in the run test script before and
// after running.
static ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
static ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
static ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
static ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");

enum TransportTypeId
{
  SIMPLE_TCP
};


enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};

//=== these may be changed by options so they are in common.cpp
//=== so changes will not be local to the file that included them.

extern DDS::Duration_t LEASE_DURATION;

extern int test_duration;
extern ::DDS::ReliabilityQosPolicyKind reliability_kind;
extern ::DDS::DurabilityQosPolicyKind durability_kind;
extern ::DDS::LivelinessQosPolicyKind liveliness_kind;
extern bool compatible;

extern ACE_TString LEASE_DURATION_STR;
extern ACE_TString entity_str;
extern ACE_TString source_str;
extern ACE_TString collocation_str;
extern std::string configuration_str;
extern std::string test_duration_str;
extern ACE_TString reliability_kind_str;
extern ACE_TString durability_kind_str;
extern ACE_TString liveliness_kind_str;
extern std::string protocol_str;

const ACE_TString& get_source_kind(const ACE_TString& argument);
const ACE_TString& get_collocation_kind(const ACE_TString& argument);
const ACE_TString& get_entity_kind(const ACE_TString& argument);

::DDS::DurabilityQosPolicyKind get_durability_kind(const ACE_TString& argument);
::DDS::LivelinessQosPolicyKind get_liveliness_kind(const ACE_TString& argument);
::DDS::ReliabilityQosPolicyKind get_reliability_kind(const ACE_TString& argument);
::DDS::Duration_t get_lease_duration(const ACE_TString& argument);

int parse_args (int argc, ACE_TCHAR *argv[]);
