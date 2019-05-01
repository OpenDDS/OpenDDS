#pragma once

#include "TransportInstance.h"

namespace Builder {

class TransportInstanceManager {
public:
  explicit TransportInstanceManager(const TransportInstanceConfigSeq& seq);

protected:
  std::vector<std::shared_ptr<TransportInstance>> instances_;
};

}

