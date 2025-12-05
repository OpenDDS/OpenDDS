#ifndef RTPSRELAY_RELAY_STATUS_REPORTER_H_
#define RTPSRELAY_RELAY_STATUS_REPORTER_H_

#include "Config.h"
#include "GuidAddrSet.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>

namespace RtpsRelay {

class RelayStatusReporter : public ACE_Event_Handler
                          , public OpenDDS::DCPS::ConfigListener {
public:
  RelayStatusReporter(const Config& config,
                      GuidAddrSet& guid_addr_set,
                      DDS::Publisher_var relay_publisher,
                      ACE_Reactor* reactor,
                      RelayStatisticsReporter& relay_statistics_reporter);
  bool setup_writer();

private:
  void on_data_available(InternalDataReader_rch reader) override;

  bool setup_writer_i();

  int handle_timeout(const ACE_Time_Value& now, const void* token) override;

private:
  GuidAddrSet& guid_addr_set_;
  const DDS::Publisher_var relay_publisher_;
  RelayStatisticsReporter& relay_statistics_reporter_;

  ACE_Thread_Mutex lock_;
  RelayStatus relay_status_;
  RelayStatusDataWriter_var writer_;
  OpenDDS::DCPS::TimeDuration publish_relay_status_, publish_relay_status_liveliness_;
};

}

#endif // RTPSRELAY_RELAY_STATUS_REPORTER_H_
