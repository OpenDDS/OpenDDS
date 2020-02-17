#include "ParticipantManager.h"

namespace Builder {

ParticipantManager::ParticipantManager(const ParticipantConfigSeq& configs, ParticipantReportSeq& reports, ReaderMap& reader_map, WriterMap& writer_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    participants_.emplace_back(std::make_shared<Participant>(configs[i], reports[i], reader_map, writer_map));
  }
}

void ParticipantManager::enable() {
  for (auto it = participants_.begin(); it != participants_.end(); ++it) {
    (*it)->enable();
  }
}

}

