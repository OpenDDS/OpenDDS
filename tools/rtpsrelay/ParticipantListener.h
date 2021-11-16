#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ListenerBase.h"
#include "RelayStatisticsReporter.h"
#include "RelayHandler.h"

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/TimeTypes.h>

namespace RtpsRelay {

class ParticipantListener : public ListenerBase {
public:
  ParticipantListener(const Config& config,
                      GuidAddrSet& guid_addr_set,
                      OpenDDS::DCPS::DomainParticipantImpl* participant,
                      RelayStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  const Config& config_;
  GuidAddrSet& guid_addr_set_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  RelayStatisticsReporter& stats_reporter_;
  std::unordered_set<OpenDDS::DCPS::GUID_t> guids_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
