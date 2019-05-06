#pragma once

#include "StoolC.h"
#include "Process.h"

namespace Stool {

class Action {
public:
  virtual ~Action() {}
  virtual bool init(const Stool::ActionConfig& config, Stool::ActionReport& report, Builder::ReaderMap& readers, Builder::WriterMap& writers);

  virtual void start() = 0;
  virtual void stop() = 0;

protected:
  const Stool::ActionConfig* config_{0};
  Stool::ActionReport* report_{0};
  Builder::ReaderMap readers_by_name_;
  Builder::WriterMap writers_by_name_;
  std::vector<std::shared_ptr<Builder::DataReader> > readers_by_index_;
  std::vector<std::shared_ptr<Builder::DataWriter> > writers_by_index_;
};

}

