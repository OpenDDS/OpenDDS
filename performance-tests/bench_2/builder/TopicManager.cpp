#include "TopicManager.h"

namespace Builder {

TopicManager::TopicManager(const TopicConfigSeq& configs, DDS::DomainParticipant_var& participant) {
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    std::shared_ptr<Topic> topic = std::make_shared<Topic>(configs[i], participant, content_filtered_topics_);
    if (topic) {
      topics_[topic->get_name()] = topic;
    }
  }
}

std::shared_ptr<Topic> TopicManager::get_topic_by_name(const std::string& name) const {
  std::shared_ptr<Topic> result;
  auto it = topics_.find(name);
  if (it != topics_.end()) {
    result = it->second;
  }
  return result;
}

DDS::ContentFilteredTopic_var TopicManager::get_content_filtered_topic_by_name(const std::string& name) const {
  DDS::ContentFilteredTopic_var result;
  auto it = content_filtered_topics_.find(name);
  if (it != content_filtered_topics_.end()) {
    result = it->second;
  }
  return result;
}

}

