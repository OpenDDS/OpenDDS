#include "TopicManager.h"

namespace Builder {

TopicManager::TopicManager(const TopicConfigSeq& configs, DDS::DomainParticipant_var& participant, ContentFilteredTopicMap& cft_map)
{
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    std::shared_ptr<Topic> topic = std::make_shared<Topic>(configs[i], participant, cft_map);
    if (topic) {
      topics_[topic->get_name()] = topic;
    }
  }
}

std::shared_ptr<Topic> TopicManager::get_topic_by_name(const std::string& name) const
{
  std::shared_ptr<Topic> result;
  auto it = topics_.find(name);
  if (it != topics_.end()) {
    result = it->second;
  }
  return result;
}

bool TopicManager::enable(bool throw_on_error)
{
  bool result = true;
  for (auto it = topics_.begin(); it != topics_.end(); ++it) {
    result &= it->second->enable(throw_on_error);
  }
  return result;
}

void TopicManager::detach_listeners()
{
  for (auto it = topics_.begin(); it != topics_.end(); ++it) {
    it->second->detach_listener();
  }
}

}
