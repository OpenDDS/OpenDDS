#pragma once

#include "Topic.h"
#include "TopicListener.h"

#include <atomic>

namespace Bench {

class WorkerTopicListener : public Builder::TopicListener {
public:
  WorkerTopicListener() = default;
  WorkerTopicListener(const Builder::PropertySeq& properties);
  virtual ~WorkerTopicListener() = default;

  void on_inconsistent_topic(DDS::Topic_ptr the_topic, const DDS::InconsistentTopicStatus& status) override;

  void set_topic(Builder::Topic& topic) override;
  void unset_topic(Builder::Topic& topic) override;

protected:
  Builder::Topic* topic_{};
  std::atomic<size_t> inconsistent_count_{0};
};

}
