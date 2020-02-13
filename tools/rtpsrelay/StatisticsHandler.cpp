#include "StatisticsHandler.h"

#include <ace/Reactor.h>

#include <iostream>

namespace RtpsRelay {

namespace {
  const OpenDDS::DCPS::TimeDuration STATISTICS_INTERVAL(60);
}

StatisticsHandler::StatisticsHandler(ACE_Reactor* a_reactor,
                                     RelayHandler* a_spdp_vertical, RelayHandler* a_spdp_horizontal,
                                     RelayHandler* a_sedp_vertical, RelayHandler* a_sedp_horizontal,
                                     RelayHandler* a_data_vertical, RelayHandler* a_data_horizontal)
  : ACE_Event_Handler(a_reactor)
  , spdp_vertical_(a_spdp_vertical)
  , spdp_horizontal_(a_spdp_horizontal)
  , sedp_vertical_(a_sedp_vertical)
  , sedp_horizontal_(a_sedp_horizontal)
  , data_vertical_(a_data_vertical)
  , data_horizontal_(a_data_horizontal)
  , m_last_collection(OpenDDS::DCPS::MonotonicTimePoint::now())
{}

void StatisticsHandler::open()
{
  reactor()->schedule_timer(this, 0, STATISTICS_INTERVAL.value(), STATISTICS_INTERVAL.value());
}

int StatisticsHandler::handle_timeout(const ACE_Time_Value& a_now, const void*)
{
  const OpenDDS::DCPS::MonotonicTimePoint now(a_now);
  const auto duration = now - m_last_collection;
  m_last_collection = now;

  std::cout << "SPDP VERTICAL "
            << spdp_vertical_->bytes_received() << " bytes in "
            << spdp_vertical_->messages_received() << " messages in "
            << spdp_vertical_->bytes_sent() << " bytes out "
            << spdp_vertical_->messages_sent() << " messages out "
            << spdp_vertical_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  std::cout << "SPDP HORIZONTAL "
            << spdp_horizontal_->bytes_received() << " bytes in "
            << spdp_horizontal_->messages_received() << " messages in "
            << spdp_horizontal_->bytes_sent() << " bytes out "
            << spdp_horizontal_->messages_sent() << " messages out "
            << spdp_horizontal_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  std::cout << "SEDP VERTICAL "
            << sedp_vertical_->bytes_received() << " bytes in "
            << sedp_vertical_->messages_received() << " messages in "
            << sedp_vertical_->bytes_sent() << " bytes out "
            << sedp_vertical_->messages_sent() << " messages out "
            << sedp_vertical_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  std::cout << "SEDP HORIZONTAL "
            << sedp_horizontal_->bytes_received() << " bytes in "
            << sedp_horizontal_->messages_received() << " messages in "
            << sedp_horizontal_->bytes_sent() << " bytes out "
            << sedp_horizontal_->messages_sent() << " messages out "
            << sedp_horizontal_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  std::cout << "DATA VERTICAL "
            << data_vertical_->bytes_received() << " bytes in "
            << data_vertical_->messages_received() << " messages in "
            << data_vertical_->bytes_sent() << " bytes out "
            << data_vertical_->messages_sent() << " messages out "
            << data_vertical_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  std::cout << "DATA HORIZONTAL "
            << data_horizontal_->bytes_received() << " bytes in "
            << data_horizontal_->messages_received() << " messages in "
            << data_horizontal_->bytes_sent() << " bytes out "
            << data_horizontal_->messages_sent() << " messages out "
            << data_horizontal_->max_fan_out() << " max fan out "
            << duration.value().sec() << '.' << duration.value().usec() << " seconds" << std::endl;

  spdp_vertical_->reset_counters();
  spdp_horizontal_->reset_counters();
  sedp_vertical_->reset_counters();
  sedp_horizontal_->reset_counters();
  data_vertical_->reset_counters();
  data_horizontal_->reset_counters();

  return 0;
}

}
