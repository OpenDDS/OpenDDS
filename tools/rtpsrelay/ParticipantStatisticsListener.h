#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_LISTENER_H_

#include "ListenerBase.h"

namespace RtpsRelay {

class ParticipantStatisticsListener : public ListenerBase {
public:

private:
  void on_data_available(DDS::DataReader_ptr reader) override;
};
}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_LISTENER_H_
