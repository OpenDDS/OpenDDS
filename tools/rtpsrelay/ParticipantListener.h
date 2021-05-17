#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ListenerBase.h"
#include "DomainStatisticsReporter.h"
#include "RelayHandler.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class ParticipantListener : public ListenerBase {
public:
  ParticipantListener(const Config& config,
                      ClaimableGuidAddrSet& guid_addr_set,
                      OpenDDS::DCPS::DomainParticipantImpl* participant,
                      DomainStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  const Config& config_;
  ClaimableGuidAddrSet& guid_addr_set_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  DomainStatisticsReporter& stats_reporter_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
