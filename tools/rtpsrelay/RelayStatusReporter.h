#ifndef RTPSRELAY_RELAY_STATUS_REPORTER_H_
#define RTPSRELAY_RELAY_STATUS_REPORTER_H_

#include "Config.h"
#include "GuidAddrSet.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>

namespace RtpsRelay {

class ConfigObserver {
public:
  // The changes passed to this method are valid samples as verified by the Config instance.
  // No need to check again.
  //void on_config_changed(const OpenDDS::DCPS::ConfigReader::SampleSequence& changes) = 0;
  void on_config_changed(const std::string& key, const std::string& value) = 0;
};

class RelayStatusReporter : public ACE_Event_Handler
                          , public ConfigObserver {
                          //, public OpenDDS::DCPS::ConfigListener {
public:
  RelayStatusReporter(const Config& config,
                      GuidAddrSet& guid_addr_set,
                      DDS::Publisher_var relay_publisher,
                      ACE_Reactor* reactor,
                      RelayStatisticsReporter& relay_statistics_reporter);
  bool setup_writer();

private:
  //void on_data_available(InternalDataReader_rch reader) override;
  //void on_config_changed(const OpenDDS::DCPS::ConfigReader::SampleSequence& changes) override;
  void on_config_changed(const std::string& key, const std::string& value) override;

  bool setup_writer_i(bool liveliness_changed = false);

  int handle_timeout(const ACE_Time_Value& now, const void* token) override;

  const Config& config_;
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
