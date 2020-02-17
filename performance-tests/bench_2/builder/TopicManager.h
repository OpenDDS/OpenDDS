#pragma once

#include "Topic.h"

#include <map>
#include <memory>

namespace Builder {

class TopicManager {
public:
  explicit TopicManager(const TopicConfigSeq& configs, DDS::DomainParticipant_var& participant);
  std::shared_ptr<Topic> get_topic_by_name(const std::string& name) const;

protected:
  std::map<std::string, std::shared_ptr<Topic>> topics_;
};

}

