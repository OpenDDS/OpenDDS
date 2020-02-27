#include "DiscoveryManager.h"

namespace Builder {

DiscoveryManager::DiscoveryManager(const DiscoveryConfigSeq& seq) {
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    discoveries_.emplace_back(std::make_shared<Discovery>(seq[i]));
  }
}

}

