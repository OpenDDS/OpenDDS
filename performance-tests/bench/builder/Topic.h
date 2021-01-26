#pragma once

#include "dds/DdsDcpsTopicC.h"

#include "Common.h"
#include "ListenerFactory.h"

namespace Builder {

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  using ContentFilteredTopicMap = std::map<std::string, DDS::ContentFilteredTopic_var>;
#else
  class ContentFilteredTopic_var_stub
  {
  public:

    inline bool operator < (const ContentFilteredTopic_var_stub&) const {
      return true;
    }
  };

  using ContentFilteredTopicMap = std::map<std::string, ContentFilteredTopic_var_stub>;
#endif

class Topic : public ListenerFactory<DDS::TopicListener> {
public:
  explicit Topic(const TopicConfig& config, DDS::DomainParticipant_var& participant,
    ContentFilteredTopicMap& content_filtered_topics_map);
  ~Topic();

  const std::string& get_name() const;
  DDS::Topic_var& get_dds_topic();

  bool enable(bool throw_on_error = false);
  void detach_listener();

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
