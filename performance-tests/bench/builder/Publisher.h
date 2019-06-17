#pragma once

#include "DataWriterManager.h"

namespace Builder {

class Publisher : public ListenerFactory<DDS::PublisherListener>  {
public:
  explicit Publisher(const PublisherConfig& config, PublisherReport& report, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map);
  ~Publisher();

  void enable();

  PublisherReport& get_report() { return report_; }
  const PublisherReport& get_report() const { return report_; }

protected:
  std::string name_;
  std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  const std::string transport_config_name_;
  PublisherReport& report_;
  DDS::DomainParticipant_var participant_;
  DDS::PublisherListener_var listener_;
  DDS::Publisher_var publisher_;
  std::shared_ptr<DataWriterManager> datawriters_;
};

}

