#include "BuilderProcess.h"

#include "dds/DCPS/Service_Participant.h"

namespace Builder {

BuilderProcess::BuilderProcess(const ProcessConfig& config)
  : config_sections_(std::make_shared<ConfigSectionManager>(config.name.in(), config.config_sections))
  , participants_(std::make_shared<ParticipantManager>(config.participants, report_.participants, reader_map_, writer_map_, cft_map_))
{
}

BuilderProcess::~BuilderProcess()
{
  reader_map_.clear();
  writer_map_.clear();
  cft_map_.clear();

  participants_.reset();
  TheServiceParticipant->shutdown();
  config_sections_.reset();
}

void BuilderProcess::detach_listeners()
{
  participants_->detach_listeners();
}

bool BuilderProcess::enable_dds_entities(bool throw_on_error)
{
  return participants_->enable(throw_on_error);
}

}
