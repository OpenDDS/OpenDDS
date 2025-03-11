#pragma once

#include "Action.h"

#include "BenchTypeSupportImpl.h"

#include <dds/DdsDcpsTopicC.h>
#include <dds/DCPS/EventDispatcher.h>

#include <random>
#include <vector>

namespace Bench {

class SetCftParametersAction : public virtual Action, public std::enable_shared_from_this<SetCftParametersAction> {
public:
  explicit SetCftParametersAction(OpenDDS::DCPS::EventDispatcher_rch event_dispatcher);

  bool init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
    Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map) override;

  void test_start() override;
  void test_stop() override;

  void do_set_expression_parameters();

protected:
  std::mutex mutex_;
  OpenDDS::DCPS::EventDispatcher_rch event_dispatcher_;
  bool started_, stopped_;
  OpenDDS::DCPS::TimeDuration set_period_;
  size_t max_count_;
  size_t param_count_;
  bool random_order_;
  DDS::InstanceHandle_t instance_;
  OpenDDS::DCPS::MonotonicTimePoint last_scheduled_time_;
  OpenDDS::DCPS::EventBase_rch event_;
  std::mt19937_64 mt_;
  size_t set_call_count_;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  DDS::ContentFilteredTopic_var content_filtered_topic_;
#endif
  Builder::StringSeqSeq acceptable_param_values_;
  std::vector<int> current_acceptable_param_values_index_;
};

}
