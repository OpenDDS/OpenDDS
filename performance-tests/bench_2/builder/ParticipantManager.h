#pragma once

#include "Participant.h"

namespace Builder {

class ParticipantManager {
public:
  explicit ParticipantManager(const ParticipantConfigSeq& configs, ParticipantReportSeq& reports,
    ReaderMap& reader_map, WriterMap& writer_map, ContentFilteredTopicMap& cft_map);

  bool enable(bool throw_on_error = false);
  void detach_listeners();

protected:
  std::vector<std::shared_ptr<Participant>> participants_;
};

}
