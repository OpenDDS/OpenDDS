#include "WorkerTopicListener.h"

namespace Bench {

WorkerTopicListener::WorkerTopicListener()
{
}

WorkerTopicListener::WorkerTopicListener(const Builder::PropertySeq&)
{
}

WorkerTopicListener::~WorkerTopicListener()
{
}

void WorkerTopicListener::on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
  std::unique_lock<std::mutex> lock(mutex_);
  ++inconsistent_count_;
}

void WorkerTopicListener::set_topic(Builder::Topic& topic) {
  topic_ = &topic;
}

}

