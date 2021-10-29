#ifndef RTPSRELAY_SPDP_REPLAY_LISTENER_H_
#define RTPSRELAY_SPDP_REPLAY_LISTENER_H_

#include "ListenerBase.h"
#include "RelayHandler.h"
#include "RelayStatisticsReporter.h"

namespace RtpsRelay {

class SpdpReplayListener : public ListenerBase {
public:
  SpdpReplayListener(SpdpHandler& spdp_handler,
    RelayStatisticsReporter& relay_statistics_reporter);

private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void on_subscription_matched(DDS::DataReader_ptr reader,
                               const DDS::SubscriptionMatchedStatus& status) override;

  SpdpHandler& spdp_handler_;
  RelayStatisticsReporter& relay_statistics_reporter_;
};

}

#endif // RTPSRELAY_SPDP_REPLAY_LISTENER_H_
