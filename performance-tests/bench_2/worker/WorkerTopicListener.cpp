#include "WorkerTopicListener.h"

namespace Bench {

void WorkerTopicListener::on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
  std::unique_lock<std::mutex> lock(mutex_);
  ++inconsistent_count_;
}

void WorkerTopicListener::set_topic(Builder::Topic& topic) {
  topic_ = &topic;
}

}

