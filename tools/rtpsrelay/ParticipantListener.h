#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ListenerBase.h"
#include "RelayParticipantStatusReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class ParticipantListener : public ListenerBase {
public:
  ParticipantListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                      GuidAddrSet& guid_addr_set,
                      RelayParticipantStatusReporter& participant_status_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  GuidAddrSet& guid_addr_set_;
  RelayParticipantStatusReporter& participant_status_reporter_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
