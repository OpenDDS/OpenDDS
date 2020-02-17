#pragma once

#include "Common.h"

#include "dds/DCPS/transport/framework/TransportConfig_rch.h"

namespace Builder {

class TransportInstance {
public:
  explicit TransportInstance(const TransportInstanceConfig& config);
  ~TransportInstance();

protected:
  std::string name_;
  OpenDDS::DCPS::TransportConfig_rch config_;
  OpenDDS::DCPS::TransportInst_rch inst_;
};

}

