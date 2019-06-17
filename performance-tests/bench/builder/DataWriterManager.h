#pragma once

#include "DataWriter.h"

#include <vector>

namespace Builder {

class DataWriterManager {
public:
  explicit DataWriterManager(const DataWriterConfigSeq& configs, DataWriterReportSeq& reports, DDS::Publisher_var& publisher, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map);

  void enable();

protected:
  std::vector<std::shared_ptr<DataWriter>> datawriters_;
};

}

