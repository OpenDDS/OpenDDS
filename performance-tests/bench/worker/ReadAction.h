#pragma once

#include "Action.h"
#include "WorkerDataReaderListener.h"

#include "BenchTypeSupportImpl.h"

#include "dds/DCPS/GuardCondition.h"

#include "dds/DCPS/EventDispatcher.h"

namespace Bench {

class ReadAction : public virtual Action, public std::enable_shared_from_this<ReadAction> {
public:
  explicit ReadAction(OpenDDS::DCPS::EventDispatcher_rch event_dispatcher);

  bool init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
    Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map) override;

  void test_start() override;
  void test_stop() override;
  void action_stop() override;

  void do_read();

protected:
  std::mutex mutex_;
  OpenDDS::DCPS::EventDispatcher_rch event_dispatcher_;
  bool started_, stopped_;
  DDS::GuardCondition_var stop_condition_;
  DDS::ReadCondition_var read_condition_;
  DataDataReader_var data_dr_;
  DDS::WaitSet_var ws_;
  WorkerDataReaderListener* dr_listener_;
  ACE_Time_Value read_period_;
  OpenDDS::DCPS::EventBase_rch event_;
};

}
