#pragma once

#include "dds/DdsDcpsTopicC.h"

#include "Action.h"
#include "ace/Proactor.h"
#include "BenchTypeSupportImpl.h"

#include <random>

namespace Bench {

class SetCftParametersAction : public Action {
public:
  SetCftParametersAction(ACE_Proactor& proactor);

  bool init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
    Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map) override;

  void start() override;
  void stop() override;

  void do_set_expression_parameters();

protected:
  std::mutex mutex_;
  ACE_Proactor& proactor_;
  bool started_, stopped_;
  ACE_Time_Value set_period_;
  size_t max_count_;
  size_t param_count_;
  DDS::InstanceHandle_t instance_;
  std::shared_ptr<ACE_Handler> handler_;
  std::mt19937_64 mt_;
  size_t set_call_count_;
};

}

