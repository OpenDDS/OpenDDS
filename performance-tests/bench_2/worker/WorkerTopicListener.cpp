#include "WorkerTopicListener.h"

namespace Bench {

WorkerTopicListener::WorkerTopicListener(const Builder::PropertySeq&)
{
}

void WorkerTopicListener::on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
  ++inconsistent_count_;
}

void WorkerTopicListener::set_topic(Builder::Topic& topic) {
  topic_ = &topic;
}

void WorkerTopicListener::unset_topic(Builder::Topic& topic) {
  if (topic_ == &topic) {
    topic_ = nullptr;
  }
}

}
