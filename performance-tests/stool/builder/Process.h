#pragma once

#include "Common.h"
#include "ConfigSectionManager.h"
#include "DiscoveryManager.h"
#include "TransportInstanceManager.h"
#include "ParticipantManager.h"

namespace Builder {

class Stool_Builder_Export Process {
public:
  explicit Process(const ProcessConfig& config);
  ~Process();

  void enable_dds_entities();

  ProcessReport& get_report() { return report_; }
  const ProcessReport& get_report() const { return report_; }

protected:
  ProcessReport report_;
  ReaderMap reader_map_;
  WriterMap writer_map_;
  std::shared_ptr<ConfigSectionManager> config_sections_;
  std::shared_ptr<DiscoveryManager> discoveries_;
  std::shared_ptr<TransportInstanceManager> instances_;
  std::shared_ptr<ParticipantManager> participants_;
};

}

