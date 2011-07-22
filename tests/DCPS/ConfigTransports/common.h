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




                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include <vector>

#define MY_DOMAIN 411
#define MY_SAME_TOPIC  "foo"
#define MY_OTHER_TOPIC  "bar"
#define MY_TYPE "foo"

class Options
{
public:
  Options();
  Options(int argc, ACE_TCHAR *argv[]);
  virtual ~Options();


protected:
  ::DDS::DurabilityQosPolicyKind get_durability_kind(const std::string& argument);
  ::DDS::LivelinessQosPolicyKind get_liveliness_kind(const std::string& argument);
  ::DDS::ReliabilityQosPolicyKind get_reliability_kind(const std::string& argument);
  ::DDS::Duration_t get_lease_duration(const std::string& argument);
  const std::string& get_collocation_kind(const std::string& argument);
  const std::string& get_entity_kind(const std::string& argument);
  bool get_entity_autoenable_kind(const std::string& argument);

public:
  int test_duration;
  std::string test_duration_str;

  DDS::Duration_t LEASE_DURATION;
  std::string LEASE_DURATION_STR;

  DDS::ReliabilityQosPolicyKind reliability_kind;
  std::string reliability_kind_str;

  DDS::DurabilityQosPolicyKind durability_kind;
  std::string durability_kind_str;

  DDS::LivelinessQosPolicyKind liveliness_kind;
  std::string liveliness_kind_str;

  bool compatible;
  bool entity_autoenable;

  std::string entity_str;
  std::string source_str;
  std::string collocation_str;
  std::string configuration_str;
  std::vector<std::string> protocol_str;

};

class Factory
{
public:
  Factory(const Options& opts);
  virtual ~Factory();

  DDS::DomainParticipant_ptr participant(DDS::DomainParticipantFactory_ptr dpf) const;

  DDS::Topic_ptr topic(DDS::DomainParticipant_ptr dp) const;

  DDS::Subscriber_ptr subscriber(DDS::DomainParticipant_ptr dp) const;

  DDS::DataReader_ptr reader(DDS::Subscriber_ptr sub, DDS::Topic_ptr topic, DDS::DataReaderListener_ptr drl) const;

  DDS::Publisher_ptr publisher(DDS::DomainParticipant_ptr dp) const;

  DDS::DataWriter_ptr writer(DDS::Publisher_ptr pub, DDS::Topic_ptr topic, DDS::DataWriterListener_ptr dwl) const;

private:
  const Options& opts_;
};

namespace OpenDDS
{
  namespace DCPS
  {
    class TransportClient;
  }
}

//=== these may be changed by options so they are in common.cpp
//=== so changes will not be local to the file that included them.


DDS::DomainParticipant_ptr
create_plain_participant(DDS::DomainParticipantFactory_ptr dpf);


bool wait_publication_matched_status(const Options& opts, const DDS::Entity_ptr e);

bool assert_publication_matched(const Options& opts, DDS::DataWriterListener_ptr dwl);

bool assert_subscription_matched(const Options& opts, DDS::DataReaderListener_ptr drl);

bool assert_supports_all(const Options& opts, const DDS::Entity_ptr e);

bool assert_supports_all(const Options& opts, const OpenDDS::DCPS::TransportClient* tc, const std::vector<std::string>& transporti);
