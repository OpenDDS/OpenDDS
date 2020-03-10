#include "DataReaderManager.h"

namespace Builder {

DataReaderManager::DataReaderManager(const DataReaderConfigSeq& configs, DataReaderReportSeq& reports, DDS::Subscriber_var& subscriber, const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    auto it = reader_map.find(configs[i].name.in());
    if (it != reader_map.end()) {
      std::stringstream ss;
      ss << "reader with name '" << configs[i].name << "' already exists in process reader map" << std::flush;
      throw std::runtime_error(ss.str());
    }
    auto reader = std::make_shared<DataReader>(configs[i], reports[i], subscriber, topics);
    datareaders_.push_back(reader);
    reader_map[configs[i].name.in()] = reader;
  }
}

void DataReaderManager::enable() {
  for (auto it = datareaders_.begin(); it != datareaders_.end(); ++it) {
    (*it)->enable();
  }
}

}

