#ifndef RTPSRELAY_PARTICIPANT_ENTRY_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_ENTRY_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "RelayHandler.h"
#include "DomainStatisticsReporter.h"

namespace RtpsRelay {

class ParticipantEntryListener : public ListenerBase {
public:
  ParticipantEntryListener(const Config& config,
                           DomainStatisticsReporter& stats_reporter);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  const Config& config_;
  size_t participant_count_;
  DomainStatisticsReporter& stats_reporter_;
};

}

#endif // RTPSRELAY_PARTICIPANT_ENTRY_LISTENER_H_
