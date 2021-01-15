#include "ParticipantManager.h"

namespace Builder {

ParticipantManager::ParticipantManager(const ParticipantConfigSeq& configs, ParticipantReportSeq& reports,
  ReaderMap& reader_map, WriterMap& writer_map, ContentFilteredTopicMap& cft_map)
{
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    participants_.emplace_back(std::make_shared<Participant>(configs[i], reports[i], reader_map, writer_map, cft_map));
  }
}

bool ParticipantManager::enable(bool throw_on_error)
{
  bool result = true;
  for (auto it = participants_.begin(); it != participants_.end(); ++it) {
    result &= (*it)->enable(throw_on_error);
  }
  return result;
}

void ParticipantManager::detach_listeners()
{
  for (auto it = participants_.begin(); it != participants_.end(); ++it) {
    (*it)->detach_listeners();
  }
}

}
