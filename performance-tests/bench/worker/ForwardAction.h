#pragma once

#include "ace/Proactor.h"

#include "Action.h"
#include "DataHandler.h"
#include "WorkerDataReaderListener.h"

#include "BenchTypeSupportImpl.h"

#include <condition_variable>

namespace Bench {

class ForwardAction : public Action, public DataHandler {
public:
  ForwardAction(ACE_Proactor& proactor);

  bool init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers, Builder::WriterMap& writers) override;

  void start() override;
  void stop() override;

  void on_data(const Data& data) override;
  void do_writes();

protected:
  class Registration {
  public:
    Registration(ForwardAction& fa, WorkerDataReaderListener* wdrl) : fa_(fa), wdrl_(wdrl) { wdrl_->add_handler(fa_); }
  protected:
    ForwardAction& fa_;
    WorkerDataReaderListener* wdrl_;
  };

  std::mutex mutex_;
  ACE_Proactor& proactor_;
  bool started_, stopped_;
  std::vector<std::shared_ptr<Registration> > registrations_;
  std::vector<DataDataWriter_var> data_dws_;
  bool prevent_copy_, force_copy_;
  size_t copy_threshold_;
  std::vector<Data> data_queue_;
  size_t queue_first_, queue_last_;
  std::condition_variable queue_not_full_;
  DDS::InstanceHandle_t instance_;
  std::shared_ptr<ACE_Handler> handler_;
};

}

