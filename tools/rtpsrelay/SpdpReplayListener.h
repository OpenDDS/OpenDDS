#ifndef RTPSRELAY_SPDP_REPLAY_LISTENER_H_
#define RTPSRELAY_SPDP_REPLAY_LISTENER_H_

#include "ListenerBase.h"
#include "RelayHandler.h"

namespace RtpsRelay {

class SpdpReplayListener : public ListenerBase {
public:
  SpdpReplayListener(SpdpHandler& spdp_handler);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  SpdpHandler& spdp_handler_;
};

}

#endif // RTPSRELAY_SPDP_REPLAY_LISTENER_H_
