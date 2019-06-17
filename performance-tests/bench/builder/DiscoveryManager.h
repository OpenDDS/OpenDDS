#pragma once

#include "Discovery.h"

#include <vector>

namespace Builder {

class DiscoveryManager {
public:
  explicit DiscoveryManager(const DiscoveryConfigSeq& seq);

protected:
  std::vector<std::shared_ptr<Discovery>> discoveries_;
};

}

