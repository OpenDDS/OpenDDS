#pragma once

#include "DataReader.h"

#include <vector>

namespace Builder {

class DataReaderManager {
public:
  explicit DataReaderManager(const DataReaderConfigSeq& configs, DataReaderReportSeq& reports, DDS::Subscriber_var& subscriber,
    const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map, const ContentFilteredTopicMap& cft_map);

  bool enable(bool throw_on_error = false);
  void detach_listeners();

protected:
  std::vector<std::shared_ptr<DataReader>> datareaders_;
};

}
