#ifndef RTPSRELAY_RELAY_PARTICIPANT_STATUS_REPORTER_H_
#define RTPSRELAY_RELAY_PARTICIPANT_STATUS_REPORTER_H_

#include "GuidAddrSet.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>

#include <unordered_map>

namespace RtpsRelay {

class RelayParticipantStatusReporter {
public:

  RelayParticipantStatusReporter(const Config& config,
                                 RelayParticipantStatusDataWriter_var writer,
                                 RelayStatisticsReporter& stats_reporter)
    : config_(config)
    , writer_(writer)
    , stats_reporter_(stats_reporter)
  {}

  void add_participant(GuidAddrSet::Proxy& proxy,
                       const OpenDDS::DCPS::GUID_t& repoid,
                       const DDS::ParticipantBuiltinTopicData& data);

  void remove_participant(GuidAddrSet::Proxy& proxy,
                          const OpenDDS::DCPS::GUID_t& repoid);

  void set_alive(const GuidAddrSet::Proxy& proxy,
                 const OpenDDS::DCPS::GUID_t& repoid,
                 bool alive);

  void set_active(const GuidAddrSet::Proxy& proxy,
                  const OpenDDS::DCPS::GUID_t& repoid,
                  bool active);

  void set_alive_active(const GuidAddrSet::Proxy& proxy,
                        const OpenDDS::DCPS::GUID_t& repoid,
                        bool alive,
                        bool active);

private:

  const Config& config_;
  RelayParticipantStatusDataWriter_var writer_;
  RelayStatisticsReporter& stats_reporter_;

  mutable ACE_Thread_Mutex mutex_;
  std::unordered_map<OpenDDS::DCPS::GUID_t, RelayParticipantStatus> guids_;
};

}

#endif // RTPSRELAY_RELAY_PARTICIPANT_STATUS_REPORTER_H_
