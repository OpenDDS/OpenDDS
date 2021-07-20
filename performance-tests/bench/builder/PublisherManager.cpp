#include "PublisherManager.h"

namespace Builder {

PublisherManager::PublisherManager(const PublisherConfigSeq& configs, PublisherReportSeq& reports,
  DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map)
{
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    publishers_.emplace_back(std::make_shared<Publisher>(configs[i], reports[i], participant, topics, writer_map));
  }
}

bool PublisherManager::enable(bool throw_on_error)
{
  bool result = true;
  for (auto it = publishers_.begin(); it != publishers_.end(); ++it) {
    result &= (*it)->enable(throw_on_error);
  }
  return result;
}

void PublisherManager::detach_listeners()
{
  for (auto it = publishers_.begin(); it != publishers_.end(); ++it) {
    (*it)->detach_listeners();
  }
}

}
