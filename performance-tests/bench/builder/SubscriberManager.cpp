#include "SubscriberManager.h"

namespace Builder {

SubscriberManager::SubscriberManager(const SubscriberConfigSeq& configs, SubscriberReportSeq& reports, DDS::DomainParticipant_var& participant,
  const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map, const ContentFilteredTopicMap& cft_map)
{
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    subscribers_.emplace_back(std::make_shared<Subscriber>(configs[i], reports[i], participant, topics, reader_map, cft_map));
  }
}

bool SubscriberManager::enable(bool throw_on_error)
{
  bool result = true;
  for (auto it = subscribers_.begin(); it != subscribers_.end(); ++it) {
    result &= (*it)->enable(throw_on_error);
  }
  return result;
}

void SubscriberManager::detach_listeners()
{
  for (auto it = subscribers_.begin(); it != subscribers_.end(); ++it) {
    (*it)->detach_listeners();
  }
}

}
