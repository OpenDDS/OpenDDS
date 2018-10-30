#include "StatisticsHandler.h"

#include <iostream>

#include <ace/Reactor.h>

namespace {
  const ACE_Time_Value STATISTICS_INTERVAL(60);
}

StatisticsHandler::StatisticsHandler(ACE_Reactor * a_reactor,
                                     RelayHandler * a_spdp_vertical, RelayHandler * a_spdp_horizontal,
                                     RelayHandler * a_sedp_vertical, RelayHandler * a_sedp_horizontal,
                                     RelayHandler * a_data_vertical, RelayHandler * a_data_horizontal)
  : ACE_Event_Handler(a_reactor)
  , m_spdp_vertical(a_spdp_vertical)
  , m_spdp_horizontal(a_spdp_horizontal)
  , m_sedp_vertical(a_sedp_vertical)
  , m_sedp_horizontal(a_sedp_horizontal)
  , m_data_vertical(a_data_vertical)
  , m_data_horizontal(a_data_horizontal)
  , m_last_collection(ACE_Time_Value().now()) {}

void StatisticsHandler::open() {
  this->reactor()->schedule_timer(this, 0, STATISTICS_INTERVAL, STATISTICS_INTERVAL);
}

int StatisticsHandler::handle_timeout(ACE_Time_Value const & a_now, void const *) {
  ACE_Time_Value duration = a_now - m_last_collection;
  m_last_collection = a_now;

  std::cout << "SPDP VERTICAL "
            << m_spdp_vertical->bytes_received() << " bytes in "
            << m_spdp_vertical->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SPDP HORIZONTAL "
            << m_spdp_horizontal->bytes_received() << " bytes in "
            << m_spdp_horizontal->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SEDP VERTICAL "
            << m_sedp_vertical->bytes_received() << " bytes in "
            << m_sedp_vertical->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "SEDP HORIZONTAL "
            << m_sedp_horizontal->bytes_received() << " bytes in "
            << m_sedp_horizontal->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "DATA VERTICAL "
            << m_data_vertical->bytes_received() << " bytes in "
            << m_data_vertical->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  std::cout << "DATA HORIZONTAL "
            << m_data_horizontal->bytes_received() << " bytes in "
            << m_data_horizontal->bytes_sent() << " bytes out "
            << duration.sec() << '.' << duration.usec() << " seconds" << std::endl;

  m_spdp_vertical->reset_byte_counts();
  m_spdp_horizontal->reset_byte_counts();
  m_sedp_vertical->reset_byte_counts();
  m_sedp_horizontal->reset_byte_counts();
  m_data_vertical->reset_byte_counts();
  m_data_horizontal->reset_byte_counts();

  return 0;
}
