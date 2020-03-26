#include "Action.h"

namespace Bench {

bool Action::init(const Bench::ActionConfig& config, Bench::ActionReport& report, Builder::ReaderMap& reader_map, Builder::WriterMap& writer_map) {
  config_ = &config;
  report_ = &report;
  for (CORBA::ULong j = 0; j < config.readers.length(); ++j) {
    auto it = reader_map.find(config.readers[j].in());
    if (it != reader_map.end()) {
      readers_by_name_.insert(*it);
      readers_by_index_.push_back(it->second);
    } else {
      return false;
    }
  }
  for (CORBA::ULong j = 0; j < config.writers.length(); ++j) {
    auto it = writer_map.find(config.writers[j].in());
    if (it != writer_map.end()) {
      writers_by_name_.insert(*it);
      writers_by_index_.push_back(it->second);
    } else {
      return false;
    }
  }
  return true;
};

}

