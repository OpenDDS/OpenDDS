#pragma once

#include "dds/DdsDcpsTopicC.h"

#include "Common.h"
#include "ListenerFactory.h"

#include <vector>

namespace Builder {

class Topic : public ListenerFactory<DDS::TopicListener> {
public:
  explicit Topic(const TopicConfig& config, DDS::DomainParticipant_var& participant,
    std::map<std::string, DDS::ContentFilteredTopic_var>& content_filtered_topics_map);
  ~Topic();

  const std::string& get_name() const;
  DDS::Topic_var& get_dds_topic();

protected:
  const std::string name_;
  const std::string type_name_;
  const std::string listener_type_name_;
  uint32_t listener_status_mask_;
  Builder::PropertySeq listener_properties_;
  const std::string transport_config_name_;
  Builder::ContentFilteredTopicSeq content_filtered_topics_;
  DDS::DomainParticipant_var participant_;
  DDS::TopicListener_var listener_;
  DDS::Topic_var topic_;
};

}

