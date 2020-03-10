#include "SubscriberManager.h"

namespace Builder {

SubscriberManager::SubscriberManager(const SubscriberConfigSeq& configs, SubscriberReportSeq& reports, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    subscribers_.emplace_back(std::make_shared<Subscriber>(configs[i], reports[i], participant, topics, reader_map));
  }
}

void SubscriberManager::enable() {
  for (auto it = subscribers_.begin(); it != subscribers_.end(); ++it) {
    (*it)->enable();
  }
}

}

