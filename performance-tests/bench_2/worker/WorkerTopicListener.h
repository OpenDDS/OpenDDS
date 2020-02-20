#pragma once

#include "Topic.h"
#include "TopicListener.h"

#include <mutex>

namespace Bench {

class WorkerTopicListener : public Builder::TopicListener {
public:
  void on_inconsistent_topic(DDS::Topic_ptr the_topic, const DDS::InconsistentTopicStatus& status) override;
  void set_topic(Builder::Topic& topic) override;

protected:
  std::mutex mutex_;
  Builder::Topic* topic_{0};
  size_t inconsistent_count_{0};
};

}

