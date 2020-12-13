#pragma once

#include "Publisher.h"

#include <vector>

namespace Builder {

class PublisherManager {
public:
  explicit PublisherManager(const PublisherConfigSeq& configs, PublisherReportSeq& reports, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map);

  bool enable(bool throw_on_error = false);
  void detach_listeners();

protected:
  std::vector<std::shared_ptr<Publisher>> publishers_;
};

}
