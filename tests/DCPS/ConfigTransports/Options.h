#ifndef OPTIONS_H
#define OPTIONS_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsTypeSupportExtC.h"

#include <vector>
#include "dds/DCPS/PoolAllocator.h"

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
  OPENDDS_STRING configuration_str;
  std::vector<OPENDDS_STRING> protocol_str;
  std::vector<OPENDDS_STRING> negotiated_str;

};
#endif
