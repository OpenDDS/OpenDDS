#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ReaderListenerBase.h"
#include "RelayParticipantStatusReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class ParticipantListener : public ReaderListenerBase {
public:
  ParticipantListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                      const GuidAddrSet_rch& guid_addr_set,
                      RelayParticipantStatusReporter& participant_status_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  GuidAddrSet_rch guid_addr_set_;
  RelayParticipantStatusReporter& participant_status_reporter_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
