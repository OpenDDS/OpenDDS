#include "DataWriterManager.h"

namespace Builder {

DataWriterManager::DataWriterManager(const DataWriterConfigSeq& configs, DataWriterReportSeq& reports, DDS::Publisher_var& publisher, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    auto it = writer_map.find(configs[i].name.in());
    if (it != writer_map.end()) {
      std::stringstream ss;
      ss << "writer with name '" << configs[i].name << "' already exists in process writer map" << std::flush;
      throw std::runtime_error(ss.str());
    }
    auto writer = std::make_shared<DataWriter>(configs[i], reports[i], publisher, topics);
    datawriters_.push_back(writer);
    writer_map[configs[i].name.in()] = writer;
  }
}

void DataWriterManager::enable() {
  for (auto it = datawriters_.begin(); it != datawriters_.end(); ++it) {
    (*it)->enable();
  }
}

}

