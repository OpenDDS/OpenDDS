#pragma once

#include "Action.h"
#include "WorkerDataReaderListener.h"

#include "BenchTypeSupportImpl.h"

#include "dds/DCPS/GuardCondition.h"

#include "ace/Proactor.h"

namespace Bench {

class ReadAction : public Action {
public:
  explicit ReadAction(ACE_Proactor& proactor);

  bool init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
    Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map) override;

  void start() override;
  void stop() override;

  void do_read();

protected:
  std::mutex mutex_;
  ACE_Proactor& proactor_;
  bool started_, stopped_;
  DDS::GuardCondition_var stop_condition_;
  DataDataReader_var data_dr_;
  WorkerDataReaderListener* dr_listener_;
  ACE_Time_Value read_period_;
  std::shared_ptr<ACE_Handler> handler_;
};

}
