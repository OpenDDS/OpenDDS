#pragma once

#include "Participant.h"

namespace Builder {

class ParticipantManager {
public:
  explicit ParticipantManager(const ParticipantConfigSeq& configs, ParticipantReportSeq& reports, ReaderMap& reader_map, WriterMap& writer_map);

  bool enable(bool throw_on_error = false);

protected:
  std::vector<std::shared_ptr<Participant>> participants_;
};

}

