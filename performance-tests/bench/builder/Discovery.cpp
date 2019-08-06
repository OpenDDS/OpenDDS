#include "Discovery.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"

#include <iostream>

namespace Builder {

Discovery::Discovery(const DiscoveryConfig& config)
  : name_(config.name.in())
{
  Log::log() << "Creating discovery '" << name_ << "' of type " << config.type << std::endl;
  if (std::string(config.type.in()) == "rtps") {
    OpenDDS::RTPS::RtpsDiscovery_rch disc = OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>(config.name.in());
    TheServiceParticipant->add_discovery(OpenDDS::DCPS::static_rchandle_cast<OpenDDS::DCPS::Discovery>(disc));
    TheServiceParticipant->set_repo_domain(config.domain, disc->key());
  } else if (std::string(config.type.in()) == "repo") {
    OpenDDS::DCPS::InfoRepoDiscovery_rch disc = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::InfoRepoDiscovery>(config.name.in(), config.ior.in());
    TheServiceParticipant->add_discovery(OpenDDS::DCPS::static_rchandle_cast<OpenDDS::DCPS::Discovery>(disc));
    TheServiceParticipant->set_repo_domain(config.domain, disc->key());
  } else {
    throw std::runtime_error("unsupported discovery type");
  }
}

}

