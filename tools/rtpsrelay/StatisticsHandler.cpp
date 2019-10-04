#include "StatisticsHandler.h"

#include <ace/Reactor.h>

#include <iostream>

namespace RtpsRelay {

namespace {
  const ACE_Time_Value STATISTICS_INTERVAL(60);
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
  , m_last_collection(ACE_Time_Value().now())
{}

void StatisticsHandler::open()
{
  reactor()->schedule_timer(this, 0, STATISTICS_INTERVAL, STATISTICS_INTERVAL);
}

int StatisticsHandler::handle_timeout(const ACE_Time_Value& a_now, const void*)
{
  const auto duration = a_now - m_last_collection;
  m_last_collection = a_now;

  std::cout << "SPDP VERTICAL "
            << spdp_vertical_->bytes_received() << " bytes in "
            << spdp_vertical_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SPDP HORIZONTAL "
            << spdp_horizontal_->bytes_received() << " bytes in "
            << spdp_horizontal_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SEDP VERTICAL "
            << sedp_vertical_->bytes_received() << " bytes in "
            << sedp_vertical_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SEDP HORIZONTAL "
            << sedp_horizontal_->bytes_received() << " bytes in "
            << sedp_horizontal_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "DATA VERTICAL "
            << data_vertical_->bytes_received() << " bytes in "
            << data_vertical_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "DATA HORIZONTAL "
            << data_horizontal_->bytes_received() << " bytes in "
            << data_horizontal_->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  spdp_vertical_->reset_byte_counts();
  spdp_horizontal_->reset_byte_counts();
  sedp_vertical_->reset_byte_counts();
  sedp_horizontal_->reset_byte_counts();
  data_vertical_->reset_byte_counts();
  data_horizontal_->reset_byte_counts();

  return 0;
}

}
