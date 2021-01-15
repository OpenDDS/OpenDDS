#pragma once

#include "DataReaderManager.h"

namespace Builder {

class Subscriber : public ListenerFactory<DDS::SubscriberListener> {
public:
  explicit Subscriber(const SubscriberConfig& config, SubscriberReport& report, DDS::DomainParticipant_var& participant,
    const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map, const ContentFilteredTopicMap& cft_map);
  ~Subscriber();

  bool enable(bool throw_on_error = false);
  void detach_listeners();

  SubscriberReport& get_report() { return report_; }
  const SubscriberReport& get_report() const { return report_; }

protected:
  std::string name_;
  std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  Builder::PropertySeq listener_properties_;
  const std::string transport_config_name_;
  SubscriberReport& report_;
  DDS::DomainParticipant_var participant_;
  DDS::SubscriberListener_var listener_;
  DDS::Subscriber_var subscriber_;
  std::shared_ptr<DataReaderManager> datareaders_;
};

}
