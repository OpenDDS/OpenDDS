#include "TransportInstanceManager.h"

namespace Builder {

TransportInstanceManager::TransportInstanceManager(const TransportInstanceConfigSeq& seq) {
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    instances_.emplace_back(std::make_shared<TransportInstance>(seq[i]));
  }
}

}

