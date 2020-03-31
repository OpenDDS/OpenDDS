#ifndef RTPSRELAY_HANDLER_STATISTICS_LISTENER_H_
#define RTPSRELAY_HANDLER_STATISTICS_LISTENER_H_

#include "ListenerBase.h"

namespace RtpsRelay {

class HandlerStatisticsListener : public ListenerBase {
public:
  explicit HandlerStatisticsListener(bool report_participant_statistics)
    : report_participant_statistics_(report_participant_statistics)
  {}

private:
  void on_data_available(DDS::DataReader_ptr reader) override;

  const bool report_participant_statistics_;
};
}

#endif // RTPSRELAY_HANDLER_STATISTICS_LISTENER_H_
