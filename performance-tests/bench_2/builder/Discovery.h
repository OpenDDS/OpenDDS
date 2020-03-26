#pragma once

#include "Common.h"

namespace Builder {

class Discovery {
public:
  explicit Discovery(const DiscoveryConfig& config);

protected:
  std::string name_;
};

}

