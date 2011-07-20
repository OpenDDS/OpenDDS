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

#include "DataReaderListener.h"
#include "DataWriterListenerImpl.h"


#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include "dds/DCPS/transport/tcp/TcpInst.h"

#include "ace/SString.h"

#define MY_DOMAIN 411
#define MY_SAME_TOPIC  "foo"
#define MY_OTHER_TOPIC  "bar"
#define MY_TYPE "foo"



//=== these may be changed by options so they are in common.cpp
//=== so changes will not be local to the file that included them.

extern DDS::Duration_t LEASE_DURATION;

extern int test_duration;
extern ::DDS::ReliabilityQosPolicyKind reliability_kind;
extern ::DDS::DurabilityQosPolicyKind durability_kind;
extern ::DDS::LivelinessQosPolicyKind liveliness_kind;
extern bool compatible;

extern ACE_TString LEASE_DURATION_STR;
extern std::string entity_str;
extern bool entity_autoenable;
extern ACE_TString source_str;
extern std::string collocation_str;
extern std::string configuration_str;
extern std::string test_duration_str;
extern ACE_TString reliability_kind_str;
extern ACE_TString durability_kind_str;
extern ACE_TString liveliness_kind_str;
extern std::vector<std::string> protocol_str;

const ACE_TString& get_source_kind(const ACE_TString& argument);
const std::string& get_collocation_kind(const std::string& argument);
const std::string& get_entity_kind(const std::string& argument);
bool get_entity_autoenable_kind(const std::string& argument);

::DDS::DurabilityQosPolicyKind get_durability_kind(const ACE_TString& argument);
::DDS::LivelinessQosPolicyKind get_liveliness_kind(const ACE_TString& argument);
::DDS::ReliabilityQosPolicyKind get_reliability_kind(const ACE_TString& argument);
::DDS::Duration_t get_lease_duration(const ACE_TString& argument);

int parse_args(int argc, ACE_TCHAR *argv[]);

DDS::DomainParticipant_ptr
create_configured_participant(DDS::DomainParticipantFactory_ptr dpf);

DDS::Topic_ptr
create_configured_topic(DDS::DomainParticipant_ptr dp);

DDS::Subscriber_ptr
create_configured_subscriber(DDS::DomainParticipant_ptr dp);

DDS::DataReader_ptr
create_configured_reader(DDS::Subscriber_ptr sub, DDS::Topic_ptr topic, DDS::DataReaderListener_ptr drl);

DDS::Publisher_ptr
create_configured_publisher(DDS::DomainParticipant_ptr dp);

DDS::DataWriter_ptr
create_configured_writer(DDS::Publisher_ptr pub, DDS::Topic_ptr topic, DDS::DataWriterListener_ptr dwl);

bool
assert_publication_matched(DDS::DataWriterListener_ptr dwl);

bool
assert_subscription_matched(DDS::DataReaderListener_ptr drl);

bool
assert_supports_all(const DDS::DataReader_ptr tc, const std::vector<std::string>& transporti);

bool
assert_supports_all(const DDS::DataWriter_ptr tc, const std::vector<std::string>& transporti);

bool
assert_supports_all(const OpenDDS::DCPS::TransportClient* tc, const std::vector<std::string>& transporti);