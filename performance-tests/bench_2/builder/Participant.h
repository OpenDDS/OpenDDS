#pragma once

#include "ListenerFactory.h"
#include "TypeSupportRegistry.h"

#include "PublisherManager.h"
#include "SubscriberManager.h"
#include "TopicManager.h"

#include "dds/DdsDcpsDomainC.h"

namespace Builder {

class Participant : public ListenerFactory<DDS::DomainParticipantListener>, public TypeSupportRegistry {
public:

  explicit Participant(const ParticipantConfig& config, ParticipantReport& report, ReaderMap& reader_map, WriterMap& writer_map);
  ~Participant();

  void enable();

  ParticipantReport& get_report() { return report_; }
  const ParticipantReport& get_report() const { return report_; }

protected:
  const std::string name_;
  const uint16_t domain_;
  const std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  const std::string transport_config_name_;
  ParticipantReport& report_;
  DDS::DomainParticipantListener_var listener_;
  DDS::DomainParticipant_var participant_;
  std::shared_ptr<TopicManager> topics_;
  std::shared_ptr<SubscriberManager> subscribers_;
  std::shared_ptr<PublisherManager> publishers_;
};

}

