#pragma once

#include "Subscriber.h"

#include <vector>

namespace Builder {

class SubscriberManager {
public:
  explicit SubscriberManager(const SubscriberConfigSeq& configs, SubscriberReportSeq& reports, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map);
  void enable();

protected:
  std::vector<std::shared_ptr<Subscriber>> subscribers_;
};

}

