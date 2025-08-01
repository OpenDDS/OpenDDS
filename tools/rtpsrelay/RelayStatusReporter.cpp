#include "RelayStatusReporter.h"

namespace RtpsRelay {

RelayStatusReporter::RelayStatusReporter(const Config& config,
                                         GuidAddrSet& guid_addr_set,
                                         RelayStatusDataWriter_var writer,
                                         ACE_Reactor* reactor)
  : ACE_Event_Handler(reactor)
  , config_(config)
  , guid_addr_set_(guid_addr_set)
  , writer_(writer)
{
  relay_status_.relay_id(config.relay_id());

  if (config.publish_relay_status() != OpenDDS::DCPS::TimeDuration::zero_value) {
    this->reactor()->schedule_timer(this, 0, ACE_Time_Value(), config.publish_relay_status().value());
  }
}

int RelayStatusReporter::handle_timeout(const ACE_Time_Value&, const void*)
{
  if (config_.log_activity()) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RelayStatusReporter::handle_timeout\n"));
  }
  OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager(), TIMER_MASK);

  {
    GuidAddrSet::Proxy proxy(guid_addr_set_);
    proxy.populate_relay_status(relay_status_);
  }

  if (writer_->write(relay_status_, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayStatusReporter::handle_timeout failed to write Relay Status\n"));
  }

  return 0;
}

}
