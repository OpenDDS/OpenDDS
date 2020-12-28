#pragma once

#include "Topic.h"

#include <map>
#include <memory>

namespace Builder {

class TopicManager {
public:
  explicit TopicManager(const TopicConfigSeq& configs, DDS::DomainParticipant_var& participant, ContentFilteredTopicMap& cft_map);

  std::shared_ptr<Topic> get_topic_by_name(const std::string& name) const;

  bool enable(bool throw_on_error = false);
  void detach_listeners();

protected:
  std::map<std::string, std::shared_ptr<Topic>> topics_;
};

}
