#pragma once

#include "BenchC.h"
#include "BuilderProcess.h"

namespace Bench {

class Action {
public:
  virtual ~Action() {}
  virtual bool init(const Bench::ActionConfig& config, Bench::ActionReport& report,
    Builder::ReaderMap& readers, Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map);

  virtual void action_start();
  virtual void test_start() = 0;
  virtual void test_stop() = 0;
  virtual void action_stop();

protected:
  const Bench::ActionConfig* config_{0};
  Bench::ActionReport* report_{0};
  Builder::ReaderMap readers_by_name_;
  Builder::WriterMap writers_by_name_;
  std::vector<std::shared_ptr<Builder::DataReader> > readers_by_index_;
  std::vector<std::shared_ptr<Builder::DataWriter> > writers_by_index_;
};

}
