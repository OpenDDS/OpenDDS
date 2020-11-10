#pragma once

#include "dds/DdsDcpsTopicC.h"

#include "Topic.h"

#include <map>
#include <memory>

namespace Builder {

class TopicManager {
public:
  explicit TopicManager(const TopicConfigSeq& configs, DDS::DomainParticipant_var& participant);

  std::shared_ptr<Topic> get_topic_by_name(const std::string& name) const;
  DDS::ContentFilteredTopic_var get_content_filtered_topic_by_name(const std::string& name) const;

  bool enable(bool throw_on_error = false);

protected:
  std::map<std::string, std::shared_ptr<Topic>> topics_;
  std::map<std::string, DDS::ContentFilteredTopic_var> content_filtered_topics_;
};

}

