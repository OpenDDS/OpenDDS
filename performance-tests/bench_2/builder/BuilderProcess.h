#pragma once

#include "Common.h"
#include "ConfigSectionManager.h"
#include "DiscoveryManager.h"
#include "TransportInstanceManager.h"
#include "ParticipantManager.h"

namespace Builder {

class Bench_Builder_Export BuilderProcess {
public:
  explicit BuilderProcess(const ProcessConfig& config);
  ~BuilderProcess();

  bool enable_dds_entities(bool throw_on_error = false);

  void detach_listeners();

  ProcessReport& get_report() { return report_; }
  const ProcessReport& get_report() const { return report_; }

  ReaderMap& get_reader_map() { return reader_map_; }
  const ReaderMap& get_reader_map() const { return reader_map_; }

  WriterMap& get_writer_map() { return writer_map_; }
  const WriterMap& get_writer_map() const { return writer_map_; }

  ContentFilteredTopicMap& get_cft_map() { return cft_map_; }
  const ContentFilteredTopicMap& get_cft_map() const { return cft_map_; }

protected:
  ProcessReport report_;
  ReaderMap reader_map_;
  WriterMap writer_map_;
  ContentFilteredTopicMap cft_map_;
  std::shared_ptr<ConfigSectionManager> config_sections_;
  std::shared_ptr<DiscoveryManager> discoveries_;
  std::shared_ptr<TransportInstanceManager> instances_;
  std::shared_ptr<ParticipantManager> participants_;
};

}
